extern "C" {
#include <stdio.h>

#include <libpdbg.h>

#include "libipl.h"
#include "libipl_internal.h"
}

#include "common.H"

static void ipl_pre6(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib) {
		if (ipl_mode() == IPL_DEFAULT && pdbg_target_index(pib) != 0)
			continue;

		pdbg_target_probe(pib);
	}
}

static int ipl_host_bootloader(void)
{
	return -1;
}

static int ipl_host_setup(void)
{
	return -1;
}

static int ipl_host_istep_enable(void)
{
	return -1;
}

static int ipl_host_init_fsi(void)
{
	return ipl_istep_via_hostboot(6, 4);
}

static int ipl_host_set_ipl_parms(void)
{
	return ipl_istep_via_hostboot(6, 5);
}

static int ipl_host_discover_targets(void)
{
	return ipl_istep_via_hostboot(6, 6);
}

static int ipl_host_update_master_tpm(void)
{
	return ipl_istep_via_hostboot(6, 7);
}

static int ipl_host_gard(void)
{
	return ipl_istep_via_hostboot(6, 8);
}

static int ipl_host_voltage_config(void)
{
	return ipl_istep_via_hostboot(6, 9);
}

static struct ipl_step ipl6[] = {
	{ IPL_DEF(host_bootloader),          6,  1,  true,  true  },
	{ IPL_DEF(host_setup),               6,  2,  true,  true  },
	{ IPL_DEF(host_istep_enable),        6,  3,  true,  true  },
	{ IPL_DEF(host_init_fsi),            6,  4,  true,  true  },
	{ IPL_DEF(host_set_ipl_parms),       6,  5,  true,  true  },
	{ IPL_DEF(host_discover_targets),    6,  6,  true,  true  },
	{ IPL_DEF(host_update_master_tpm),   6,  7,  true,  true  },
	{ IPL_DEF(host_gard),                6,  8,  true,  true  },
	{ IPL_DEF(host_voltage_config),      6,  9,  true,  true  },
	{ NULL, NULL, -1, -1, false, false },
};

__attribute__((constructor))
static void ipl_register_ipl6(void)
{
	ipl_register(6, ipl6, ipl_pre6);
}
