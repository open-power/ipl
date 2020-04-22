extern "C" {
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include <libpdbg.h>
}

#include "libipl.H"
#include "libipl_internal.H"

#include <ekb/chips/p10/procedures/hwp/perv/p10_start_cbs.H>
#include <ekb/chips/p10/procedures/hwp/perv/p10_setup_ref_clock.H>
#include <ekb/chips/p10/procedures/hwp/perv/p10_clock_test.H>
#include <ekb/chips/p10/procedures/hwp/perv/p10_setup_sbe_config.H>
#include <ekb/chips/p10/procedures/hwp/perv/p10_select_boot_master.H>

static void ipl_pre0(void)
{
	ipl_pre();
}

static int ipl_poweron(void)
{
	return -1;
}

static int ipl_startipl(void)
{
	return -1;
}

static int ipl_set_ref_clock(void)
{
	struct pdbg_target *proc;
	int rc = 0;

	pdbg_for_each_class_target("proc", proc) {
		fapi2::ReturnCode fapirc;

		if (pdbg_target_status(proc) != PDBG_TARGET_ENABLED)
			continue;

		fapirc = p10_setup_ref_clock(proc);
		if (fapirc != fapi2::FAPI2_RC_SUCCESS) {
			ipl_log(IPL_ERROR, "Istep set_ref_clock failed on chip %d, rc=%d\n",
                                pdbg_target_index(proc), fapirc);
			rc++;
		}

		ipl_error_callback(fapirc == fapi2::FAPI2_RC_SUCCESS);
	}

	return rc;
}

static int ipl_proc_clock_test(void)
{
	struct pdbg_target *proc;
	int rc = 0;

	pdbg_for_each_class_target("proc", proc) {
		fapi2::ReturnCode fapirc;

		if (pdbg_target_status(proc) != PDBG_TARGET_ENABLED)
			continue;

		fapirc = p10_clock_test(proc);
		if (fapirc != fapi2::FAPI2_RC_SUCCESS) {
			ipl_log(IPL_ERROR, "HWP clock_test failed on proc %d, rc=%d\n",
				pdbg_target_index(proc), fapirc);
			rc++;
		}

		ipl_error_callback(fapirc == fapi2::FAPI2_RC_SUCCESS);
	}

	return rc;
}

static int ipl_proc_prep_ipl(void)
{
	return -1;
}

static bool is_master_proc(struct pdbg_target *proc)
{
	uint8_t type;

	if (!pdbg_target_get_attribute(proc, "ATTR_PROC_MASTER_TYPE", 1, 1, &type)) {
		ipl_log(IPL_ERROR, "Attribute [ATTR_PROC_MASTER_TYPE] read failed \n");
		/* TODO: Revisit this error logic. */
		return false;
	}

	/* Attribute value 0 corresponds to master processor */
	if (type == 0)
		return true;
	else
		return false;
}

static int ipl_proc_select_boot_master(void)
{
	struct pdbg_target *proc;
	int rc = 1;

	pdbg_for_each_class_target("proc", proc) {
		fapi2::ReturnCode fapirc;

		if (pdbg_target_status(proc) != PDBG_TARGET_ENABLED)
			continue;

		if (!is_master_proc(proc))
			continue;

		fapirc = p10_select_boot_master(proc);
		if (fapirc == fapi2::FAPI2_RC_SUCCESS)
			rc = 0;

		ipl_error_callback(fapirc == fapi2::FAPI2_RC_SUCCESS);
		break;
        }

	return rc;
}

static void set_core_status(void)
{
	struct pdbg_target *core;

	pdbg_for_each_class_target("core", core) {
		uint8_t buf[5];

		if (!pdbg_target_get_attribute_packed(core,
						      "ATTR_HWAS_STATE",
						      "41",
						      buf)) {
			ipl_log(IPL_ERROR, "Attribute [ATTR_HWAS_STATE] read failed\n");
			continue;
		}

		// TODO hwas state should be set based on guard records status.
		buf[4] |= 0x40;

		if (!pdbg_target_set_attribute_packed(core,
						      "ATTR_HWAS_STATE",
						      "41",
						      buf)) {
			ipl_log(IPL_ERROR, "Attribute [ATTR_HWAS_STATE] update failed \n");
			continue;
		}
	}
}

static bool small_core_enabled(void)
{
	struct stat statbuf;
	int ret;

	/* If /tmp/small_core file exists, boot in small core mode */
	ret = stat("/tmp/small_core", &statbuf);
	if (ret == -1)
		return false;

	if (S_ISREG(statbuf.st_mode)) {
		ipl_log(IPL_INFO, "Booting in small core mode\n");
		return true;
	}

	return false;
}

static int ipl_sbe_config_update(void)
{
	struct pdbg_target *root, *proc;
	uint32_t boot_flags = 0;
	int rc = 0;
	uint8_t istep_mode, core_mode;

	root = pdbg_target_root();
	if (!pdbg_target_get_attribute(root, "ATTR_ISTEP_MODE", 1, 1, &istep_mode)) {
		ipl_log(IPL_ERROR, "Attribute [ATTR_ISTEP_MODE] read failed \n");
		return 1;
	}

	// Bit 0 indicates istep IPL (0b1) (Used by SBE, HB – ATTR_ISTEP_MODE)
	if (istep_mode)
		boot_flags = 0x80000000;

	if (!pdbg_target_set_attribute(root, "ATTR_BOOT_FLAGS", 4, 1, &boot_flags)) {
		ipl_log(IPL_ERROR, "Attribute [ATTR_BOOT_FLAGS] update failed \n");
		return 1;
	}

	//Initialize core target functional status
	set_core_status();

	if (small_core_enabled())
		core_mode = 0x00;  /* CORE_UNFUSED mode */
	else
		core_mode = 0x01;  /* CORE_FUSED mode */

	if (!pdbg_target_set_attribute(root, "ATTR_FUSED_CORE_MODE", 1, 1, &core_mode)) {
		ipl_log(IPL_ERROR, "Attribute [ATTR_FUSED_CORE_MODE] update failed \n");
		return 1;
	}

	pdbg_for_each_class_target("proc", proc) {
		fapi2::ReturnCode fapirc;

		if (pdbg_target_status(proc) != PDBG_TARGET_ENABLED)
			continue;

		fapirc = p10_setup_sbe_config(proc);
		if (fapirc != fapi2::FAPI2_RC_SUCCESS)
			rc++;

		ipl_error_callback(fapirc == fapi2::FAPI2_RC_SUCCESS);
	}

	return rc;
}

static int ipl_sbe_start(void)
{
	struct pdbg_target *proc;
	int rc = 0;

	pdbg_for_each_class_target("proc", proc) {
		fapi2::ReturnCode fapirc;

		if (pdbg_target_status(proc) != PDBG_TARGET_ENABLED)
			continue;

		fapirc = p10_start_cbs(proc, true);
		if (fapirc != fapi2::FAPI2_RC_SUCCESS)
			rc++;

		ipl_error_callback(fapirc == fapi2::FAPI2_RC_SUCCESS);
	}

	/*
         * Pause here for few seconds to give some time for sbe to boot
         * FIXME: Replace this with HWP which will check SBE boot status
	 */
        sleep(5);

	return rc;
}

static struct ipl_step ipl0[] = {
	{ IPL_DEF(poweron),                   0,  1,  true,  true  },
	{ IPL_DEF(startipl),                  0,  2,  true,  true  },
	{ IPL_DEF(set_ref_clock),             0,  6,  true,  true  },
	{ IPL_DEF(proc_clock_test),           0,  7,  true,  true  },
	{ IPL_DEF(proc_prep_ipl),             0,  8,  true,  true  },
	{ IPL_DEF(proc_select_boot_master),   0, 11,  true,  true  },
	{ IPL_DEF(sbe_config_update),         0, 13,  true,  true  },
	{ IPL_DEF(sbe_start),                 0, 14,  true,  true  },
	{ NULL, NULL, -1, -1, false, false },
};

__attribute__((constructor))
static void ipl_register_ipl0(void)
{
	ipl_register(0, ipl0, ipl_pre0);
}
