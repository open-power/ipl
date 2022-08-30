#include "libphal.H"
#include "log.H"
#include "phal_exception.H"
#include "utils_buffer.H"
#include "utils_pdbg.H"
#include "utils_tempfile.H"

#include <ekb/chips/p10/procedures/hwp/perv/p10_sbe_hreset.H>
#include <ekb/chips/p10/procedures/hwp/sbe/p10_get_sbe_msg_register.H>
#include <ekb/hwpf/fapi2/include/return_code_defs.H>
#include <unistd.h>
#include <ekb/hwpf/fapi2/include/return_code_defs.H>
#include <ekb/chips/p10/procedures/hwp/perv/p10_extract_sbe_rc.H>

namespace openpower::phal
{
namespace sbe
{

using namespace openpower::phal::logging;
using namespace openpower::phal;
using namespace openpower::phal::utils::pdbg;
using namespace openpower::phal::pdbg;

void sbeErrorHandler(struct pdbg_target *proc);

/**
 * @brief helper function to log SBE debug data
 *
 * @param[in] proc processor target to operate on
 */
void logSbeDebugData(struct pdbg_target *proc)
{
	std::array<const uint32_t, 5> cfamAddr = {0x2986, 0x2809, 0x282A,
						  0x2829, 0x1007};
	for (const uint32_t addr : cfamAddr) {
		try {
			uint32_t val = 0xFFFFFFFF; // Invalid value
			val = getCFAM(proc, addr);
			if (val != 0xFFFFFFFF) {
				log(level::INFO, "%s CFAM(0x%X) : 0x%X",
				    pdbg_target_path(proc), addr, val);
			}
		} catch (...) {
		}
	}
}

/**
 * @brief helper function to perform HRESET on secondary processor
 *        if sbe is in HALT state.
 * @param[in] proc processor target to operate on
 *
 * Exceptions: SbeError/PdbgError with failure reason code
 */
void sbeHaltStateRecovery(struct pdbg_target *proc)
{
	// Skip for primary processor.
	if (isPrimaryProc(proc)) {
		return;
	}
	// Required to probe processor associated fsi to run hardware procedures
	char path[16];
	struct pdbg_target *fsi;
	sprintf(path, "/proc%d/fsi", pdbg_target_index(proc));
	fsi = pdbg_target_from_path(NULL, path);
	if (fsi == nullptr) {
		log(level::ERROR, "Failed to get fsi target on (%s)",
		    pdbg_target_path(proc));
		throw pdbgError_t(exception::PDBG_TARGET_NOT_OPERATIONAL);
	}

	if (pdbg_target_probe(fsi) != PDBG_TARGET_ENABLED) {
		log(level::ERROR, "fsi(%s) probe: fail to enable",
		    pdbg_target_path(fsi));
		throw pdbgError_t(exception::PDBG_TARGET_NOT_OPERATIONAL);
	}

	// get SBE current state based on sbe message register
	sbeMsgReg_t sbeReg;
	fapi2::ReturnCode fapiRC;
	fapiRC = p10_get_sbe_msg_register(proc, sbeReg);
	if (fapiRC != fapi2::FAPI2_RC_SUCCESS) {
		log(level::ERROR,
		    "Failed: checking SBE state in Halt (%s) fapiRC=%x",
		    pdbg_target_path(proc), fapiRC);
		throw sbeError_t(exception::SBE_STATE_READ_FAIL);
	}

	// Check SBE state is in halt
	if (sbeReg.currState != SBE_STATE_HALT) {
		// No special actions required
		return;
	}

	log(level::INFO, "SBE(%s) HRESET Requested", pdbg_target_path(proc));

	// update SBE state to CHECK_CFAM.
	setState(proc, SBE_STATE_CHECK_CFAM);

	// SBE is in Halt state, call p10_sbe_reset hwp and re-check state
	fapiRC = p10_sbe_hreset(proc, true);
	if (fapiRC != fapi2::FAPI2_RC_SUCCESS) {
		log(level::ERROR, "failed hreset hwp(%s), fapiRC=%x",
		    pdbg_target_path(proc), fapiRC);
		throw sbeError_t(exception::HWP_EXECUTION_FAILED);
	}

	// wait for 5 seconds for the change in SBE state
	sleep(5);

	// Check sbe current state using message register
	fapiRC = p10_get_sbe_msg_register(proc, sbeReg);
	if (fapiRC != fapi2::FAPI2_RC_SUCCESS) {
		log(level::ERROR,
		    "Failed: Checking SBE state procedure (%s) fapiRC=%x",
		    pdbg_target_path(proc), fapiRC);
		throw sbeError_t(exception::SBE_STATE_READ_FAIL);
	}
	if (sbeReg.currState == SBE_STATE_RUNTIME) {
		log(level::INFO, "SBE (%s), recoverd from halt state",
		    pdbg_target_path(proc));
		setState(proc, SBE_STATE_BOOTED);
	} else {
		log(level::ERROR, "SBE (%s), Fail to recover from halt state",
		    pdbg_target_path(proc));
		logSbeDebugData(proc);
	}
}

void validateSBEState(struct pdbg_target *proc)
{
	// In order to allow SBE operation following conditions should be met
	// 1. Caller has to make sure processor target should be functional
	// or dump functional
	// 2. Processor's SBE state should be marked as BOOTED or CHECK_CFAM
	// 3. In case the SBE state is marked as CHECK_CFAM then SBE's
	// PERV_SB_MSG_FSI register must be checked to see if SBE is booted or
	// NOT.

	// get PIB target
	struct pdbg_target *pib = getPibTarget(proc);

	// Get the current SBE state
	enum sbe_state state;

	if (sbe_get_state(pib, &state)) {
		log(level::ERROR, "Failed to read SBE state information (%s)",
		    pdbg_target_path(proc));
		throw sbeError_t(exception::SBE_STATE_READ_FAIL);
	}

	// SBE_STATE_CHECK_CFAM case is already handled by pdbg api
	if (state == SBE_STATE_BOOTED) {
		return;
	} else {
		log(level::INFO,
		    "SBE (%s) is not ready for chip-op: state(0x%08x)",
		    pdbg_target_path(proc), state);
		throw sbeError_t(exception::SBE_CHIPOP_NOT_ALLOWED);
	}
}
void setState(struct pdbg_target *proc, enum sbe_state state)
{
	// get PIB target
	struct pdbg_target *pib = getPibTarget(proc);
	if (sbe_set_state(pib, state)) {
		log(level::ERROR,
		    "Failed to set SBE state(%d) information (%s)", state,
		    pdbg_target_path(proc));
		throw sbeError_t(exception::SBE_STATE_WRITE_FAIL);
	}
}

enum sbe_state getState(struct pdbg_target *proc)
{
	// get PIB target
	struct pdbg_target *pib = getPibTarget(proc);
	enum sbe_state state = SBE_STATE_NOT_USABLE; // default value

	if (sbe_get_state(pib, &state)) {
		log(level::ERROR, "Failed to get SBE state information (%s)",
		    pdbg_target_path(proc));
		throw sbeError_t(exception::SBE_STATE_READ_FAIL);
	}
	return state;
}

bool isPrimaryIplDone()
{
	struct pdbg_target *proc, *pib;
	try {
		// get primary processor target
		proc = getPrimaryProc();

		// get pib target associated to primary processor
		pib = getPibTarget(proc);
	} catch (const pdbgError_t &e) {
		// throw sbeError type exception with same error
		throw sbeError_t(e.errType());
	}

	bool done = false;

	if (sbe_is_ipl_done(pib, &done)) {
		log(level::ERROR, "Failed: sbe_is_ipl_done (%s)",
		    pdbg_target_path(proc));
		throw sbeError_t(exception::SBE_STATE_READ_FAIL);
	}
	return done;
}

bool isDumpAllowed(struct pdbg_target *proc)
{
	// get SBE state
	enum sbe_state state = getState(proc);
	bool allowed = true;

	if (state == SBE_STATE_DEBUG_MODE) {
		allowed = false;
	}
	return allowed;
}

sbeError_t captureFFDC(struct pdbg_target *proc)
{
	// get SBE FFDC info
	bufPtr_t bufPtr;
	uint32_t ffdcLen = 0;
	uint32_t status = 0;

	// Log SBE debug data.
	logSbeDebugData(proc);

	// get PIB target
	struct pdbg_target *pib = getPibTarget(proc);

	if (sbe_ffdc_get(pib, &status, bufPtr.getPtr(), &ffdcLen)) {
		log(level::ERROR, "sbe_ffdc_get function failed");
		throw sbeError_t(exception::SBE_FFDC_GET_FAILED);
	}
	// TODO Need to remove this once pdbg header file support in place
	const auto SBEFIFO_PRI_UNKNOWN_ERROR = 0x00FE0000;
	const auto SBEFIFO_SEC_HW_TIMEOUT = 0x0010;

	if (status == (SBEFIFO_PRI_UNKNOWN_ERROR | SBEFIFO_SEC_HW_TIMEOUT)) {
		log(level::ERROR, "SBE chipop timeout reported(%s)",
		    pdbg_target_path(proc));
		// only for primary processor SBE run p10_extract_sbe_rc hwp and
		// perform recovery actions
		if (isPrimaryProc(proc)) {
			try {
				sbeErrorHandler(proc);
			} catch (const std::exception &ex) {
				// TODO: for now just return SBE_CMD_TIMEOUT
				// error, until openpower-proc-control and
				// phosphor-logging caters for new errors that
				// could be thrown from sbeErrorHandler
				log(level::ERROR, "Exception caught %s",
				    ex.what());
			}
		}
		// TODO: Need to call p10_extract_sbe_rc for secondary processor
		// chip-op failure but should not perform any recovery action.
		// Just need to log the details got from p10_extract_sbe_rc for
		// debugging purpose
		return sbeError_t(exception::SBE_CMD_TIMEOUT);
	}

	// Handle empty buffer.
	if (!ffdcLen) {
		// log message and return.
		log(level::ERROR, "Empty SBE FFDC returned (%s)",
		    pdbg_target_path(proc));
		return sbeError_t(exception::SBE_FFDC_NO_DATA);
	}

	// create ffdc file
	tmpfile_t ffdcFile(bufPtr.getData(), ffdcLen);
	return sbeError_t(exception::SBE_CMD_FAILED, ffdcFile.getFd(),
			  ffdcFile.getPath().c_str());
}

void mpiplContinue(struct pdbg_target *proc)
{
	log(level::INFO, "Enter: mpiplContinue(%s)", pdbg_target_path(proc));

	// validate SBE state
	validateSBEState(proc);

	// get PIB target
	struct pdbg_target *pib = getPibTarget(proc);

	// call pdbg back-end function
	auto ret = sbe_mpipl_continue(pib);
	if (ret != 0) {
		throw captureFFDC(proc);
	}
}

void mpiplEnter(struct pdbg_target *proc)
{
	log(level::INFO, "Enter: mpiplEnter(%s)", pdbg_target_path(proc));

	// validate SBE state
	validateSBEState(proc);

	// get PIB target
	struct pdbg_target *pib = getPibTarget(proc);

	// call pdbg back-end function
	auto ret = sbe_mpipl_enter(pib);
	if (ret != 0) {
		throw captureFFDC(proc);
	}
}

void getTiInfo(struct pdbg_target *proc, uint8_t **data, uint32_t *dataLen)
{
	log(level::INFO, "Enter: getTiInfo(%s)", pdbg_target_path(proc));

	// Validate input target is processor target.
	validateProcTgt(proc);

	if (!isTgtPresent(proc)) {
		log(level::ERROR, "getTiInfo(%s) Target is not present",
		    pdbg_target_path(proc));
	}
	// SBE halt state need recovery before dump chip-ops
	sbeHaltStateRecovery(proc);

	// validate SBE state
	validateSBEState(proc);

	// get PIB target
	struct pdbg_target *pib = getPibTarget(proc);

	// call pdbg back-end function
	auto ret = sbe_mpipl_get_ti_info(pib, data, dataLen);
	if (ret != 0) {
		throw captureFFDC(proc);
	}
}

void getDump(struct pdbg_target *proc, const uint8_t type, const uint8_t clock,
	     const uint8_t faCollect, uint8_t **data, uint32_t *dataLen)
{
	log(level::INFO, "Enter: getDump(%d) on %s", type,
	    pdbg_target_path(proc));

	// Validate input target is processor target.
	validateProcTgt(proc);

	if (!isTgtPresent(proc)) {
		log(level::ERROR, "getDump(%s) Target is not present",
		    pdbg_target_path(proc));
	}
	// SBE halt state need recovery before dump chip-ops
	sbeHaltStateRecovery(proc);

	// validate SBE state
	validateSBEState(proc);

	// get PIB target
	struct pdbg_target *pib = getPibTarget(proc);

	// call pdbg back-end function
	if (sbe_dump(pib, type, clock, faCollect, data, dataLen)) {
		throw captureFFDC(proc);
	}
}

void threadStopProc(struct pdbg_target *proc)
{
	validateProcTgt(proc);

	log(level::INFO, "Enter: threadStopProc(%s)", pdbg_target_path(proc));

	// SBE halt state need recovery before thread stop all chip-ops
	sbeHaltStateRecovery(proc);

	// validate SBE state
	validateSBEState(proc);

	// get PIB target
	struct pdbg_target *pib = getPibTarget(proc);

	// call pdbg back-end function
	auto ret = thread_stop_proc(pib);
	if (ret != 0) {
		throw captureFFDC(proc);
	}
}

/**
 * @brief Retry reipl from current failed side
 *
 * p10_extract_sbe_rc HWP is run to identify the recovery action
 * for the SBE seeprom failure.
 *
 * Restart and ReIPL from the same side unless the no
 * of retries is less than max retry count.
 *
 * If reached max retry count then reipl using backup boot seeprom
 *
 * @param[in] proc processor target to operate on
 * @return throws exception if not able to switch or for any
 *  attribute read/write error.
 */

void restartSBERCHandler(struct pdbg_target *proc)
{
	log(level::INFO, "SBERC:restartSBERCHandler on %s",
	    pdbg_target_path(proc));
}

/**
 * @brief ReIPL using backup boot seeprom
 *
 * Current sbe seeprom/sbe boot code used by the processor could
 * be faulty due to which chip-op could fail.
 *
 * p10_extract_sbe_rc HWP is run to identify the recovery action
 * for the SBE seeprom failure.
 *
 * This method is to cater to ReIPL using backup boot seeprom
 * recovery action specified by p10_extract_sbe_rc. This could
 * be because the current side seeprom is not recoverable.
 *
 * Check the currently used boot seeprom side used and mark it
 * as bad and then select the backup boot seeprom if the same is
 * not bad.
 *
 * If both primary and secondary seeprom's are bad throw exception
 * to the caller so that the processor can be deconfigured/guarded.
 *
 * @param[in] proc processor target to operate on
 * @return throws exception if not able to switch or for any
 *  attribute read/write error.
 */
void reiplUsingBKPBootSeeprom(struct pdbg_target *proc)
{
	log(level::INFO, "SBERC:reiplUsingBKPBootSeeprom on %s",
	    pdbg_target_path(proc));
}

/**
 * @brief ReIPL using backup measurement seeprom
 *
 * Current sbe seeprom/sbe measurement seeprom code used by the
 * the processor could be faulty due to which chip-op could fail.
 *
 * p10_extract_sbe_rc HWP is run to identify the recovery action
 * for the SBE seeprom failure.
 *
 * This method is to cater to ReIPL using backup meas seeprom
 * recovery action specified by p10_extract_sbe_rc. This could
 * be because the current side of the seeprom is not recoverable.
 *
 * Check the currently used measurement seeprom side used and mark
 * it as bad and then select the backup measurement seeprom if the
 * same is not bad.
 *
 * If both primary and secondary seeprom's are bad throw exception
 * to the caller so that the processor can be deconfigured/guarded.
 *
 * @param[in] proc processor target to operate on

 * @return throws exception if not able to switch or for any
 * 		attribute read/write error.
 */
void reiplUsingBKPMeasurementSeeprom(struct pdbg_target *proc)
{
	log(level::INFO, "SBERC:reiplUsingBKPMeasurementSeeprom on %s",
	    pdbg_target_path(proc));
}

/**
 * @brief Handle SBE error and perform recovery action
 *
 * For any SBE access/chip-op failure SBE FFDC is collected,
 * If SBE FFDC collection fails with a Timeout error, then
 * we assume SBE is in a bad state. Execute p10_extract_sbe_rc
 * HWP to determine the SBE failure reason and the recovery
 * action to be done by BMC for the failure.
 *
 * Routine should be called as part of SBE error handling
 *
 * This method puts the SBE state as FAILED in device tree
 *
 * @param[in] proc Non NULL proc Target handle.
 * @return Exception thrown on failure
 */
void sbeErrorHandler(struct pdbg_target *proc)
{
	try {
		assert(proc != nullptr);

		log(level::INFO, "SBERC: sbeErrorHandler on %s",
		    pdbg_target_path(proc));

		auto sbeState = getState(proc);

		if (sbeState == SBE_STATE_DEBUG_MODE ||
		    sbeState == SBE_STATE_FAILED) {
			// The SBE on this processor is not in a state
			// where chipOps can be issued. SBE Error handling
			// will be skipped.
			log(level::ERROR,
			    "SBERC: sbe for proc %s is not in DEBUG/FAILED "
			    "state %d",
			    pdbg_target_path(proc), sbeState);
			throw sbeError_t(exception::SBE_INVALID_STATE);
		}

		// set state to debug mode to extract sbe rc
		setState(proc, SBE_STATE_DEBUG_MODE);

		P10_EXTRACT_SBE_RC::RETURN_ACTION recovAction;
		auto fapiRc = p10_extract_sbe_rc(proc, recovAction, true);
		if (fapiRc != fapi2::FAPI2_RC_SUCCESS) {
			log(level::INFO,
			    "SBERC: p10_extract_sbe_rc returned error %d, skip "
			    "recovery",
			    fapiRc);
			return;
		}
		log(level::INFO,
		    "SBERC: p10_extract_sbe_rc for proc=%s returned rc=0x%08X "
		    "and "
		    "SBE Recovery Action=0x%08X",
		    pdbg_target_path(proc), fapiRc, recovAction);

		// marking sbe state failed after p10_extract_sbe_rc so
		// that no further recovery actions are executed in parallel
		setState(proc, SBE_STATE_FAILED);

		switch (recovAction) {
		case P10_EXTRACT_SBE_RC::RETURN_ACTION::RESTART_SBE:
		case P10_EXTRACT_SBE_RC::RETURN_ACTION::RESTART_CBS:
			log(level::INFO,
			    "SBERC: recovery action RESTART_SBE/RESTART_CBS "
			    "received "
			    "on Proc %s",
			    pdbg_target_path(proc));
			(void)restartSBERCHandler(proc);
			break;

		case P10_EXTRACT_SBE_RC::RETURN_ACTION::REIPL_BKP_SEEPROM:
		case P10_EXTRACT_SBE_RC::RETURN_ACTION::REIPL_UPD_SEEPROM:
			log(level::INFO,
			    "SBERC: recovery action "
			    "REIPL_BKP_SEEPROM/REIPL_UPD_SEEPROM "
			    "received on Proc %s",
			    recovAction, pdbg_target_path(proc));
			(void)reiplUsingBKPBootSeeprom(proc);
			break;

		case P10_EXTRACT_SBE_RC::RETURN_ACTION::REIPL_BKP_MSEEPROM:
		case P10_EXTRACT_SBE_RC::RETURN_ACTION::REIPL_UPD_MSEEPROM:
			log(level::INFO,
			    "SBERC: recovery action "
			    "REIPL_BKP_MSEEPROM/REIPL_UPD_MSEEPROM "
			    "received on Proc %s",
			    pdbg_target_path(proc));
			(void)reiplUsingBKPMeasurementSeeprom(proc);
			break;

		case P10_EXTRACT_SBE_RC::RETURN_ACTION::NO_RECOVERY_ACTION:
			log(level::INFO,
			    "SBERC: recovery action NO_RECOVERY_ACTION "
			    "received on "
			    "Proc %s",
			    pdbg_target_path(proc));
			(void)reiplUsingBKPBootSeeprom(proc);
			break;

		case P10_EXTRACT_SBE_RC::RETURN_ACTION::REIPL_BKP_BMSEEPROM:
			log(level::INFO,
			    "SBERC: recovery action REIPL_BKP_BMSEEPROM "
			    "received on "
			    "Proc %s",
			    pdbg_target_path(proc));
			(void)reiplUsingBKPBootSeeprom(proc);
			(void)reiplUsingBKPMeasurementSeeprom(proc);
			break;
		default:
			log(level::INFO,
			    "SBERC: recovery action default received on Proc "
			    "%s",
			    pdbg_target_path(proc));
			break;
		}
	} catch (const std::exception &ex) {
		log(level::INFO,
		    "SBERC: exception thrown in sbeErrorHandler error %s",
		    ex.what());
		throw ex;
	}
}
} // namespace sbe
} // namespace openpower::phal
