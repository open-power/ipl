extern "C" {
#include <stdio.h>

#include <libpdbg.h>
}

#include "libipl.H"
#include "libipl_internal.H"

static void ipl_pre6(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib)
		pdbg_target_probe(pib);
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

static int ipl_host_init_bmc_pcie(void)
{
	return -1;
}

static int ipl_host_init_fsi(void)
{
	return -1;
}

static int ipl_host_set_ipl_parms(void)
{
	return -1;
}

static int ipl_host_discover_targets(void)
{
	return -1;
}

static int ipl_host_update_master_tpm(void)
{
	return -1;
}

static int ipl_host_gard(void)
{
	return -1;
}

static int ipl_host_revert_sbe_mcs_setup(void)
{
	return -1;
}

static int ipl_host_start_occ_xstop_handler(void)
{
	return -1;
}

static int ipl_host_voltage_config(void)
{
	return -1;
}

static struct ipl_step ipl6[] = {
	{ IPL_DEF(host_bootloader),              6,  1,  false, true  },
	{ IPL_DEF(host_setup),                   6,  2,  false, true  },
	{ IPL_DEF(host_istep_enable),            6,  3,  false, true  },
	{ IPL_DEF(host_init_bmc_pcie),           6,  4,  false, true  },
	{ IPL_DEF(host_init_fsi),                6,  5,  false, true  },
	{ IPL_DEF(host_set_ipl_parms),           6,  6,  false, true  },
	{ IPL_DEF(host_discover_targets),        6,  7,  false, true  },
	{ IPL_DEF(host_update_master_tpm),       6,  8,  false, true  },
	{ IPL_DEF(host_gard),                    6,  9,  false, true  },
	{ IPL_DEF(host_revert_sbe_mcs_setup),    6, 10,  true,  true  },
	{ IPL_DEF(host_start_occ_xstop_handler), 6, 11,  false, true  },
	{ IPL_DEF(host_voltage_config),          6, 12,  true,  true  },
	{ NULL, NULL, -1, -1, false, false },
};

__attribute__((constructor))
static void ipl_register_ipl6(void)
{
	ipl_register(6, ipl6, ipl_pre6);
}
