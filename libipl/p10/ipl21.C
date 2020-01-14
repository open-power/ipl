extern "C" {
#include <stdio.h>

#include <libpdbg.h>

#include "libipl.h"
#include "libipl_internal.h"
}

#include "common.H"

static void ipl_pre21(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib) {
		if (ipl_mode() == IPL_DEFAULT && pdbg_target_index(pib) != 0)
			continue;

		pdbg_target_probe(pib);
	}
}

static int ipl_host_micro_update(void)
{
	return ipl_istep_via_hostboot(21, 1);
}

static int ipl_host_runtime_setup(void)
{
	return ipl_istep_via_hostboot(21, 2);
}

static int ipl_host_verify_hdat(void)
{
	return ipl_istep_via_hostboot(21, 3);
}

static int ipl_host_start_payload(void)
{
	return ipl_istep_via_hostboot(21, 4);
}

static int ipl_host_post_start_payload(void)
{
	return ipl_istep_via_hostboot(21, 5);
}

static int ipl_switchbcu(void)
{
	return ipl_istep_via_hostboot(21, 6);
}

static int ipl_completeipl(void)
{
	return ipl_istep_via_hostboot(21, 7);
}

static struct ipl_step ipl21[] = {
	{ IPL_DEF(host_micro_update),         21,  1,  true,  true  },
	{ IPL_DEF(host_runtime_setup),        21,  2,  true,  true  },
	{ IPL_DEF(host_verify_hdat),          21,  3,  true,  true  },
	{ IPL_DEF(host_start_payload),        21,  4,  true,  true  },
	{ IPL_DEF(host_post_start_payload),   21,  5,  true,  true  },
	{ IPL_DEF(switchbcu),                 21,  6,  true,  true  },
	{ IPL_DEF(completeipl),               21,  7,  true,  true  },
	{ NULL, NULL, -1, -1, false, false },
};

__attribute__((constructor))
static void ipl_register_ipl21(void)
{
	ipl_register(21, ipl21, ipl_pre21);
}
