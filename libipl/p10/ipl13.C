extern "C" {
#include <stdio.h>

#include <libpdbg.h>
}

#include "libipl.H"
#include "libipl_internal.H"
#include "common.H"

static void ipl_pre13(void)
{
	ipl_pre();
}

static int ipl_mss_scominit(void)
{
	return ipl_istep_via_hostboot(13, 1);
}

static int ipl_mss_draminit(void)
{
	return ipl_istep_via_hostboot(13, 2);
}

static int ipl_mss_draminit_mc(void)
{
	return ipl_istep_via_hostboot(13, 3);
}

static struct ipl_step ipl13[] = {
    {IPL_DEF(mss_scominit), 13, 1, true, true},
    {IPL_DEF(mss_draminit), 13, 2, true, true},
    {IPL_DEF(mss_draminit_mc), 13, 3, true, true},
    {NULL, NULL, -1, -1, false, false},
};

__attribute__((constructor)) static void ipl_register_ipl13(void)
{
	ipl_register(13, ipl13, ipl_pre13);
}
