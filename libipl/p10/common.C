extern "C" {
#include <stdio.h>

#include <libpdbg.h>
}

#include "libipl.H"
#include "libipl_internal.H"
#include "common.H"

#include <ekb/hwpf/fapi2/include/return_code_defs.H>
#include <ekb/chips/p10/procedures/hwp/istep/p10_do_fw_hb_istep.H>

bool ipl_is_master_proc(struct pdbg_target *proc)
{
	uint8_t type;

	if (!pdbg_target_get_attribute(proc, "ATTR_PROC_MASTER_TYPE", 1, 1, &type)) {
		ipl_log(IPL_ERROR, "Attribute [ATTR_PROC_MASTER_TYPE] read failed \n");

		if (pdbg_target_index(proc) == 0)
			return true;

		return false;
	}

	/* Attribute value 0 corresponds to master processor */
	if (type == 0)
		return true;

	return false;
}

int ipl_istep_via_sbe(int major, int minor)
{
	struct pdbg_target *pib, *proc;
	int rc = 1;

	pdbg_for_each_class_target("pib", pib) {
		if (pdbg_target_status(pib) != PDBG_TARGET_ENABLED)
			continue;

		// Run SBE isteps only on master processor
		proc = pdbg_target_require_parent("proc", pib);
		if (!ipl_is_master_proc(proc))
			continue;

		rc = sbe_istep(pib, major, minor);
		if (rc)
			ipl_log(IPL_ERROR, "Istep %d.%d failed on chip %d, rc=%d\n",
				major, minor, pdbg_target_index(pib), rc);

		ipl_error_callback(rc == 0);
		break;
	}

	return rc;
}

int ipl_istep_via_hostboot(int major, int minor)
{
	struct pdbg_target *proc;
        uint64_t retry_limit_ms = 30 * 60 * 1000;
        uint64_t delay_ms = 100;
	int rc = 1;

	pdbg_for_each_class_target("proc", proc) {
		fapi2::ReturnCode fapi_rc;

		if (pdbg_target_status(proc) != PDBG_TARGET_ENABLED)
			continue;

		// Run HWP only on master processor
		if (!ipl_is_master_proc(proc))
			continue;

		fapi_rc = p10_do_fw_hb_istep(proc, major, minor,
					     retry_limit_ms, delay_ms);
		if (fapi_rc != fapi2::FAPI2_RC_SUCCESS)
			ipl_log(IPL_ERROR, "Istep %d.%d failed on chip %d, rc=%d\n",
				major, minor, pdbg_target_index(proc), fapi_rc);
		else
			rc = 0;

		ipl_error_callback(fapi_rc == fapi2::FAPI2_RC_SUCCESS);
		break;
	}

	return rc;
}
