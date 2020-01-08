extern "C" {
#include <stdio.h>

#include <libpdbg.h>

#include "libipl.h"
#include "libipl_internal.h"
}

#include "common.H"

static void ipl_pre7(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib) {
		if (ipl_mode() == IPL_DEFAULT && pdbg_target_index(pib) != 0)
			continue;

		pdbg_target_probe(pib);
	}
}

static int ipl_host_mss_attr_cleanup(void)
{
	return ipl_istep_via_hostboot(7, 1);
}

static int ipl_mss_volt(void)
{
	return ipl_istep_via_hostboot(7, 2);
}

static int ipl_mss_freq(void)
{
	return ipl_istep_via_hostboot(7, 3);
}

static int ipl_mss_eff_config(void)
{
	return ipl_istep_via_hostboot(7, 4);
}

static int ipl_mss_attr_update(void)
{
	return ipl_istep_via_hostboot(7, 5);
}

static struct ipl_step ipl7[] = {
	{ IPL_DEF(host_mss_attr_cleanup),   7,  1,  true,  true  },
	{ IPL_DEF(mss_volt),                7,  2,  true,  true  },
	{ IPL_DEF(mss_freq),                7,  3,  true,  true  },
	{ IPL_DEF(mss_eff_config),          7,  4,  true,  true  },
	{ IPL_DEF(mss_attr_update),         7,  5,  true,  true  },
	{ NULL, NULL, -1, -1, false, false },
};

__attribute__((constructor))
static void ipl_register_ipl7(void)
{
	ipl_register(7, ipl7, ipl_pre7);
}
