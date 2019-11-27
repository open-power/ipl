extern "C" {
#include <stdio.h>

#include <libpdbg.h>

#include "libipl.h"
#include "libipl_internal.h"
}

static void ipl_pre1(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib) {
		if (ipl_mode() == IPL_DEFAULT && pdbg_target_index(pib) != 0)
			continue;

		pdbg_target_probe(pib);
	}
}

static int ipl_proc_sbe_enable_seeprom(void)
{
	return -1;
}

static int ipl_proc_sbe_pib_init(void)
{
	return -1;
}

static int ipl_proc_sbe_measure(void)
{
	return -1;
}

static struct ipl_step ipl1[] = {
	{ IPL_DEF(proc_sbe_enable_seeprom),   1,  1,  true,  true, },
	{ IPL_DEF(proc_sbe_pib_init),         1,  2,  true,  true  },
	{ IPL_DEF(proc_sbe_measure),          1,  3,  true,  true  },
	{ NULL, NULL, -1, -1, false, false },
};

__attribute__((constructor))
static void ipl_register_ipl1(void)
{
	ipl_register(1, ipl1, ipl_pre1);
}
