#include "exception.H"
#include "libphal.H"
#include "log.H"
#include "temporary_file.H"
#include "utils.H"

extern "C" {
#include <libpdbg_sbe.h>
}

namespace libphal
{
namespace sbe
{

void validateSBEState(struct pdbg_target *pib)
{
	// In order to allow SBE operation following conditions should be met
	// 1. Caller has to make sure pib belong to functional/dump functional
	// processor
	// 2. Processor's SBE state should be marked as BOOTED or CHECK_CFAM
	// 3. In case the SBE state is marked as CHECK_CFAM then SBE's
	// PERV_SB_MSG_FSI register must be checked to see if SBE is booted or
	// NOT.
	// check if SBE is in HALT state and perform HRESET if in HALT state
	// halt_state_check

	// Get the current SBE state
	enum sbe_state state;

	if (sbe_get_state(pib, &state)) {
		phalLog(PHAL_ERROR,
			"Failed to read SBE state information (%s) \n",
			pdbg_target_path(pib));
		throw sbeError_t(exception::SBE_STATE_READ_FAIL);
	}

	// SBE_STATE_CHECK_CFAM case is already handled by pdbg api
	if (state == SBE_STATE_BOOTED)
		return;

	// TODO , check halt state and routine for recover SBE.

	if (state != SBE_STATE_BOOTED) {
		phalLog(PHAL_INFO,
			"SBE (%s) is not ready for chip-op: state(0x%08x) \n",
			pdbg_target_path(pib), state);
		throw sbeError_t(exception::SBE_CHIPOP_NOT_ALLOWED);
	}
}

void captureFFDC(struct pdbg_target *pib)
{
	// get SBE FFDC info
	bufPtr_t bufPtr;
	uint32_t ffdcLen = 0;
	uint32_t status = 0;
	if (sbe_ffdc_get(pib, &status, bufPtr.getPtr(), &ffdcLen)) {
		phalLog(PHAL_ERROR, "sbe_ffdc_get function failed \n");
		throw sbeError_t(exception::SBE_FFDC_GET_FAILED);
	}
	// TODO Need to remove this once pdbg header file support in place
	const auto SBEFIFO_PRI_UNKNOWN_ERROR = 0x00FE0000;
	const auto SBEFIFO_SEC_HW_TIMEOUT = 0x0010;

	if (status == (SBEFIFO_PRI_UNKNOWN_ERROR | SBEFIFO_SEC_HW_TIMEOUT)) {
		phalLog(PHAL_INFO, "SBE chipop timeout reported(%s) \n",
			pdbg_target_path(pib));
		throw sbeError_t(exception::SBE_CHIPOP_TIMEOUT);
	}

	// Handle empty buffer.
	if (!ffdcLen) {
		// log message and continue with empty file to support
		// generic flow.
		phalLog(PHAL_ERROR, "Empty FFDC returned (%s) \n",
			pdbg_target_path(pib));
	}

	// create ffdc file
	tmpfile_t ffdcFile(bufPtr.getData(), ffdcLen);
	throw sbeError_t(exception::SBE_CHIPOP_FAILED, ffdcFile.getFd(),
			 ffdcFile.getPath().c_str());
}

void mpiplContinue(struct pdbg_target *pib)
{
	// validate SBE state
	validateSBEState(pib);

	// call pdbg back-end function
	auto ret = sbe_mpipl_continue(pib);
	if (ret != 0) {
		captureFFDC(pib);
	}
}

void getTiInfo(struct pdbg_target *pib, uint8_t **data, uint32_t *dataLen)
{
	// validate SBE state
	validateSBEState(pib);

	// call pdbg back-end function
	auto ret = sbe_mpipl_get_ti_info(pib, data, dataLen);
	if (ret != 0) {
		captureFFDC(pib);
	}
}

} // namespace sbe
} // namespace libphal
