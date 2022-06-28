extern "C" {
#include <stdio.h>

#include <libpdbg.h>
}

#include "libipl.H"
#include "libipl_internal.H"
#include "common.H"

static void ipl_pre20(void)
{
	ipl_pre();
}

static int ipl_host_load_payload(void)
{
	return ipl_istep_via_hostboot(20, 1);
}

static int ipl_host_load_complete(void)
{
	return ipl_istep_via_hostboot(20, 2);
}

static struct ipl_step ipl20[] = {
    {IPL_DEF(host_load_payload), 20, 1, true, true},
    {IPL_DEF(host_load_complete), 20, 2, true, true},
    {NULL, NULL, -1, -1, false, false},
};

__attribute__((constructor)) static void ipl_register_ipl20(void)
{
	ipl_register(20, ipl20, ipl_pre20);
}
