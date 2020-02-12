extern "C" {
#include <stdio.h>

#include <libpdbg.h>
}

#include "libipl.H"
#include "libipl_internal.H"

static void ipl_pre1(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib)
		pdbg_target_probe(pib);
}

static int ipl_proc_sbe_enable_seeprom(void)
{
	return -1;
}

static struct ipl_step ipl1[] = {
	{ IPL_DEF(proc_sbe_enable_seeprom), 1,  1,  true,  true  },
	{ NULL, NULL, -1, -1, false, false },
};

__attribute__((constructor))
static void ipl_register_ipl1(void)
{
	ipl_register(1, ipl1, ipl_pre1);
}
