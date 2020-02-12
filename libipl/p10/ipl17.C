extern "C" {
#include <stdio.h>

#include <libpdbg.h>
}

#include "libipl.H"
#include "libipl_internal.H"
#include "common.H"

static void ipl_pre17(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib) {
		if (ipl_mode() == IPL_DEFAULT && pdbg_target_index(pib) != 0)
			continue;

		pdbg_target_probe(pib);
	}
}

static int ipl_collect_drawers(void)
{
	return ipl_istep_via_hostboot(17, 1);
}

static int ipl_proc_psiinit(void)
{
	return ipl_istep_via_hostboot(17, 2);
}

static int ipl_psi_diag(void)
{
	return ipl_istep_via_hostboot(17, 3);
}

static struct ipl_step ipl17[] = {
	{ IPL_DEF(collect_drawers),   17,  1,  true,  true  },
	{ IPL_DEF(proc_psiinit),      17,  2,  true,  true  },
	{ IPL_DEF(psi_diag),          17,  3,  true,  true  },
	{ NULL, NULL, -1, -1, false, false },
};

__attribute__((constructor))
static void ipl_register_ipl17(void)
{
	ipl_register(17, ipl17, ipl_pre17);
}
