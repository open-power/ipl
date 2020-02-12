extern "C" {
#include <stdio.h>

#include <libpdbg.h>
}

#include "libipl.H"
#include "libipl_internal.H"

static void ipl_pre5(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib)
		pdbg_target_probe(pib);
}

static int ipl_proc_sbe_load_bootloader(void)
{
	struct pdbg_target *pib;
	int rc = 0;

	pdbg_for_each_class_target("pib", pib)
		rc |= sbe_istep(pib, 5, 1);

	return rc;
}

static int ipl_proc_sbe_instruct_start(void)
{
	struct pdbg_target *pib;
	int rc = 0;

	pdbg_for_each_class_target("pib", pib)
		rc |= sbe_istep(pib, 5, 1);

	return rc;
}

static struct ipl_step ipl5[] = {
	{ IPL_DEF(proc_sbe_load_bootloader), 5,  1,  true,  true  },
	{ IPL_DEF(proc_sbe_instruct_start),  5,  2,  true,  true  },
	{ NULL, NULL, -1, -1, false, false },
};

__attribute__((constructor))
static void ipl_register_ipl5(void)
{
	ipl_register(5, ipl5, ipl_pre5);
}
