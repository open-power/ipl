extern "C" {
#include <stdio.h>

#include <libpdbg.h>

#include "libipl.h"
#include "libipl_internal.h"
}

static void ipl_pre8(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib)
		pdbg_target_probe(pib);
}

static int ipl_host_slave_sbe_config(void)
{
	return -1;
}

static int ipl_host_setup_sbe(void)
{
	return -1;
}

static int ipl_host_cbs_start(void)
{
	return -1;
}

static int ipl_proc_check_slave_sbe_seeprom_complete(void)
{
	return -1;
}

static int ipl_host_attnlisten_proc(void)
{
	return -1;
}

static int ipl_proc_fbc_eff_config(void)
{
	return -1;
}

static int ipl_proc_eff_config_links(void)
{
	return -1;
}

static int ipl_proc_attr_update(void)
{
	return -1;
}

static int ipl_proc_chiplet_fabric_scominit(void)
{
	return -1;
}

static int ipl_host_set_voltages(void)
{
	return -1;
}

static int ipl_proc_io_scominit(void)
{
	return -1;
}

static int ipl_proc_load_ioppe(void)
{
	return -1;
}

static int ipl_proc_init_ioppe(void)
{
	return -1;
}

static int ipl_proc_iohs_enable_ridi(void)
{
	return -1;
}

static struct ipl_step ipl8[] = {
	{ IPL_DEF(host_slave_sbe_config),                   8,  1,  false, true  },
	{ IPL_DEF(host_setup_sbe),                          8,  2,  false, true  },
	{ IPL_DEF(host_cbs_start),                          8,  3,  false, true  },
	{ IPL_DEF(proc_check_slave_sbe_seeprom_complete),   8,  4,  true,  true  },
	{ IPL_DEF(host_attnlisten_proc),                    8,  5,  false, true  },
	{ IPL_DEF(proc_fbc_eff_config),                     8,  6,  true,  true  },
	{ IPL_DEF(proc_eff_config_links),                   8,  7,  true,  true  },
	{ IPL_DEF(proc_attr_update),                        8,  8,  true,  true  },
	{ IPL_DEF(proc_chiplet_fabric_scominit),            8,  9,  true,  true  },
	{ IPL_DEF(host_set_voltages),                       8, 10,  true,  true  },
	{ IPL_DEF(proc_io_scominit),                        8, 11,  true,  true  },
	{ IPL_DEF(proc_load_ioppe),                         8, 12,  true,  true  },
	{ IPL_DEF(proc_init_ioppe),                         8, 13,  true,  true  },
	{ IPL_DEF(proc_iohs_enable_ridi),                   8, 14,  true,  true  },
	{ NULL, NULL, -1, -1, false, false },
};

__attribute__((constructor))
static void ipl_register_ipl8(void)
{
	ipl_register(8, ipl8, ipl_pre8);
}
