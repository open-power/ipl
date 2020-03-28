extern "C" {
#include <stdio.h>
#include <unistd.h>

#include <libpdbg.h>
}

#include "libipl.H"
#include "libipl_internal.H"
#include <ekb/chips/p10/procedures/hwp/perv/p10_start_cbs.H>
#include <ekb/chips/p10/procedures/hwp/perv/p10_setup_ref_clock.H>
#include <ekb/chips/p10/procedures/hwp/perv/p10_clock_test.H>

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

		fapirc = p10_setup_ref_clock(proc);
		if (fapirc != fapi2::FAPI2_RC_SUCCESS) {
			ipl_log(IPL_ERROR, "Istep set_ref_clock failed on chip %d, rc=%d\n",
                                pdbg_target_index(proc), fapirc);
			ipl_error_callback(true);
			rc++;
		}
	}

	return rc;
}

static int ipl_proc_clock_test(void)
{
	struct pdbg_target *proc;
	int rc = 0;

	pdbg_for_each_class_target("proc", proc) {
		fapi2::ReturnCode fapirc;

		fapirc = p10_clock_test(proc);
		if (fapirc != fapi2::FAPI2_RC_SUCCESS) {
			ipl_log(IPL_ERROR, "HWP clock_test failed on proc %d, rc=%d\n",
				pdbg_target_index(proc), fapirc);
			ipl_error_callback(true);
			rc++;
		}
	}

	return rc;
}

static int ipl_proc_prep_ipl(void)
{
	return -1;
}

static int ipl_proc_select_boot_master(void)
{
	return -1;
}

static int ipl_sbe_config_update(void)
{
	return -1;
}

static int ipl_sbe_start(void)
{
	struct pdbg_target *proc;

	pdbg_for_each_class_target("proc", proc) {
		if (ipl_mode() <= IPL_DEFAULT && pdbg_target_index(proc) != 0)
			continue;

		ipl_error_callback((p10_start_cbs(proc, true) == fapi2::FAPI2_RC_SUCCESS));
	}

	/*
         * Pause here for few seconds to give some time for sbe to boot
         * FIXME: Replace this with HWP which will check SBE boot status
	 */
        sleep(5);

	return 0;
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
