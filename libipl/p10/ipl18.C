extern "C" {
#include <stdio.h>

#include <libpdbg.h>
}

#include "libipl.H"
#include "libipl_internal.H"
#include "common.H"

static void ipl_pre18(void)
{
	ipl_pre();
}

static int ipl_sys_proc_eff_config_links(void)
{
	return ipl_istep_via_hostboot(18, 1);
}

static int ipl_sys_proc_chiplet_fabric_scominit(void)
{
	return ipl_istep_via_hostboot(18, 2);
}

static int ipl_sys_fabric_dl_pre_trainadv(void)
{
	return ipl_istep_via_hostboot(18, 3);
}

static int ipl_sys_fabric_dl_setup_training(void)
{
	return ipl_istep_via_hostboot(18, 4);
}

static int ipl_sys_proc_fabric_link_layer(void)
{
	return ipl_istep_via_hostboot(18, 5);
}

static int ipl_sys_fabric_dl_post_trainadv(void)
{
	return ipl_istep_via_hostboot(18, 6);
}

static int ipl_sys_proc_fabric_iovalid(void)
{
	return ipl_istep_via_hostboot(18, 7);
}

static int ipl_sys_proc_fbc_eff_config_aggregate(void)
{
	return ipl_istep_via_hostboot(18, 8);
}

static int ipl_proc_tod_setup(void)
{
	return ipl_istep_via_hostboot(18, 9);
}

static int ipl_proc_tod_init(void)
{
	return ipl_istep_via_hostboot(18, 10);
}

static int ipl_cec_ipl_complete(void)
{
	return -1;
}

static int ipl_startprd_system(void)
{
	return -1;
}

static int ipl_attn_listenall(void)
{
	return -1;
}

static struct ipl_step ipl18[] = {
    {IPL_DEF(sys_proc_eff_config_links), 18, 1, true, true},
    {IPL_DEF(sys_proc_chiplet_fabric_scominit), 18, 2, true, true},
    {IPL_DEF(sys_fabric_dl_pre_trainadv), 18, 3, true, true},
    {IPL_DEF(sys_fabric_dl_setup_training), 18, 4, true, true},
    {IPL_DEF(sys_proc_fabric_link_layer), 18, 5, true, true},
    {IPL_DEF(sys_fabric_dl_post_trainadv), 18, 6, true, true},
    {IPL_DEF(sys_proc_fabric_iovalid), 18, 7, true, true},
    {IPL_DEF(sys_proc_fbc_eff_config_aggregate), 18, 8, true, true},
    {IPL_DEF(proc_tod_setup), 18, 9, true, true},
    {IPL_DEF(proc_tod_init), 18, 10, true, true},
    {IPL_DEF(cec_ipl_complete), 18, 11, true, true},
    {IPL_DEF(startprd_system), 18, 12, true, true},
    {IPL_DEF(attn_listenall), 18, 13, true, true},
    {NULL, NULL, -1, -1, false, false},
};

__attribute__((constructor)) static void ipl_register_ipl18(void)
{
	ipl_register(18, ipl18, ipl_pre18);
}
