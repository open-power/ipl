extern "C" {
#include <stdio.h>
#include <unistd.h>
#include <libpdbg.h>
}

#include "libipl.H"
#include "libipl_internal.H"
#include "common.H"

#include <ekb/hwpf/fapi2/include/return_code_defs.H>
#include <ekb/chips/p10/procedures/hwp/istep/p10_do_fw_hb_istep.H>
#include <ekb/chips/p10/procedures/hwp/sbe/p10_get_sbe_msg_register.H>

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

	ipl_log(IPL_INFO, "Istep: SBE %d.%d : started\n", major, minor);

	pdbg_for_each_class_target("pib", pib) {
		if (pdbg_target_status(pib) != PDBG_TARGET_ENABLED)
			continue;

		// Run SBE isteps only on functional master processor
		proc = pdbg_target_require_parent("proc", pib);
		if (!ipl_is_master_proc(proc))
			continue;

		if (!ipl_is_functional(proc)) {
			ipl_log(IPL_ERROR, "Master processor (%d) is not functional\n",
				pdbg_target_index(proc));
			ipl_error_callback(false);
			return 1;
		}

		ipl_log(IPL_INFO, "Running sbe_istep on processor %d\n",
			pdbg_target_index(proc));

		rc = sbe_istep(pib, major, minor);
		if (rc)
			ipl_log(IPL_ERROR, "Istep %d.%d failed on chip %d, rc=%d\n",
				major, minor, pdbg_target_index(proc), rc);

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

	ipl_log(IPL_INFO, "Istep: Hostboot %d.%d : started\n", major, minor);

	pdbg_for_each_class_target("proc", proc) {
		fapi2::ReturnCode fapi_rc;

		// Run HWP only on functional master processor
		if (!ipl_is_master_proc(proc))
			continue;

		if (!ipl_is_functional(proc)) {
			ipl_log(IPL_ERROR, "Master processor(%d) is not functional\n",
				pdbg_target_index(proc));
			ipl_error_callback(false);
			return 1;
		}

		ipl_log(IPL_INFO, "Running p10_do_fw_hb_istep HWP on processor %d\n",
			pdbg_target_index(proc));

		fapi_rc = p10_do_fw_hb_istep(proc, major, minor,
					     retry_limit_ms, delay_ms);
		if (fapi_rc != fapi2::FAPI2_RC_SUCCESS)
			ipl_log(IPL_ERROR, "Istep %d.%d failed on chip %d, rc=%d\n",
				major, minor, pdbg_target_index(proc), fapi_rc);
		else
			rc = 0;

		ipl_error_callback(rc == 0);
		break;
	}

	return rc;
}

bool ipl_sbe_booted(struct pdbg_target *proc, uint32_t wait_time_seconds)
{
	sbeMsgReg_t sbeReg;
	fapi2::ReturnCode fapi_rc;
	uint32_t loopcount;

	loopcount = wait_time_seconds > 0 ? wait_time_seconds : 5;

	while (loopcount > 0) {
	  	fapi_rc = p10_get_sbe_msg_register(proc, sbeReg);
		if (fapi_rc == fapi2::FAPI2_RC_SUCCESS) {
			if (sbeReg.sbeBooted) {
				ipl_log(IPL_INFO, "SBE booted. sbeReg[0x%08x]\n",
					uint32_t(sbeReg.reg));
				return true;
			} else {
				ipl_log(IPL_DEBUG, "SBE boot is in progress. sbeReg[0x%08x]\n",
					uint32_t(sbeReg.reg));
			}
		} else {
			ipl_log(IPL_ERROR,
				"p10_get_sbe_msg_register failed for proc %d, rc=%d\n",
				pdbg_target_index(proc), fapi_rc);
		}

		loopcount--;
		sleep(1);
	}

	return false;
}

bool ipl_is_functional(struct pdbg_target *target)
{
	uint8_t buf[5];

	if (!pdbg_target_get_attribute_packed(target, "ATTR_HWAS_STATE", "41", 1, buf)) {
		ipl_log(IPL_INFO, "Attribute [ATTR_HWAS_STATE] read failed\n");

		//Checking pdbg functional state
		if (pdbg_target_status(target) == PDBG_TARGET_ENABLED)
			return true;

		return false;
	}

	//isFuntional bit is stored in 4th byte and bit 3 position in HWAS_STATE
	return (buf[4] & 0x20);
}

bool ipl_check_functional_master(void)
{
	struct pdbg_target *proc;

	pdbg_for_each_class_target("proc", proc) {
		if (!ipl_is_master_proc(proc))
			continue;

		if (!ipl_is_functional(proc)) {
			ipl_log(IPL_ERROR, "Master processor(%d) is not functional\n",
				pdbg_target_index(proc));
			return false;
		}
	}

	return true;
}
