extern "C" {
#include <stdio.h>

#include <libpdbg.h>

#include "libipl.h"
#include "libipl_internal.h"
}

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

	pdbg_for_each_class_target("pib", pib)
		pdbg_target_probe(pib);
}

static int ipl_poweron(void)
{
	struct pdbg_target *pib, *memb;

	pdbg_for_each_class_target("pib", pib)
		p9_pre_poweron(pib);

	pdbg_for_each_class_target("memb", memb)
		cen_pre_poweron(memb);

	return 0;
}

static int ipl_startipl(void)
{
	return -1;
}

static int ipl_disableattns(void)
{
	return -1;
}

static int ipl_updatehwmodel(void)
{
	return -1;
}

static int ipl_alignment_check(void)
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

	pdbg_for_each_class_target("pib", pib)
		p9_select_clock_mux(pib);

	pdbg_for_each_class_target("pib", pib)
		p9_clock_test(pib, BOTH_SRC0);

	return 0;
}

static int ipl_proc_prep_ipl(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib)
		p9_set_fsi_gp_shadow(pib);

	return 0;
}

static int ipl_edramrepair(void)
{
	return -1;
}

static int ipl_asset_protection(void)
{
	return -1;
}

static int ipl_proc_select_boot_master(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib)
		p9_select_boot_master(pib);

	return 0;
}

static int ipl_hb_config_update(void)
{
	return -1;
}

static int ipl_sbe_config_update(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib)
		p9_setup_sbe_config(pib);

	return 0;
}

static int ipl_sbe_start(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib)
		p9_start_cbs(pib, false);

	pdbg_for_each_class_target("pib", pib)
		p9_nv_ref_clk_enable(pib);

	return 0;
}

static int ipl_startPRD(void)
{
	return -1;
}

static int ipl_proc_attn_listen(void)
{
	return -1;
}

static struct ipl_step ipl0[] = {
	{ IPL_DEF(poweron),                 0,  1,  true,  true  },
	{ IPL_DEF(startipl),                0,  2,  false, true  },
	{ IPL_DEF(disableattns),            0,  3,  false, true  },
	{ IPL_DEF(updatehwmodel),           0,  4,  false, true  },
	{ IPL_DEF(alignment_check),         0,  5,  false, true  },
	{ IPL_DEF(set_ref_clock),           0,  6,  true,  true  },
	{ IPL_DEF(proc_clock_test),         0,  7,  true,  true  },
	{ IPL_DEF(proc_prep_ipl),           0,  8,  true,  true  },
	{ IPL_DEF(edramrepair),             0,  9,  false, true  },
	{ IPL_DEF(asset_protection),        0, 10,  false, true  },
	{ IPL_DEF(proc_select_boot_master), 0, 11,  true,  true  },
	{ IPL_DEF(hb_config_update),        0, 12,  false, true  },
	{ IPL_DEF(sbe_config_update),       0, 13,  true,  true  },
	{ IPL_DEF(sbe_start),               0, 14,  true,  true  },
	{ IPL_DEF(startPRD),                0, 15,  false, true  },
	{ IPL_DEF(proc_attn_listen),        0, 16,  false, true  },
	{ NULL, NULL, -1, -1, false, false },
};

__attribute__((constructor))
static void ipl_register_ipl0(void)
{
	ipl_register(0, ipl0, ipl_pre0);
}
