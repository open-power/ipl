extern "C" {
#include <stdio.h>

#include <libpdbg.h>

#include "libipl.h"
#include "libipl_internal.h"
}

#include <ekb/chips/p10/procedures/hwp/perv/p10_start_cbs.H>

static void ipl_pre0(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib)
		pdbg_target_probe(pib);
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
	return -1;
}

static int ipl_proc_clock_test(void)
{
	return -1;
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
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib) {
		if (ipl_mode() == IPL_DEFAULT && pdbg_target_index(pib) != 0)
			continue;

		p10_start_cbs(pib, true);
	}

	return 0;
}

static struct ipl_step ipl0[] = {
	{ IPL_DEF(poweron),                   0,  2,  true,  true  },
	{ IPL_DEF(startipl),                  0,  3,  true,  true  },
	{ IPL_DEF(set_ref_clock),             0,  7,  true,  true  },
	{ IPL_DEF(proc_clock_test),           0,  8,  true,  true  },
	{ IPL_DEF(proc_prep_ipl),             0,  9,  true,  true  },
	{ IPL_DEF(proc_select_boot_master),   0, 12,  true,  true  },
	{ IPL_DEF(sbe_config_update),         0, 14,  true,  true  },
	{ IPL_DEF(sbe_start),                 0, 15,  true,  true  },
	{ NULL, NULL, -1, -1, false, false },
};

__attribute__((constructor))
static void ipl_register_ipl0(void)
{
	ipl_register(0, ipl0, ipl_pre0);
}
