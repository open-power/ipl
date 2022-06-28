extern "C" {
#include <stdio.h>

#include <libpdbg.h>
}

#include "libipl.H"
#include "libipl_internal.H"
#include "common.H"

static void ipl_pre19(void)
{
	ipl_pre();
}

static int ipl_prep_host(void)
{
	return -1;
}

static struct ipl_step ipl19[] = {
    {IPL_DEF(prep_host), 19, 1, true, true},
    {NULL, NULL, -1, -1, false, false},
};

__attribute__((constructor)) static void ipl_register_ipl19(void)
{
	ipl_register(19, ipl19, ipl_pre19);
}
