extern "C" {
#include <stdio.h>
#include <assert.h>

#include <libpdbg.h>
}

#include "libipl.H"
#include "libipl_internal.H"

#include "common.H"

static void ipl_pre9(void)
{
	ipl_pre();
}

static int ipl_proc_io_dccal_done(void)
{
	return ipl_istep_via_hostboot(9, 1);
}

static int ipl_fabric_dl_pre_trainadv(void)
{
	return ipl_istep_via_hostboot(9, 2);
}

static int ipl_fabric_dl_setup_training(void)
{
	return ipl_istep_via_hostboot(9, 3);
}

static int ipl_proc_fabric_link_layer(void)
{
	return ipl_istep_via_hostboot(9, 4);
}

static int ipl_fabric_dl_post_trainadv(void)
{
	return ipl_istep_via_hostboot(9, 5);
}

static int ipl_proc_fabric_iovalid(void)
{
	return ipl_istep_via_hostboot(9, 6);
}

static int ipl_proc_fbc_eff_config_aggregate(void)
{
	return ipl_istep_via_hostboot(9, 7);
}

static struct ipl_step ipl9[] = {
    {IPL_DEF(proc_io_dccal_done), 9, 1, true, true},
    {IPL_DEF(fabric_dl_pre_trainadv), 9, 2, true, true},
    {IPL_DEF(fabric_dl_setup_training), 9, 3, true, true},
    {IPL_DEF(proc_fabric_link_layer), 9, 4, true, true},
    {IPL_DEF(fabric_dl_post_trainadv), 9, 5, true, true},
    {IPL_DEF(proc_fabric_iovalid), 9, 6, true, true},
    {IPL_DEF(proc_fbc_eff_config_aggregate), 9, 7, true, true},
    {NULL, NULL, -1, -1, false, false},
};

__attribute__((constructor)) static void ipl_register_ipl9(void)
{
	ipl_register(9, ipl9, ipl_pre9);
}
