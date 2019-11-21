extern "C" {
#include <stdio.h>

#include <libpdbg.h>

#include "libipl.h"
#include "libipl_internal.h"
}

static void ipl_pre15(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib)
		pdbg_target_probe(pib);
}

static int ipl_host_build_stop_image(void)
{
	return -1;
}

static int ipl_proc_set_homer_bar(void)
{
	return -1;
}

static int ipl_host_establish_ec_chiplet(void)
{
	return -1;
}

static int ipl_host_start_stop_engine(void)
{
	return -1;
}

static struct ipl_step ipl15[] = {
	{ IPL_DEF(host_build_stop_image),       15,  1,  true,  true  },
	{ IPL_DEF(proc_set_homer_bar),          15,  2,  true,  true  },
	{ IPL_DEF(host_establish_ec_chiplet),   15,  3,  true,  true  },
	{ IPL_DEF(host_start_stop_engine),      15,  4,  true,  true  },
	{ NULL, NULL, -1, -1, false, false },
};

__attribute__((constructor))
static void ipl_register_ipl15(void)
{
	ipl_register(15, ipl15, ipl_pre15);
}
