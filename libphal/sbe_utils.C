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

} // namespace sbe
} // namespace libphal
