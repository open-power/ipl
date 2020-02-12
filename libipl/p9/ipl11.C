extern "C" {
#include <stdio.h>

#include <libpdbg.h>
}

#include "libipl.H"
#include "libipl_internal.H"

static void ipl_pre11(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib)
		pdbg_target_probe(pib);
}

static int ipl_host_prd_hwreconfig(void)
{
	return -1;
}

static int ipl_cen_tp_chiplet_init1(void)
{
	return -1;
}

static int ipl_cen_pll_initf(void)
{
	return -1;
}

static int ipl_cen_pll_setup(void)
{
	return -1;
}

static int ipl_cen_tp_chiplet_init2(void)
{
	return -1;
}

static int ipl_cen_tp_arrayinit(void)
{
	return -1;
}

static int ipl_cen_tp_chiplet_init3(void)
{
	return -1;
}

static int ipl_cen_chiplet_init(void)
{
	return -1;
}

static int ipl_cen_arrayinit(void)
{
	return -1;
}

static int ipl_cen_initf(void)
{
	return -1;
}

static int ipl_cen_do_manual_inits(void)
{
	return -1;
}

static int ipl_cen_startclocks(void)
{
	return -1;
}

static int ipl_cen_scominits(void)
{
	return -1;
}

static struct ipl_step ipl11[] = {
	{ IPL_DEF(host_prd_hwreconfig),  11,  1,  true,  true  },
	{ IPL_DEF(cen_tp_chiplet_init1), 11,  2,  true,  true  },
	{ IPL_DEF(cen_pll_initf),        11,  3,  true,  true  },
	{ IPL_DEF(cen_pll_setup),        11,  4,  true,  true  },
	{ IPL_DEF(cen_tp_chiplet_init2), 11,  5,  true,  true  },
	{ IPL_DEF(cen_tp_arrayinit),     11,  6,  true,  true  },
	{ IPL_DEF(cen_tp_chiplet_init3), 11,  7,  true,  true  },
	{ IPL_DEF(cen_chiplet_init),     11,  8,  true,  true  },
	{ IPL_DEF(cen_arrayinit),        11,  9,  true,  true  },
	{ IPL_DEF(cen_initf),            11, 10,  true,  true  },
	{ IPL_DEF(cen_do_manual_inits),  11, 11,  true,  true  },
	{ IPL_DEF(cen_startclocks),      11, 12,  true,  true  },
	{ IPL_DEF(cen_scominits),        11, 13,  true,  true  },
	{ NULL, NULL, -1, -1, false, false },
};

__attribute__((constructor))
static void ipl_register_ipl11(void)
{
	ipl_register(11, ipl11, ipl_pre11);
}
