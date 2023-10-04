#include "libphal.H"
#include "log.H"
#include "phal_exception.H"
#include "utils_buffer.H"
#include "utils_pdbg.H"
#include "utils_tempfile.H"

#include <ekb/chips/p10/procedures/hwp/perv/p10_sbe_hreset.H>
#include <ekb/chips/p10/procedures/hwp/sbe/p10_get_sbe_msg_register.H>
#include <ekb/chips/p10/procedures/hwp/perv/p10_extract_sbe_rc.H>
#include <ekb/hwpf/fapi2/include/return_code_defs.H>
#include <unistd.h>

namespace openpower::phal
{
namespace sbe
{

using namespace openpower::phal::logging;
using namespace openpower::phal;
using namespace openpower::phal::utils::pdbg;
using namespace openpower::phal::pdbg;

static sbeError_t switchPdbgBackend(std::string_view backendString,
				    struct pdbg_target *&proc)
{
	log(level::INFO,
	    "Enter switchPdbgBackend with targeted backend as (%s)",
	    backendString);
	if (backendString.empty()) {
		log(level::ERROR, "Backend type is not provided for switching");
		return exception::PDBG_INIT_FAIL;
	}

	log(level::INFO, "Switching PDBG backend to (%s)", backendString);
	enum pdbg_backend backend;
	if (("SBEFIFO" == backendString) || ("sbefifo" == backendString))
		backend = pdbg_backend::PDBG_BACKEND_SBEFIFO;
	else if (("KERNEL" == backendString) || ("kernel" == backendString))
		backend = pdbg_backend::PDBG_BACKEND_KERNEL;
	else if (("FSI" == backendString) || ("fsi" == backendString))
		backend = pdbg_backend::PDBG_BACKEND_FSI;
	else if (("I2C" == backendString) || ("i2c" == backendString))
		backend = pdbg_backend::PDBG_BACKEND_I2C;
	else
		backend = pdbg_backend::PDBG_DEFAULT_BACKEND;

	// First clear the existing dev tree
	if (pdbg_target_root())
		pdbg_release_dt_root();

	// Then set the backend to the targeted one
	if (!pdbg_set_backend(backend, nullptr)) {
		log(level::ERROR, "Backend type can not be set to %s",
		    backendString);
		return exception::PDBG_INIT_FAIL;
	}
	constexpr auto devtree =
	    "/var/lib/phosphor-software-manager/pnor/rw/DEVTREE";
	// PDBG_DTB environment variable set to CEC device tree path
	if (setenv("PDBG_DTB", devtree, 1)) {
		log(level::ERROR, "Failed to set PDBG_DTB. ErrNo: %s",
		    strerror(errno));
		return exception::PDBG_INIT_FAIL;
	}
	constexpr auto PDATA_INFODB_PATH =
	    "/usr/share/pdata/attributes_info.db";
	// PDATA_INFODB environment variable set to attributes tool infodb path
	if (setenv("PDATA_INFODB", PDATA_INFODB_PATH, 1)) {
		log(level::ERROR, "Failed to set PDATA_INFODB_PATH. ErrNo: %s",
		    strerror(errno));
		return exception::PDBG_INIT_FAIL;
	}
	// initialize the targeting system
	if (!pdbg_targets_init(nullptr)) {
		log(level::ERROR, "pdbg_targets_init failed for backend %s",
		    backendString);
		return exception::PDBG_INIT_FAIL;
	}
	// Get the new root node of the new device tree
	auto root = pdbg_target_root();
	if (!root) {
		log(level::ERROR,
		    "root target can not be aquired after switching backend %s",
		    backendString);
		return exception::PDBG_INIT_FAIL;
	}
	// Probe all the new targets before returning
	pdbg_target_probe_all(root);

	// Get the primary proc from the new device tree
	proc = getPrimaryProc();
	if (!getPibTarget(proc)) {
		log(level::ERROR,
		    "Probing PIB target failed after swicthing PDBG backend to "
		    "%s",
		    backendString);
		return exception::PDBG_TARGET_NOT_OPERATIONAL;
	}
	if (!getFsiTarget(proc)) {
		log(level::ERROR,
		    "Probing FSI target failed after swicthing PDBG backend to "
		    "%s",
		    backendString);
		return exception::PDBG_TARGET_NOT_OPERATIONAL;
	}
	return exception::NONE;
}

std::tuple<fapi2::ReturnCode, std::string>
    getSbeExtractRc(struct pdbg_target *proc)
{
	log(level::INFO, "Enter getSbeExtractRc for proc target (%s)",
	    pdbg_target_path(proc));

	// Execute SBE extract rc to get the reason code for SEEPROM/SBE boot
	// failure
	P10_EXTRACT_SBE_RC::RETURN_ACTION recovAction;
	// p10_extract_sbe_rc is returning the error along with
	// recovery action.
	auto fapiRc = p10_extract_sbe_rc(proc, recovAction, true);
	std::string recovActionString;

	switch (recovAction) {
	case P10_EXTRACT_SBE_RC::RETURN_ACTION::ERROR_RECOVERED: {
		recovActionString = "ERROR_RECOVERED";
		break;
	}
	case P10_EXTRACT_SBE_RC::RETURN_ACTION::NO_RECOVERY_ACTION: {
		recovActionString = "NO_RECOVERY_ACTION";
		break;
	}
	case P10_EXTRACT_SBE_RC::RETURN_ACTION::RECONFIG_WITH_CLOCK_GARD: {
		recovActionString = "RECONFIG_WITH_CLOCK_GARD";
		break;
	}
	case P10_EXTRACT_SBE_RC::RETURN_ACTION::REIPL_BKP_BMSEEPROM: {
		recovActionString = "REIPL_BKP_BMSEEPROM";
		break;
	}
	case P10_EXTRACT_SBE_RC::RETURN_ACTION::REIPL_BKP_MSEEPROM: {
		recovActionString = "REIPL_BKP_MSEEPROM";
		break;
	}
	case P10_EXTRACT_SBE_RC::RETURN_ACTION::REIPL_UPD_MSEEPROM: {
		recovActionString = "REIPL_UPD_MSEEPROM";
		break;
	}
	case P10_EXTRACT_SBE_RC::RETURN_ACTION::REIPL_UPD_SPI_CLK_DIV: {
		recovActionString = "REIPL_UPD_SPI_CLK_DIV";
		break;
	}
	case P10_EXTRACT_SBE_RC::RETURN_ACTION::RESTART_CBS: {
		recovActionString = "RESTART_CBS";
		break;
	}
	case P10_EXTRACT_SBE_RC::RETURN_ACTION::RESTART_SBE: {
		recovActionString = "RESTART_SBE";
		break;
	}
	default: {
		recovActionString = "UNKNOWN_RECOV_ACTION";
		break;
	}
	}
	return std::make_tuple(fapiRc, recovActionString);
}

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
		/**
		 * Okay, so the SBE is dead with the current backend SBEFIFO.
		 * Let's switch it to KERNEL and try to run the SBE_EXTRACT_RC
		 * to see if we can get some FFDC data along with the SBE dead's
		 * reason
		 */
		auto switchBackend = [&](std::string_view backend) {
			auto sbeErr_t = switchPdbgBackend(backend, proc);
			if (sbeErr_t.errType() != exception::NONE)
				log(level::ERROR,
				    "Switching the backend to '%s' failed",
				    backend.data());
			return sbeErr_t;
		};

		if (switchBackend("KERNEL").errType() == exception::NONE) {
			log(level::INFO, "Calling Extract SBE RC for details");
			const auto &[fapiSBE_RC, recovAction] =
			    getSbeExtractRc(proc);
			if (fapiSBE_RC == fapi2::FAPI2_RC_SUCCESS) {
				log(level::INFO,
				    "Recovery action from getSbeExtractRc "
				    "returned as: %s",
				    recovAction);
				// Log the SBE debug data and then return
				logSbeDebugData(proc);
				// But before that switch the PDBG backend back
				// to SBEFIFO but don't throw any error here if
				// it fails as during the next reboot which will
				// be happening eventually after this method
				// returns the backend will be automatically
				// switched to SBEFIFO
				switchBackend("SBEFIFO");
				throw sbeError_t(exception::SBE_EXTRACT_RC);
			} else {
				log(level::ERROR, "getSbeExtractRc HWP failed");
				// Try to switch back the PDBG backend to
				// SBEFIFO before we throw the error
				switchBackend("SBEFIFO");
				throw sbeError_t(
				    exception::SBE_STATE_READ_FAIL);
			}
		} else {
			// Try to switch back the PDBG backend to SBEFIFO before
			// we throw the error
			switchBackend("SBEFIFO");
			throw sbeError_t(exception::SBE_STATE_READ_FAIL);
		}
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

sbeError_t captureFFDC(struct pdbg_target *&proc)
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
	constexpr auto SBEFIFO_PRI_UNKNOWN_ERROR = 0x00FE0000;
	constexpr auto SBEFIFO_SEC_HW_TIMEOUT = 0x0010;

	if (status == (SBEFIFO_PRI_UNKNOWN_ERROR | SBEFIFO_SEC_HW_TIMEOUT)) {
		log(level::ERROR, "SBE chipop timeout reported(%s)",
		    pdbg_target_path(proc));
		/**
		 * Okay, so the SBE is dead with the current backend SBEFIFO.
		 * Let's switch it to KERNEL and try to run the SBE_EXTRACT_RC
		 * to see if we can get some FFDC data along with the SBE dead's
		 * reason
		 */
		auto switchBackend = [&](std::string_view backend) {
			auto sbeErr_t = switchPdbgBackend(backend, proc);
			if (sbeErr_t.errType() != exception::NONE)
				log(level::ERROR,
				    "Switching the backend to '%s' failed",
				    backend.data());
			return sbeErr_t;
		};

		if (switchBackend("KERNEL").errType() != exception::NONE) {
			// Try to switch the PDBG backend back to SBEFIFO before
			// returning but don't throw any error if it fails
			switchBackend("SBEFIFO");
			return sbeError_t(exception::SBE_CMD_TIMEOUT);
		}
		log(level::INFO, "Calling Extract SBE RC for details");
		const auto &[fapiRC, recovAction] = getSbeExtractRc(proc);
		if (fapiRC != fapi2::FAPI2_RC_SUCCESS) {
			log(level::INFO, "Calling getSbeExtractRc failed");
			// Try to switch the PDBG backend back to SBEFIFO before
			// returning but don't throw any error if it fails
			switchBackend("SBEFIFO");
			return sbeError_t(exception::SBE_CMD_TIMEOUT);
		} else {
			log(level::INFO,
			    "Recovery action returned from getSbeExtractRc as: "
			    "%s",
			    recovAction);
			// Try to get the SBE FFDC with the new backend
			if (sbe_ffdc_get(pib, &status, bufPtr.getPtr(),
					 &ffdcLen)) {
				log(level::ERROR,
				    "sbe_ffdc_get function failed with new "
				    "backend too");
				throw sbeError_t(
				    exception::SBE_FFDC_GET_FAILED);
			} else {
				// Handle empty buffer.
				if (!ffdcLen) {
					// log message and return.
					log(level::ERROR,
					    "Empty SBE FFDC returned (%s)",
					    pdbg_target_path(proc));
					return sbeError_t(
					    exception::SBE_FFDC_NO_DATA);
				}
				// Try to switch the PDBG backend back to
				// SBEFIFO before returning but don't throw any
				// error if it fails
				switchBackend("SBEFIFO");
				// create ffdc file
				tmpfile_t ffdcFile(bufPtr.getData(), ffdcLen);
				return sbeError_t(exception::SBE_EXTRACT_RC,
						  ffdcFile.getFd(),
						  ffdcFile.getPath().c_str());
			}
		}
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

} // namespace sbe
} // namespace openpower::phal
