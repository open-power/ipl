extern "C" {
#include <stdio.h>

#include <libpdbg.h>

#include "libipl.h"
#include "libipl_internal.h"
}

static void ipl_pre17(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib)
		pdbg_target_probe(pib);
}

static int ipl_collect_drawers(void)
{
	return -1;
}

static int ipl_micro_update(void)
{
	return -1;
}

static int ipl_proc_psiinit(void)
{
	return -1;
}

static int ipl_psi_diag(void)
{
	return -1;
}

static struct ipl_step ipl17[] = {
	{ IPL_DEF(collect_drawers), 17,  1,  false, true  },
	{ IPL_DEF(micro_update),    17,  2,  false, true  },
	{ IPL_DEF(proc_psiinit),    17,  3,  false, true  },
	{ IPL_DEF(psi_diag),        17,  4,  false, true  },
	{ NULL, NULL, -1, -1, false, false },
};

__attribute__((constructor))
static void ipl_register_ipl17(void)
{
	ipl_register(17, ipl17, ipl_pre17);
}
