#include <fapi2.H>
#include "libphal.H"
#include "log.H"
#include "phal_exception.H"
#include "utils_buffer.H"
#include "utils_pdbg.H"
#include "utils_tempfile.H"
//#include "plat/plat_target.H"
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

#include "sbeInf/response_info.H"
#include "sbeInf/sbe_cmd_implementation.H"

#include <ekb/chips/p10/procedures/hwp/perv/p10_sbe_hreset.H>
#include <ekb/chips/p10/procedures/hwp/sbe/p10_get_sbe_msg_register.H>
#include <ekb/hwpf/fapi2/include/return_code_defs.H>
#include <unistd.h>
#include <unordered_map>

extern "C" {
#include <libsbefifo.h>
}
namespace openpower::phal
{
namespace sbe
{

using namespace openpower::phal::logging;
using namespace openpower::phal;
using namespace openpower::phal::utils::pdbg;
using namespace openpower::phal::pdbg;

constexpr uint16_t minFFDCPackageWordSizePOZ = 5;
constexpr uint16_t pozFfdcMagicCode = 0xFBAD;
constexpr uint16_t ffdcMagicCode = 0xFFDC;
constexpr uint16_t slidOffset = 2 * sizeof(uint32_t);
constexpr bool CO_CMD_SUCCESS = true;
constexpr bool CO_CMD_FAILURE = false;

struct pozFfdcHeader {
	uint32_t magicByte : 16;
	uint32_t lengthinWords : 16;
	uint32_t seqId : 16;
	uint32_t cmdClass : 8;
	uint32_t cmd : 8;
	uint32_t slid : 16;
	uint32_t severity : 8;
	uint32_t chipId : 8;
	uint32_t fapiRc;
} __attribute__((packed));
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
void sbeHaltStateRecovery(struct pdbg_target *proc,
			  bool ignorePrimaryProc = true)
{
	// Skip for primary processor.
	if (ignorePrimaryProc && isPrimaryProc(proc)) {
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

void validateSBEState(struct pdbg_target *chip)
{

	// In order to allow SBE operation following conditions should be met
	// 1. Caller has to make sure processor target should be functional
	// or dump functional
	// 2. Processor's SBE state should be marked as BOOTED or CHECK_CFAM
	// 3. In case the SBE state is marked as CHECK_CFAM then SBE's
	// PERV_SB_MSG_FSI register must be checked to see if SBE is booted or
	// NOT.
	enum sbe_state state = getState(chip);

	// SBE_STATE_CHECK_CFAM case is already handled by pdbg api
	if (state == SBE_STATE_BOOTED) {
		return;
	} else {
		log(level::INFO,
		    "SBE (%s) is not ready for chip-op: state(0x%08x)",
		    pdbg_target_path(chip), state);
		throw sbeError_t(exception::SBE_CHIPOP_NOT_ALLOWED);
	}
}

void setState(struct pdbg_target *chip, enum sbe_state state)
{
	bool isOcmb = is_ody_ocmb_chip(chip);
	struct pdbg_target *targetToSet =
	    isOcmb ? get_ody_fsi_target(chip) : getPibTarget(chip);

	int rc = isOcmb ? sbe_ody_set_state(targetToSet, state)
			: sbe_set_state(targetToSet, state);

	if (rc) {
		log(level::ERROR,
		    "Failed to set SBE state(%d) information (%s)", state,
		    pdbg_target_path(chip));
		throw sbeError_t(exception::SBE_STATE_WRITE_FAIL);
	}
}

enum sbe_state getState(struct pdbg_target *chip)
{
	bool isOcmb = is_ody_ocmb_chip(chip);
	struct pdbg_target *targetToCheck =
	    isOcmb ? get_ody_fsi_target(chip) : getPibTarget(chip);

	enum sbe_state state = SBE_STATE_NOT_USABLE; // default value

	int rc = isOcmb ? sbe_ody_get_state(targetToCheck, &state)
			: sbe_get_state(targetToCheck, &state);

	if (rc) {
		log(level::ERROR, "Failed to get SBE state information (%s)",
		    pdbg_target_path(chip));

		// Unlike PROC, OCMB doesnt have standby power
		// so state read will fail until that is
		// initialized so for OCMB, return a read fail
		// as not usable
		if (!isOcmb) {
			throw sbeError_t(exception::SBE_STATE_READ_FAIL);
		}
		state = SBE_STATE_NOT_USABLE;
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

sbeError_t processPOZFFDC(const uint8_t *bufPtr, uint32_t ffdcLen,
			  bool coSuccess)
{
	uint32_t offset = 0;
	FFDCFileList ffdcFileList;

	while (offset < ffdcLen) {
		const auto *ffdcHeader =
		    reinterpret_cast<const pozFfdcHeader *>(bufPtr + offset);
		if (ntohs(ffdcHeader->magicByte) != pozFfdcMagicCode) {
			log(level::ERROR,
			    "UNKNOWN magic word: (%X) in the FFDC packet",
			    ffdcHeader->magicByte);
			throw std::runtime_error("Invalid FFDC magic byte");
		}

		uint32_t lenInBytes =
		    ntohs(ffdcHeader->lengthinWords) * sizeof(uint32_t);
		uint16_t slid = ntohs(ffdcHeader->slid);
		uint8_t severity = ffdcHeader->severity;

		auto iter = ffdcFileList.find(slid);
		if (iter == ffdcFileList.end()) {
			// New SLID, create a new TemporaryFile
			tmpfile_t ffdcFile(
			    const_cast<uint8_t *>(bufPtr + offset), lenInBytes);
			ffdcFileList[slid] = std::make_tuple(
			    severity, ffdcFile.getFd(), ffdcFile.getPath());
		} else {
			// Existing SLID, update severity if higher, append data
			// to existing file
			auto &[existingSeverity, existingFd, existingPath] =
			    iter->second;
			if (severity > existingSeverity) {
				existingSeverity =
				    severity; // Update severity if higher
			}
			// Append new data to the existing file
			int fd =
			    open(existingPath.c_str(), O_WRONLY | O_APPEND);
			if (fd == -1) {
				throw std::runtime_error(
				    "Failed to open existing FFDC file for "
				    "appending");
			}
			if (write(fd, bufPtr + offset, lenInBytes) !=
			    static_cast<ssize_t>(lenInBytes)) {
				close(fd);
				throw std::runtime_error(
				    "Failed to append data to FFDC file");
			}
			close(fd);
		}

		offset += lenInBytes;
	}

	if (coSuccess) {
		return sbeError_t(exception::SBE_INTERNAL_FFDC_DATA,
				  ffdcFileList);
	}
	return sbeError_t(exception::SBE_CMD_FAILED, ffdcFileList);
}

sbeError_t captureFFDC(struct pdbg_target *target, bool coSuccess)
{
	bufPtr_t bufPtr;
	uint32_t ffdcLen = 0;
	uint32_t status = 0;

	bool isOcmb = is_ody_ocmb_chip(target);

	struct pdbg_target *opTarget = target;
	if (!isOcmb) {
		opTarget = getPibTarget(target);
		logSbeDebugData(target);

	} else {
		if (pdbg_target_probe(get_ody_chipop_target(opTarget)) !=
		    PDBG_TARGET_ENABLED) {
			log(level::ERROR,
			    "Failed to capture FFDC, Target not enabled (%s)",
			    pdbg_target_path(opTarget));
			throw sbeError_t(exception::PDBG_TARGET_INVALID);
		}
	}

	if (sbe_ffdc_get(opTarget, &status, bufPtr.getPtr(), &ffdcLen)) {
		log(level::ERROR, "Failed to get SBE FFDC data (%s)",
		    pdbg_target_path(opTarget));
		throw sbeError_t(exception::SBE_FFDC_GET_FAILED);
	}

	if (status == (SBEFIFO_PRI_UNKNOWN_ERROR | SBEFIFO_SEC_HW_TIMEOUT)) {
		return sbeError_t(exception::SBE_CMD_TIMEOUT);
	}

	if (!ffdcLen) {
		return sbeError_t(exception::SBE_FFDC_NO_DATA);
	}

	uint16_t magicByte =
	    ntohs(*reinterpret_cast<uint16_t *>(bufPtr.getData()));
	if (magicByte == ffdcMagicCode) {
		// Process as PROC FFDC
		tmpfile_t ffdcFile(bufPtr.getData(), ffdcLen);
		return coSuccess
			   ? sbeError_t(exception::SBE_INTERNAL_FFDC_DATA,
					ffdcFile.getFd(), ffdcFile.getPath())
			   : sbeError_t(exception::SBE_CMD_FAILED,
					ffdcFile.getFd(), ffdcFile.getPath());
	} else if (magicByte == pozFfdcMagicCode) {
		// Process as OCMB FFDC (POZ)
		return processPOZFFDC(bufPtr.getData(), ffdcLen, coSuccess);
	} else {
		log(level::ERROR, "UnSupported FFDC format, magic word: (%X)",
		    magicByte);
		throw std::runtime_error("Unsupported FFDC format");
	}
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
	sbeHaltStateRecovery(proc, false);

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

void getDump(struct pdbg_target* chip,
             const uint8_t type, const uint8_t clock, const uint8_t faCollect,
             uint8_t** data, uint32_t* dataLen)
{
    std::vector<uint8_t> dataVec;
    std::vector<sbeIntf::ResponseInfo::FFDC> ffdcVec;

    log(level::ERROR, "In get dump");

    fapi2::Target<fapi2::TARGET_TYPE_PROC_CHIP> fapiTarget(chip);
    auto rc = sbeIntf::getDump(fapiTarget, type, clock, faCollect, dataVec, ffdcVec);

    if (rc != fapi2::FAPI2_RC_SUCCESS)
    {
	log(level::ERROR, "fapi rc not success");
        *data = nullptr;
        *dataLen = 0;
        throw sbeError_t(exception::SBE_CMD_FAILED);
    }

    *dataLen = dataVec.size();
    *data = static_cast<uint8_t*>(malloc(*dataLen));
    if (!*data)
    {
        *dataLen = 0;
        throw sbeError_t(exception::SBE_CMD_FAILED);
    }

    std::memcpy(*data, dataVec.data(), *dataLen);

    // need to handle FFDC
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
