extern "C" {
#include <stdio.h>

#include <libpdbg.h>

#include "libipl.h"
#include "libipl_internal.h"
}

static void ipl_pre18(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib)
		pdbg_target_probe(pib);
}

static int ipl_sys_proc_chiplet_scominit(void)
{
	return -1;
}

static int ipl_sys_proc_abus_scominit(void)
{
	return -1;
}

static int ipl_sys_proc_scomoverride_chiplets(void)
{
	return -1;
}

static int ipl_sys_fabric_erepair(void)
{
	return -1;
}

static int ipl_sys_fabric_pre_trainadv(void)
{
	return -1;
}

static int ipl_sys_fabric_io_dccal(void)
{
	return -1;
}

static int ipl_sys_fabric_io_run_training(void)
{
	return -1;
}

static int ipl_sys_fabric_post_trainadv(void)
{
	return -1;
}

static int ipl_sys_proc_smp_link_layer(void)
{
	return -1;
}

static int ipl_host_coalesce_host(void)
{
	return -1;
}

static int ipl_proc_tod_setup(void)
{
	return -1;
}

static int ipl_proc_tod_init(void)
{
	return -1;
}

static int ipl_cec_ipl_complete(void)
{
	return -1;
}

static int ipl_startprd_system(void)
{
	return -1;
}

static int ipl_attn_listenall(void)
{
	return -1;
}

static struct ipl_step ipl18[] = {
	{ IPL_DEF(sys_proc_chiplet_scominit),      18,  1,  true,  true  },
	{ IPL_DEF(sys_proc_abus_scominit),         18,  2,  true,  true  },
	{ IPL_DEF(sys_proc_scomoverride_chiplets), 18,  3,  true,  true  },
	{ IPL_DEF(sys_fabric_erepair),             18,  4,  true,  true  },
	{ IPL_DEF(sys_fabric_pre_trainadv),        18,  5,  true,  true  },
	{ IPL_DEF(sys_fabric_io_dccal),            18,  6,  true,  true  },
	{ IPL_DEF(sys_fabric_io_run_training),     18,  7,  true,  true  },
	{ IPL_DEF(sys_fabric_post_trainadv),       18,  8,  true,  true  },
	{ IPL_DEF(sys_proc_smp_link_layer),        18,  9,  true,  true  },
	{ IPL_DEF(host_coalesce_host),             18, 10,  false, true  },
	{ IPL_DEF(proc_tod_setup),                 18, 11,  true,  true  },
	{ IPL_DEF(proc_tod_init),                  18, 12,  true,  true  },
	{ IPL_DEF(cec_ipl_complete),               18, 13,  false, true  },
	{ IPL_DEF(startprd_system),                18, 14,  false, true  },
	{ IPL_DEF(attn_listenall),                 18, 15,  false, true  },
	{ NULL, NULL, -1, -1, false, false },
};

__attribute__((constructor))
static void ipl_register_ipl18(void)
{
	ipl_register(18, ipl18, ipl_pre18);
}
