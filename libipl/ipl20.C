extern "C" {
#include <stdio.h>

#include <libpdbg.h>

#include "libipl.h"
#include "libipl_internal.h"
}

static void ipl_pre20(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib)
		pdbg_target_probe(pib);
}

static int ipl_host_load_payload(void)
{
	return -1;
}

static int ipl_host_load_complete(void)
{
	return -1;
}

static struct ipl_step ipl20[] = {
	{ IPL_DEF(host_load_payload),  20,  1,  false, true  },
	{ IPL_DEF(host_load_complete), 20,  2,  false, true  },
	{ NULL, NULL, -1, -1, false, false },
};

__attribute__((constructor))
static void ipl_register_ipl20(void)
{
	ipl_register(20, ipl20, ipl_pre20);
}
