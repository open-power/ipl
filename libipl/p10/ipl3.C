extern "C" {
#include <stdio.h>

#include <libpdbg.h>

#include "libipl.h"
#include "libipl_internal.h"
}

static void ipl_pre3(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib)
		pdbg_target_probe(pib);
}

static int ipl_proc_sbe_chiplet_setup(void)
{
	return -1;
}

static int ipl_proc_sbe_chiplet_clk_config(void)
{
	return -1;
}

static int ipl_proc_sbe_chiplet_reset(void)
{
	return -1;
}

static int ipl_proc_sbe_gptr_time_initf(void)
{
	return -1;
}

static int ipl_proc_sbe_chiplet_pll_initf(void)
{
	return -1;
}

static int ipl_proc_sbe_chiplet_pll_setup(void)
{
	return -1;
}

static int ipl_proc_sbe_repr_initf(void)
{
	return -1;
}

static int ipl_proc_sbe_abist_setup(void)
{
	return -1;
}

static int ipl_proc_sbe_arrayinit(void)
{
	return -1;
}

static int ipl_proc_sbe_lbist(void)
{
	return -1;
}

static int ipl_proc_sbe_initf(void)
{
	return -1;
}

static int ipl_proc_sbe_startclocks(void)
{
	return -1;
}

static int ipl_proc_sbe_chiplet_init(void)
{
	return -1;
}

static int ipl_proc_sbe_chiplet_fir_init(void)
{
	return -1;
}

static int ipl_proc_sbe_dts_init(void)
{
	return -1;
}

static int ipl_proc_sbe_skew_adjust_setup(void)
{
	return -1;
}

static int ipl_proc_sbe_nest_enable_ridi(void)
{
	return -1;
}

static int ipl_proc_sbe_scominit(void)
{
	return -1;
}

static int ipl_proc_sbe_lpc(void)
{
	return -1;
}

static int ipl_proc_sbe_fabricinit(void)
{
	return -1;
}

static int ipl_proc_sbe_check_master(void)
{
	return -1;
}

static int ipl_proc_sbe_mcs_setup(void)
{
	return -1;
}

static int ipl_proc_sbe_select_ex(void)
{
	return -1;
}

static struct ipl_step ipl3[] = {
	{ IPL_DEF(proc_sbe_chiplet_setup),        3,  1,  true,  true  },
	{ IPL_DEF(proc_sbe_chiplet_clk_config),   3,  2,  true,  true  },
	{ IPL_DEF(proc_sbe_chiplet_reset),        3,  3,  true,  true  },
	{ IPL_DEF(proc_sbe_gptr_time_initf),      3,  4,  true,  true  },
	{ IPL_DEF(proc_sbe_chiplet_pll_initf),    3,  5,  true,  true  },
	{ IPL_DEF(proc_sbe_chiplet_pll_setup),    3,  6,  true,  true  },
	{ IPL_DEF(proc_sbe_repr_initf),           3,  7,  true,  true  },
	{ IPL_DEF(proc_sbe_abist_setup),          3,  8,  true,  true  },
	{ IPL_DEF(proc_sbe_arrayinit),            3,  9,  true,  true  },
	{ IPL_DEF(proc_sbe_lbist),                3, 10,  true,  true  },
	{ IPL_DEF(proc_sbe_initf),                3, 11,  true,  true  },
	{ IPL_DEF(proc_sbe_startclocks),          3, 12,  true,  true  },
	{ IPL_DEF(proc_sbe_chiplet_init),         3, 13,  true,  true  },
	{ IPL_DEF(proc_sbe_chiplet_fir_init),     3, 14,  true,  true  },
	{ IPL_DEF(proc_sbe_dts_init),             3, 15,  true,  true  },
	{ IPL_DEF(proc_sbe_skew_adjust_setup),    3, 16,  true,  true  },
	{ IPL_DEF(proc_sbe_nest_enable_ridi),     3, 17,  true,  true  },
	{ IPL_DEF(proc_sbe_scominit),             3, 18,  true,  true  },
	{ IPL_DEF(proc_sbe_lpc),                  3, 19,  true,  true  },
	{ IPL_DEF(proc_sbe_fabricinit),           3, 20,  true,  true  },
	{ IPL_DEF(proc_sbe_check_master),         3, 21,  true,  true  },
	{ IPL_DEF(proc_sbe_mcs_setup),            3, 22,  true,  true  },
	{ IPL_DEF(proc_sbe_select_ex),            3, 23,  true,  true  },
	{ NULL, NULL, -1, -1, false, false },
};

__attribute__((constructor))
static void ipl_register_ipl3(void)
{
	ipl_register(3, ipl3, ipl_pre3);
}
