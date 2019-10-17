extern "C" {
#include <stdio.h>

#include <libpdbg.h>

#include "libipl.h"
#include "libipl_internal.h"
}

static void ipl_pre4(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib)
		pdbg_target_probe(pib);
}

static int ipl_proc_hcd_cache_poweron(void)
{
	return -1;
}

static int ipl_proc_hcd_cache_chiplet_reset(void)
{
	return -1;
}

static int ipl_proc_hcd_cache_chiplet_l3_dcc_setup(void)
{
	return -1;
}

static int ipl_proc_hcd_cache_gptr_time_initf(void)
{
	return -1;
}

static int ipl_proc_hcd_cache_dpll_initf(void)
{
	return -1;
}

static int ipl_proc_hcd_cache_dpll_setup(void)
{
	return -1;
}

static int ipl_proc_hcd_cache_dcc_skewadjust_setup(void)
{
	return -1;
}

static int ipl_proc_hcd_cache_chiplet_init(void)
{
	return -1;
}

static int ipl_proc_hcd_cache_repair_initf(void)
{
	return -1;
}

static int ipl_proc_hcd_cache_arrayinit(void)
{
	return -1;
}

static int ipl_proc_hcd_cache_abist(void)
{
	return -1;
}

static int ipl_proc_hcd_cache_lbist(void)
{
	return -1;
}

static int ipl_proc_hcd_cache_initf(void)
{
	return -1;
}

static int ipl_proc_hcd_cache_startclocks(void)
{
	return -1;
}

static int ipl_proc_hcd_cache_scominit(void)
{
	return -1;
}

static int ipl_proc_hcd_cache_scom_customize(void)
{
	return -1;
}

static int ipl_proc_hcd_cache_ras_runtime_scom(void)
{
	return -1;
}

static int ipl_proc_hcd_cache_occ_runtime_scom(void)
{
	return -1;
}

static int ipl_proc_hcd_exit_mode(void)
{
	return -1;
}

static int ipl_proc_hcd_core_pcb_arb(void)
{
	return -1;
}

static int ipl_proc_hcd_core_poweron(void)
{
	return -1;
}

static int ipl_proc_hcd_core_chiplet_reset(void)
{
	return -1;
}

static int ipl_proc_hcd_core_gptr_time_initf(void)
{
	return -1;
}

static int ipl_proc_hcd_core_chiplet_init(void)
{
	return -1;
}

static int ipl_proc_hcd_core_repair_initf(void)
{
	return -1;
}

static int ipl_proc_hcd_core_arrayinit(void)
{
	return -1;
}

static int ipl_proc_hcd_core_abist(void)
{
	return -1;
}

static int ipl_proc_hcd_core_lbist(void)
{
	return -1;
}

static int ipl_proc_hcd_core_initf(void)
{
	return -1;
}

static int ipl_proc_hcd_core_startclocks(void)
{
	return -1;
}

static int ipl_proc_hcd_core_scominit(void)
{
	return -1;
}

static int ipl_proc_hcd_core_scom_customize(void)
{
	return -1;
}

static int ipl_proc_hcd_core_ras_runtime_scom(void)
{
	return -1;
}

static int ipl_proc_hcd_core_occ_runtime_scom(void)
{
	return -1;
}

static struct ipl_step ipl4[] = {
	{ IPL_DEF(proc_hcd_cache_poweron),                4,  1,  true,  true  },
	{ IPL_DEF(proc_hcd_cache_chiplet_reset),          4,  2,  true,  true  },
	{ IPL_DEF(proc_hcd_cache_chiplet_l3_dcc_setup),   4,  3,  true,  true  },
	{ IPL_DEF(proc_hcd_cache_gptr_time_initf),        4,  4,  true,  true  },
	{ IPL_DEF(proc_hcd_cache_dpll_initf),             4,  5,  true,  true  },
	{ IPL_DEF(proc_hcd_cache_dpll_setup),             4,  6,  true,  true  },
	{ IPL_DEF(proc_hcd_cache_dcc_skewadjust_setup),   4,  7,  true,  true  },
	{ IPL_DEF(proc_hcd_cache_chiplet_init),           4,  8,  true,  true  },
	{ IPL_DEF(proc_hcd_cache_repair_initf),           4,  9,  true,  true  },
	{ IPL_DEF(proc_hcd_cache_arrayinit),              4, 10,  true,  true  },
	{ IPL_DEF(proc_hcd_cache_abist),                  4, 11,  true,  false },
	{ IPL_DEF(proc_hcd_cache_lbist),                  4, 12,  true,  false },
	{ IPL_DEF(proc_hcd_cache_initf),                  4, 13,  true,  true  },
	{ IPL_DEF(proc_hcd_cache_startclocks),            4, 14,  true,  true  },
	{ IPL_DEF(proc_hcd_cache_scominit),               4, 15,  true,  true  },
	{ IPL_DEF(proc_hcd_cache_scom_customize),         4, 16,  true,  true  },
	{ IPL_DEF(proc_hcd_cache_ras_runtime_scom),       4, 17,  true,  true  },
	{ IPL_DEF(proc_hcd_cache_occ_runtime_scom),       4, 18,  true,  true  },
	{ IPL_DEF(proc_hcd_exit_mode),                    4, 19,  true,  true  },
	{ IPL_DEF(proc_hcd_core_pcb_arb),                 4, 20,  true,  true  },
	{ IPL_DEF(proc_hcd_core_poweron),                 4, 21,  true,  true  },
	{ IPL_DEF(proc_hcd_core_chiplet_reset),           4, 22,  true,  true  },
	{ IPL_DEF(proc_hcd_core_gptr_time_initf),         4, 23,  true,  true  },
	{ IPL_DEF(proc_hcd_core_chiplet_init),            4, 24,  true,  true  },
	{ IPL_DEF(proc_hcd_core_repair_initf),            4, 25,  true,  true  },
	{ IPL_DEF(proc_hcd_core_arrayinit),               4, 26,  true,  true  },
	{ IPL_DEF(proc_hcd_core_abist),                   4, 27,  true,  false },
	{ IPL_DEF(proc_hcd_core_lbist),                   4, 28,  true,  false },
	{ IPL_DEF(proc_hcd_core_initf),                   4, 29,  true,  true  },
	{ IPL_DEF(proc_hcd_core_startclocks),             4, 30,  true,  true  },
	{ IPL_DEF(proc_hcd_core_scominit),                4, 31,  true,  true  },
	{ IPL_DEF(proc_hcd_core_scom_customize),          4, 32,  true,  true  },
	{ IPL_DEF(proc_hcd_core_ras_runtime_scom),        4, 33,  true,  true  },
	{ IPL_DEF(proc_hcd_core_occ_runtime_scom),        4, 34,  true,  true  },
	{ NULL, NULL, -1, -1, false, false },
};

__attribute__((constructor))
static void ipl_register_ipl4(void)
{
	ipl_register(4, ipl4, ipl_pre4);
}
