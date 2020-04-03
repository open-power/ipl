extern "C" {
#include <stdio.h>

#include <libpdbg.h>
}

#include "libipl.H"
#include "libipl_internal.H"
#include "common.H"

static void ipl_pre17(void)
{
	ipl_pre();
}

static struct ipl_step ipl17[] = {
	{ NULL, NULL, -1, -1, false, false },
};

__attribute__((constructor))
static void ipl_register_ipl17(void)
{
	ipl_register(17, ipl17, ipl_pre17);
}
