extern "C" {
#include <stdio.h>

#include <libpdbg.h>

#include "libipl.h"
#include "libipl_internal.h"
}

#include "common.H"

static void ipl_pre14(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib) {
		if (ipl_mode() == IPL_DEFAULT && pdbg_target_index(pib) != 0)
			continue;

		pdbg_target_probe(pib);
	}
}

static int ipl_mss_memdiag(void)
{
	return ipl_istep_via_hostboot(14, 1);
}

static int ipl_mss_thermal_init(void)
{
	return ipl_istep_via_hostboot(14, 2);
}

static int ipl_proc_load_iop_xram(void)
{
	return ipl_istep_via_hostboot(14, 3);
}

static int ipl_proc_pcie_config(void)
{
	return ipl_istep_via_hostboot(14, 4);
}

static int ipl_proc_setup_mmio_bars(void)
{
	return ipl_istep_via_hostboot(14, 5);
}

static int ipl_proc_htm_setup(void)
{
	return ipl_istep_via_hostboot(14, 6);
}

static int ipl_proc_exit_cache_contained(void)
{
	return ipl_istep_via_hostboot(14, 7);
}

static int ipl_host_mpipl_service(void)
{
	return ipl_istep_via_hostboot(14, 8);
}

static int ipl_proc_psiinit(void)
{
	return ipl_istep_via_hostboot(14, 9);
}

static struct ipl_step ipl14[] = {
	{ IPL_DEF(mss_memdiag),                 14,  1,  true,  true  },
	{ IPL_DEF(mss_thermal_init),            14,  2,  true,  true  },
	{ IPL_DEF(proc_load_iop_xram),          14,  3,  true,  true  },
	{ IPL_DEF(proc_pcie_config),            14,  4,  true,  true  },
	{ IPL_DEF(proc_setup_mmio_bars),        14,  5,  true,  true  },
	{ IPL_DEF(proc_exit_cache_contained),   14,  6,  true,  true  },
	{ IPL_DEF(proc_htm_setup),              14,  7,  true,  true  },
	{ IPL_DEF(host_mpipl_service),          14,  8,  true,  true  },
	{ IPL_DEF(proc_psiinit),                14,  9,  true,  true  },
	{ NULL, NULL, -1, -1, false, false },
};

__attribute__((constructor))
static void ipl_register_ipl14(void)
{
	ipl_register(14, ipl14, ipl_pre14);
}
