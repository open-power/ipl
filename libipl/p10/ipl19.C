extern "C" {
#include <stdio.h>

#include <libpdbg.h>

#include "libipl.h"
#include "libipl_internal.h"
}

#include "common.H"

static void ipl_pre19(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib) {
		if (ipl_mode() == IPL_DEFAULT && pdbg_target_index(pib) != 0)
			continue;

		pdbg_target_probe(pib);
	}
}

static int ipl_prep_host(void)
{
	return ipl_istep_via_hostboot(19, 1);
}

static struct ipl_step ipl19[] = {
	{ IPL_DEF(prep_host),   19,  1,  true,  true  },
	{ NULL, NULL, -1, -1, false, false },
};

__attribute__((constructor))
static void ipl_register_ipl19(void)
{
	ipl_register(19, ipl19, ipl_pre19);
}
