extern "C" {
#include <stdio.h>

#include <libpdbg.h>
}

#include "libipl.H"
#include "libipl_internal.H"

#include <ekb/chips/p9/procedures/hwp/perv/p9_check_slave_sbe_seeprom_complete.H>
#include <ekb/chips/p9/procedures/hwp/nest/p9_fbc_eff_config.H>
#include <ekb/chips/p9/procedures/hwp/nest/p9_fbc_eff_config_links.H>
#include <ekb/chips/p9/procedures/hwp/nest/p9_attr_update.H>
#include <ekb/chips/p9/procedures/hwp/nest/p9_chiplet_fabric_scominit.H>
#include <ekb/chips/p9/procedures/hwp/io/p9_io_xbus_scominit.H>
#include <ekb/chips/p9/procedures/hwp/perv/p9_xbus_enable_ridi.H>

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
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib) {

		/* This procedure is listed in istep list but is a noop in the ekb */
		p9_check_slave_sbe_seeprom_complete(pib);
	}

	return 0;
}

static int ipl_host_attnlisten_proc(void)
{
	return -1;
}

static int ipl_host_p9_fbc_eff_config(void)
{
	p9_fbc_eff_config();

	return 0;
}

static int ipl_host_p9_eff_config_links(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib)
		p9_fbc_eff_config_links(pib, SMP_ACTIVATE_PHASE1, 1, 0);

	return 0;
}

static int ipl_proc_attr_update(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib)
		p9_attr_update(pib);

	return 0;
}

static int ipl_proc_chiplet_scominit(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib)
		p9_chiplet_fabric_scominit(pib);

	return 0;
}

static int ipl_proc_xbus_scominit(void)
{
	struct pdbg_target *xbus, *xbusses[2];
	int i = 0;

	pdbg_for_each_class_target("xbus", xbus) {
		assert(i < 2);
		xbusses[i] = xbus;
		i++;
	}

	assert(i == 2);
	for(i = 0; i < 2; i++)
		printf("Found XBUS %s\n", pdbg_target_path(xbusses[i]));

	p9_io_xbus_scominit(xbusses[1], xbusses[0], 0);
	p9_io_xbus_scominit(xbusses[1], xbusses[0], 1);
	p9_io_xbus_scominit(xbusses[0], xbusses[1], 0);
	p9_io_xbus_scominit(xbusses[0], xbusses[1], 1);

	return 0;
}

static int ipl_proc_xbus_enable_ridi(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib)
		p9_xbus_enable_ridi(pib);

	return 0;
}

static struct ipl_step ipl8[] = {
	{ IPL_DEF(host_slave_sbe_config),                 8,  1,  false, true  },
	{ IPL_DEF(host_setup_sbe),                        8,  2,  false, true  },
	{ IPL_DEF(host_cbs_start),                        8,  3,  false, true  },
	{ IPL_DEF(proc_check_slave_sbe_seeprom_complete), 8,  4,  true,  true  },
	{ IPL_DEF(host_attnlisten_proc),                  8,  5,  false, true  },
	{ IPL_DEF(host_p9_fbc_eff_config),                8,  6,  true,  true  },
	{ IPL_DEF(host_p9_eff_config_links),              8,  7,  true,  true  },
	{ IPL_DEF(proc_attr_update),                      8,  8,  true,  true  },
	{ IPL_DEF(proc_chiplet_scominit),                 8,  9,  true,  true  },
	{ IPL_DEF(proc_xbus_scominit),                    8, 10,  true,  true  },
	{ IPL_DEF(proc_xbus_enable_ridi),                 8, 11,  true,  true  },
	{ NULL, NULL, -1, -1, false, false },
};

__attribute__((constructor))
static void ipl_register_ipl8(void)
{
	ipl_register(8, ipl8, ipl_pre8);
}
