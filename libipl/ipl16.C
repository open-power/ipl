extern "C" {
#include <stdio.h>

#include <libpdbg.h>

#include "libipl.h"
#include "libipl_internal.h"
}

static void ipl_pre16(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib)
		pdbg_target_probe(pib);
}

static int ipl_host_activate_master(void)
{
	return -1;
}

static int ipl_host_activate_slave_cores(void)
{
	return -1;
}

static int ipl_host_secure_rng(void)
{
	return -1;
}

static int ipl_mss_scrub(void)
{
	return -1;
}

static int ipl_host_load_io_ppe(void)
{
	return -1;
}

static int ipl_host_ipl_complete(void)
{
	return -1;
}

static struct ipl_step ipl16[] = {
	{ IPL_DEF(host_activate_master),      16,  1,  true,  true  },
	{ IPL_DEF(host_activate_slave_cores), 16,  2,  true,  true  },
	{ IPL_DEF(host_secure_rng),           16,  3,  true,  true  },
	{ IPL_DEF(mss_scrub),                 16,  4,  false, true  },
	{ IPL_DEF(host_load_io_ppe),          16,  5,  false, true  },
	{ IPL_DEF(host_ipl_complete),         16,  6,  false, true  },
	{ NULL, NULL, -1, -1, false, false },
};

__attribute__((constructor))
static void ipl_register_ipl16(void)
{
	ipl_register(16, ipl16, ipl_pre16);
}
