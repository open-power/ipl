extern "C" {
#include <libpdbg.h>
}

#include "libipl.H"
#include "libipl_internal.H"
#include "common.H"

#include <ekb/chips/p10/procedures/hwp/perv/p10_pre_poweroff.H>

int ipl_pre_poweroff(void)
{
	struct pdbg_target *proc;
	int rc = 0;

	ipl_log(IPL_INFO, "pre-poweroff: Started\n");

	ipl_pre();

	pdbg_for_each_class_target("proc", proc)
	{
		fapi2::ReturnCode fapi_rc;

		if (!ipl_is_present(proc))
			continue;

		ipl_log(IPL_INFO,
			"Running p10_pre_poweroff HWP on processor %d\n",
			pdbg_target_index(proc));

		fapi_rc = p10_pre_poweroff(proc);
		if (fapi_rc != fapi2::FAPI2_RC_SUCCESS) {
			ipl_log(IPL_ERROR,
				"p10_pre_poweroff failed for proc index %d\n",
				pdbg_target_index(proc));
			ipl_process_fapi_error(fapi_rc, proc, false);
			// Count num of failed proc to do pre-poweroff
			++rc;
		}
	}
	// Update SBE state to Not usable in pre-poweroff path
	// Boot error callback is only required for failure handling
	ipl_set_sbe_state_all(SBE_STATE_NOT_USABLE);

	return rc;
}
