extern "C" {
#include <stdio.h>

#include <libpdbg.h>

#include "libipl.h"
#include "libipl_internal.h"
}

static void ipl_pre12(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib)
		pdbg_target_probe(pib);
}

static int ipl_mss_getecid(void)
{
	return -1;
}

static int ipl_omi_attr_update(void)
{
	return -1;
}

static int ipl_proc_omi_scominit(void)
{
	return -1;
}

static int ipl_ocmb_omi_scominit(void)
{
	return -1;
}

static int ipl_omi_pre_trainadv(void)
{
	return -1;
}

static int ipl_omi_setup(void)
{
	return -1;
}

static int ipl_omi_io_run_training(void)
{
	return -1;
}

static int ipl_omi_train_check(void)
{
	return -1;
}

static int ipl_omi_post_trainadv(void)
{
	return -1;
}

static int ipl_host_attnlisten_memb(void)
{
	return -1;
}

static int ipl_host_omi_init(void)
{
	return -1;
}

static int ipl_update_omi_firmware(void)
{
	return -1;
}

static struct ipl_step ipl12[] = {
	{ IPL_DEF(mss_getecid),            12,  1,  true,  true  },
	{ IPL_DEF(omi_attr_update),        12,  2,  true,  true  },
	{ IPL_DEF(proc_omi_scominit),      12,  3,  true,  true  },
	{ IPL_DEF(ocmb_omi_scominit),      12,  4,  true,  true  },
	{ IPL_DEF(omi_pre_trainadv),       12,  5,  true,  true  },
	{ IPL_DEF(omi_setup),              12,  6,  true,  true  },
	{ IPL_DEF(omi_io_run_training),    12,  7,  true,  true  },
	{ IPL_DEF(omi_train_check),        12,  8,  true,  true  },
	{ IPL_DEF(omi_post_trainadv),      12,  9,  true,  true  },
	{ IPL_DEF(host_attnlisten_memb),   12, 10,  true,  true  },
	{ IPL_DEF(host_omi_init),          12, 11,  true,  true  },
	{ IPL_DEF(update_omi_firmware),    12, 12,  true,  true  },
	{ NULL, NULL, -1, -1, false, false },
};

__attribute__((constructor))
static void ipl_register_ipl12(void)
{
	ipl_register(12, ipl12, ipl_pre12);
}
