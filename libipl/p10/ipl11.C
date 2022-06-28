extern "C" {
#include <stdio.h>

#include <libpdbg.h>
}

#include "libipl.H"
#include "libipl_internal.H"
#include "common.H"

static void ipl_pre11(void)
{
	ipl_pre();
}

static int ipl_host_prd_hwreconfig(void)
{
	return ipl_istep_via_hostboot(11, 1);
}

static int ipl_host_set_mem_volt(void)
{
	return ipl_istep_via_hostboot(11, 2);
}

static int ipl_proc_ocmb_enable(void)
{
	return ipl_istep_via_hostboot(11, 3);
}

static int ipl_ocmb_check_for_ready(void)
{
	return ipl_istep_via_hostboot(11, 4);
}

static struct ipl_step ipl11[] = {
    {IPL_DEF(host_prd_hwreconfig), 11, 1, true, true},
    {IPL_DEF(host_set_mem_volt), 11, 2, true, true},
    {IPL_DEF(proc_ocmb_enable), 11, 3, true, true},
    {IPL_DEF(ocmb_check_for_ready), 11, 4, true, true},
    {NULL, NULL, -1, -1, false, false},
};

__attribute__((constructor)) static void ipl_register_ipl11(void)
{
	ipl_register(11, ipl11, ipl_pre11);
}
