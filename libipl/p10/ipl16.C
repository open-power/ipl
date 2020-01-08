extern "C" {
#include <stdio.h>

#include <libpdbg.h>

#include "libipl.h"
#include "libipl_internal.h"
}

#include "common.H"

static void ipl_pre16(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib) {
		if (ipl_mode() == IPL_DEFAULT && pdbg_target_index(pib) != 0)
			continue;

		pdbg_target_probe(pib);
	}
}

static int ipl_host_activate_master(void)
{
	return ipl_istep_via_hostboot(16, 1);
}

static int ipl_host_activate_slave_cores(void)
{
	return ipl_istep_via_hostboot(16, 2);
}

static int ipl_host_secure_rng(void)
{
	return ipl_istep_via_hostboot(16, 3);
}

static int ipl_mss_scrub(void)
{
	return ipl_istep_via_hostboot(16, 4);
}

static int ipl_host_ipl_complete(void)
{
	return ipl_istep_via_hostboot(16, 5);
}

static struct ipl_step ipl16[] = {
	{ IPL_DEF(host_activate_master),        16,  1,  true,  true  },
	{ IPL_DEF(host_activate_slave_cores),   16,  2,  true,  true  },
	{ IPL_DEF(host_secure_rng),             16,  3,  true,  true  },
	{ IPL_DEF(mss_scrub),                   16,  4,  true,  true  },
	{ IPL_DEF(host_ipl_complete),           16,  5,  true,  true  },
	{ NULL, NULL, -1, -1, false, false },
};

__attribute__((constructor))
static void ipl_register_ipl16(void)
{
	ipl_register(16, ipl16, ipl_pre16);
}
