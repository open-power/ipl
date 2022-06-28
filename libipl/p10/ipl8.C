extern "C" {
#include <stdio.h>

#include <libpdbg.h>
}

#include "libipl.H"
#include "libipl_internal.H"
#include "common.H"

static void ipl_pre8(void)
{
	ipl_pre();
}

static int ipl_host_setup_sbe(void)
{
	return ipl_istep_via_hostboot(8, 1);
}

static int ipl_host_secondary_sbe_config(void)
{
	return ipl_istep_via_hostboot(8, 2);
}

static int ipl_host_cbs_start(void)
{
	return ipl_istep_via_hostboot(8, 3);
}

static int ipl_proc_check_secondary_sbe_seeprom_complete(void)
{
	return ipl_istep_via_hostboot(8, 4);
}

static int ipl_host_attnlisten_proc(void)
{
	return ipl_istep_via_hostboot(8, 5);
}

static int ipl_proc_fbc_eff_config(void)
{
	return ipl_istep_via_hostboot(8, 6);
}

static int ipl_proc_eff_config_links(void)
{
	return ipl_istep_via_hostboot(8, 7);
}

static int ipl_proc_attr_update(void)
{
	return ipl_istep_via_hostboot(8, 8);
}

static int ipl_proc_chiplet_fabric_scominit(void)
{
	return ipl_istep_via_hostboot(8, 9);
}

static int ipl_host_set_voltages(void)
{
	return ipl_istep_via_hostboot(8, 10);
}

static int ipl_proc_io_scominit(void)
{
	return ipl_istep_via_hostboot(8, 11);
}

static int ipl_proc_load_ioppe(void)
{
	return ipl_istep_via_hostboot(8, 12);
}

static int ipl_proc_iohs_enable_ridi(void)
{
	return ipl_istep_via_hostboot(8, 13);
}

static int ipl_proc_init_ioppe(void)
{
	return ipl_istep_via_hostboot(8, 14);
}

static struct ipl_step ipl8[] = {
    {IPL_DEF(host_setup_sbe), 8, 1, true, true},
    {IPL_DEF(host_secondary_sbe_config), 8, 2, true, true},
    {IPL_DEF(host_cbs_start), 8, 3, true, true},
    {IPL_DEF(proc_check_secondary_sbe_seeprom_complete), 8, 4, true, true},
    {IPL_DEF(host_attnlisten_proc), 8, 5, true, true},
    {IPL_DEF(proc_fbc_eff_config), 8, 6, true, true},
    {IPL_DEF(proc_eff_config_links), 8, 7, true, true},
    {IPL_DEF(proc_attr_update), 8, 8, true, true},
    {IPL_DEF(proc_chiplet_fabric_scominit), 8, 9, true, true},
    {IPL_DEF(host_set_voltages), 8, 10, true, true},
    {IPL_DEF(proc_io_scominit), 8, 11, true, true},
    {IPL_DEF(proc_load_ioppe), 8, 12, true, true},
    {IPL_DEF(proc_iohs_enable_ridi), 8, 13, true, true},
    {IPL_DEF(proc_init_ioppe), 8, 14, true, true},
    {NULL, NULL, -1, -1, false, false},
};

__attribute__((constructor)) static void ipl_register_ipl8(void)
{
	ipl_register(8, ipl8, ipl_pre8);
}
