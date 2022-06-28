extern "C" {
#include <stdio.h>

#include <libpdbg.h>
}

#include "libipl.H"
#include "libipl_internal.H"
#include "common.H"

static void ipl_pre15(void)
{
	ipl_pre();
}

static int ipl_host_build_stop_image(void)
{
	return ipl_istep_via_hostboot(15, 1);
}

static int ipl_proc_set_homer_bar(void)
{
	return ipl_istep_via_hostboot(15, 2);
}

static int ipl_host_establish_ec_chiplet(void)
{
	return ipl_istep_via_hostboot(15, 3);
}

static int ipl_host_start_stop_engine(void)
{
	return ipl_istep_via_hostboot(15, 4);
}

static struct ipl_step ipl15[] = {
    {IPL_DEF(host_build_stop_image), 15, 1, true, true},
    {IPL_DEF(proc_set_homer_bar), 15, 2, true, true},
    {IPL_DEF(host_establish_ec_chiplet), 15, 3, true, true},
    {IPL_DEF(host_start_stop_engine), 15, 4, true, true},
    {NULL, NULL, -1, -1, false, false},
};

__attribute__((constructor)) static void ipl_register_ipl15(void)
{
	ipl_register(15, ipl15, ipl_pre15);
}
