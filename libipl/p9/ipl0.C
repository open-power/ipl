extern "C" {
#include <stdio.h>

#include <libpdbg.h>
}

#include "libipl.H"
#include "libipl_internal.H"

#include <ekb/chips/p9/procedures/hwp/perv/p9_pre_poweron.H>
#include <ekb/chips/centaur/procedures/hwp/perv/cen_pre_poweron.H>
#include <ekb/chips/p9/procedures/hwp/perv/p9_select_clock_mux.H>
#include <ekb/chips/p9/procedures/hwp/perv/p9_clock_test.H>
#include <ekb/chips/p9/procedures/hwp/perv/p9_set_fsi_gp_shadow.H>
#include <ekb/chips/p9/procedures/hwp/perv/p9_select_boot_master.H>
#include <ekb/chips/p9/procedures/hwp/perv/p9_setup_sbe_config.H>
#include <ekb/chips/p9/procedures/hwp/perv/p9_start_cbs.H>
#include <ekb/chips/p9/procedures/hwp/perv/p9_nv_ref_clk_enable.H>

static void ipl_pre0(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib) {
		if (ipl_mode() == IPL_DEFAULT && pdbg_target_index(pib) != 0)
			continue;

		pdbg_target_probe(pib);
	}
}

static int ipl_poweron(void)
{
	struct pdbg_target *pib, *memb;

	pdbg_for_each_class_target("pib", pib) {
		if (ipl_mode() <= IPL_DEFAULT && pdbg_target_index(pib) != 0)
			continue;

		p9_pre_poweron(pib);
	}

	pdbg_for_each_class_target("memb", memb) {
		if (ipl_mode() <= IPL_DEFAULT && pdbg_target_index(pib) != 0)
			continue;

		cen_pre_poweron(memb);
	}

	return 0;
}

static int ipl_startipl(void)
{
	return -1;
}

static int ipl_set_ref_clock(void)
{
	return -1;
}

static int ipl_proc_clock_test(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib) {
		if (ipl_mode() <= IPL_DEFAULT && pdbg_target_index(pib) != 0)
			continue;

		p9_select_clock_mux(pib);
	}

	pdbg_for_each_class_target("pib", pib) {
		if (ipl_mode() <= IPL_DEFAULT && pdbg_target_index(pib) != 0)
			continue;
		p9_clock_test(pib, BOTH_SRC0);
	}

	return 0;
}

static int ipl_proc_prep_ipl(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib) {
		if (ipl_mode() <= IPL_DEFAULT && pdbg_target_index(pib) != 0)
			continue;

		p9_set_fsi_gp_shadow(pib);
	}

	return 0;
}

static int ipl_proc_select_boot_master(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib) {
		if (ipl_mode() <= IPL_DEFAULT && pdbg_target_index(pib) != 0)
			continue;

		p9_select_boot_master(pib);
	}

	return 0;
}

static int ipl_sbe_config_update(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib) {
		if (ipl_mode() <= IPL_DEFAULT && pdbg_target_index(pib) != 0)
			continue;

		p9_setup_sbe_config(pib);
	}

	return 0;
}

static int ipl_sbe_start(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib) {
		if (ipl_mode() <= IPL_DEFAULT && pdbg_target_index(pib) != 0)
			continue;

		p9_start_cbs(pib, true);
	}

	pdbg_for_each_class_target("pib", pib) {
		if (ipl_mode() <= IPL_DEFAULT && pdbg_target_index(pib) != 0)
			continue;

		p9_nv_ref_clk_enable(pib);
	}

	return 0;
}

static struct ipl_step ipl0[] = {
	{ IPL_DEF(poweron),                 0,  1,  true,  true  },
	{ IPL_DEF(startipl),                0,  2,  false, true  },
	{ IPL_DEF(set_ref_clock),           0,  6,  true,  true  },
	{ IPL_DEF(proc_clock_test),         0,  7,  true,  true  },
	{ IPL_DEF(proc_prep_ipl),           0,  8,  true,  true  },
	{ IPL_DEF(proc_select_boot_master), 0, 11,  true,  true  },
	{ IPL_DEF(sbe_config_update),       0, 13,  true,  true  },
	{ IPL_DEF(sbe_start),               0, 14,  true,  true  },
	{ NULL, NULL, -1, -1, false, false },
};

__attribute__((constructor))
static void ipl_register_ipl0(void)
{
	ipl_register(0, ipl0, ipl_pre0);
}
