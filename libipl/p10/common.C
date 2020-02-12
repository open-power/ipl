extern "C" {
#include <stdio.h>

#include <libpdbg.h>
}

#include "libipl.H"
#include "libipl_internal.H"
#include "common.H"

#include <hwpf/fapi2/include/return_code_defs.H>
#include <ekb/chips/p10/procedures/hwp/istep/p10_do_fw_hb_istep.H>

int ipl_istep_via_sbe(int major, int minor)
{
	struct pdbg_target *pib;
	int rc = 0;

	pdbg_for_each_class_target("pib", pib) {
		if (ipl_mode() == IPL_DEFAULT && pdbg_target_index(pib) != 0)
			continue;

		rc |= sbe_istep(pib, major, minor);
		if (rc)
			ipl_log("Istep %d.%d failed on chip %d, rc=%d\n",
				major, minor, pdbg_target_index(pib), rc);
	}

	return rc;
}

int ipl_istep_via_hostboot(int major, int minor)
{
	fapi2::ReturnCode fapi_rc;
	struct pdbg_target *pib;
        uint64_t retry_limit_ms = 30 * 60 * 1000;
        uint64_t delay_ms = 100;
	int rc = 0;

	pdbg_for_each_class_target("pib", pib) {
		if (ipl_mode() == IPL_DEFAULT && pdbg_target_index(pib) != 0)
			continue;

		fapi_rc = p10_do_fw_hb_istep(pib, major, minor,
					     retry_limit_ms, delay_ms);
		if (fapi_rc != fapi2::FAPI2_RC_SUCCESS) {
			ipl_log("Istep %d.%d failed on chip %d, rc=%d\n",
				major, minor, pdbg_target_index(pib), rc);
			rc = rc + 1;
		}
	}

	return rc;
}
