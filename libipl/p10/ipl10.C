extern "C" {
#include <stdio.h>

#include <libpdbg.h>

#include "libipl.h"
#include "libipl_internal.h"
}

#include "common.H"

static void ipl_pre10(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib) {
		if (ipl_mode() == IPL_DEFAULT && pdbg_target_index(pib) != 0)
			continue;

		pdbg_target_probe(pib);
	}
}

static int ipl_proc_build_smp(void)
{
	return ipl_istep_via_hostboot(10, 1);
}

static int ipl_host_slave_sbe_update(void)
{
	return ipl_istep_via_hostboot(10, 2);
}

static int ipl_host_secureboot_lockdown(void)
{
	return ipl_istep_via_hostboot(10, 3);
}

static int ipl_proc_chiplet_scominit(void)
{
	return ipl_istep_via_hostboot(10, 4);
}

static int ipl_proc_pau_scominit(void)
{
	return ipl_istep_via_hostboot(10, 5);
}

static int ipl_proc_pcie_scominit(void)
{
	return ipl_istep_via_hostboot(10, 6);
}

static int ipl_proc_scomoverride_chiplets(void)
{
	return ipl_istep_via_hostboot(10, 7);
}

static int ipl_proc_chiplet_enable_ridi(void)
{
	return ipl_istep_via_hostboot(10, 8);
}

static int ipl_host_rng_bist(void)
{
	return ipl_istep_via_hostboot(10, 9);
}

static struct ipl_step ipl10[] = {
	{ IPL_DEF(proc_build_smp),               10,  1,  true,  true  },
	{ IPL_DEF(host_slave_sbe_update),        10,  2,  false, true  },
	{ IPL_DEF(host_secureboot_lockdown),     10,  3,  true,  true  },
	{ IPL_DEF(proc_chiplet_scominit),        10,  4,  true,  true  },
	{ IPL_DEF(proc_pau_scominit),            10,  5,  true,  true  },
	{ IPL_DEF(proc_pcie_scominit),           10,  6,  true,  true  },
	{ IPL_DEF(proc_scomoverride_chiplets),   10,  7,  true,  true  },
	{ IPL_DEF(proc_chiplet_enable_ridi),     10,  8,  true,  true  },
	{ IPL_DEF(host_rng_bist),                10,  9,  true,  true  },
	{ NULL, NULL, -1, -1, false, false },
};

__attribute__((constructor))
static void ipl_register_ipl10(void)
{
	ipl_register(10, ipl10, ipl_pre10);
}