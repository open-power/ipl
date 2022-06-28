extern "C" {
#include <stdio.h>

#include <libpdbg.h>
}

#include "libipl.H"
#include "libipl_internal.H"

#include "common.H"

static void ipl_pre4(void)
{
	ipl_pre();
}

static int ipl_proc_hcd_cache_poweron(void)
{
	return ipl_istep_via_sbe(4, 1);
}

static int ipl_proc_hcd_cache_reset(void)
{
	return ipl_istep_via_sbe(4, 2);
}

static int ipl_proc_hcd_cache_gptr_time_initf(void)
{
	return ipl_istep_via_sbe(4, 3);
}

static int ipl_proc_hcd_cache_repair_initf(void)
{
	return ipl_istep_via_sbe(4, 4);
}

static int ipl_proc_hcd_cache_arrayinit(void)
{
	return ipl_istep_via_sbe(4, 5);
}

static int ipl_proc_hcd_cache_initf(void)
{
	return ipl_istep_via_sbe(4, 6);
}

static int ipl_proc_hcd_cache_startclocks(void)
{
	return ipl_istep_via_sbe(4, 7);
}

static int ipl_proc_hcd_cache_scominit(void)
{
	return ipl_istep_via_sbe(4, 8);
}

static int ipl_proc_hcd_cache_scom_customize(void)
{
	return ipl_istep_via_sbe(4, 9);
}

static int ipl_proc_hcd_cache_ras_runtime_scom(void)
{
	return ipl_istep_via_sbe(4, 10);
}

static int ipl_proc_hcd_core_poweron(void)
{
	return ipl_istep_via_sbe(4, 11);
}

static int ipl_proc_hcd_core_reset(void)
{
	return ipl_istep_via_sbe(4, 12);
}

static int ipl_proc_hcd_core_gptr_time_initf(void)
{
	return ipl_istep_via_sbe(4, 13);
}

static int ipl_proc_hcd_core_repair_initf(void)
{
	return ipl_istep_via_sbe(4, 14);
}

static int ipl_proc_hcd_core_arrayinit(void)
{
	return ipl_istep_via_sbe(4, 15);
}

static int ipl_proc_hcd_core_initf(void)
{
	return ipl_istep_via_sbe(4, 16);
}

static int ipl_proc_hcd_core_startclocks(void)
{
	return ipl_istep_via_sbe(4, 17);
}

static int ipl_proc_hcd_core_scominit(void)
{
	return ipl_istep_via_sbe(4, 18);
}

static int ipl_proc_hcd_core_scom_customize(void)
{
	return ipl_istep_via_sbe(4, 19);
}

static int ipl_proc_hcd_core_ras_runtime_scom(void)
{
	return ipl_istep_via_sbe(4, 20);
}

static struct ipl_step ipl4[] = {
    {IPL_DEF(proc_hcd_cache_poweron), 4, 1, true, true},
    {IPL_DEF(proc_hcd_cache_reset), 4, 2, true, true},
    {IPL_DEF(proc_hcd_cache_gptr_time_initf), 4, 3, true, true},
    {IPL_DEF(proc_hcd_cache_repair_initf), 4, 4, true, true},
    {IPL_DEF(proc_hcd_cache_arrayinit), 4, 5, true, true},
    {IPL_DEF(proc_hcd_cache_initf), 4, 6, true, true},
    {IPL_DEF(proc_hcd_cache_startclocks), 4, 7, true, true},
    {IPL_DEF(proc_hcd_cache_scominit), 4, 8, true, true},
    {IPL_DEF(proc_hcd_cache_scom_customize), 4, 9, true, true},
    {IPL_DEF(proc_hcd_cache_ras_runtime_scom), 4, 10, true, true},
    {IPL_DEF(proc_hcd_core_poweron), 4, 11, true, true},
    {IPL_DEF(proc_hcd_core_reset), 4, 12, true, true},
    {IPL_DEF(proc_hcd_core_gptr_time_initf), 4, 13, true, true},
    {IPL_DEF(proc_hcd_core_repair_initf), 4, 14, true, true},
    {IPL_DEF(proc_hcd_core_arrayinit), 4, 15, true, true},
    {IPL_DEF(proc_hcd_core_initf), 4, 16, true, true},
    {IPL_DEF(proc_hcd_core_startclocks), 4, 17, true, true},
    {IPL_DEF(proc_hcd_core_scominit), 4, 18, true, true},
    {IPL_DEF(proc_hcd_core_scom_customize), 4, 19, true, true},
    {IPL_DEF(proc_hcd_core_ras_runtime_scom), 4, 20, true, true},
    {NULL, NULL, -1, -1, false, false},
};

__attribute__((constructor)) static void ipl_register_ipl4(void)
{
	ipl_register(4, ipl4, ipl_pre4);
}
