extern "C" {
#include <stdio.h>

#include <libpdbg.h>

#include "libipl.h"
#include "libipl_internal.h"
}

static void ipl_pre19(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib)
		pdbg_target_probe(pib);
}

static struct ipl_step ipl19[] = {
	{ NULL, NULL, -1, -1, false, false },
};

__attribute__((constructor))
static void ipl_register_ipl19(void)
{
	ipl_register(19, ipl19, ipl_pre19);
}
