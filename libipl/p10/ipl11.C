extern "C" {
#include <stdio.h>

#include <libpdbg.h>

#include "libipl.h"
#include "libipl_internal.h"
}

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

static int ipl_host_set_mem_volt(void)
{
	return -1;
}

static int ipl_proc_ocmb_enable(void)
{
	return -1;
}

static int ipl_ocmb_check_for_ready(void)
{
	return -1;
}

static struct ipl_step ipl11[] = {
	{ IPL_DEF(host_prd_hwreconfig),    11,  1,  true,  true  },
	{ IPL_DEF(host_set_mem_volt),      11,  2,  true,  true  },
	{ IPL_DEF(proc_ocmb_enable),       11,  3,  true,  true  },
	{ IPL_DEF(ocmb_check_for_ready),   11,  4,  true,  true  },
	{ NULL, NULL, -1, -1, false, false },
};

__attribute__((constructor))
static void ipl_register_ipl11(void)
{
	ipl_register(11, ipl11, ipl_pre11);
}
