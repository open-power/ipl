extern "C" {
#include <stdio.h>

#include <libpdbg.h>
}

#include "libipl.H"
#include "libipl_internal.H"
#include "common.H"

static void ipl_pre2(void)
{
	ipl_pre();
}

static int ipl_proc_sbe_ld_image(void)
{
	return -1;
}

static int ipl_proc_sbe_attr_setup(void)
{
	return ipl_istep_via_sbe(2, 2);
}

static int ipl_proc_sbe_tp_dpll_bypass(void)
{
	return ipl_istep_via_sbe(2, 3);
}

static int ipl_proc_sbe_tp_chiplet_reset(void)
{
	return ipl_istep_via_sbe(2, 4);
}

static int ipl_proc_sbe_tp_gptr_time_initf(void)
{
	return ipl_istep_via_sbe(2, 5);
}

static int ipl_proc_sbe_dft_probe_setup_1(void)
{
	return ipl_istep_via_sbe(2, 6);
}

static int ipl_proc_sbe_npll_initf(void)
{
	return ipl_istep_via_sbe(2, 7);
}

static int ipl_proc_sbe_rcs_setup(void)
{
	return ipl_istep_via_sbe(2, 8);
}

static int ipl_proc_sbe_tp_switch_gears(void)
{
	return ipl_istep_via_sbe(2, 9);
}

static int ipl_proc_sbe_npll_setup(void)
{
	return ipl_istep_via_sbe(2, 10);
}

static int ipl_proc_sbe_tp_repr_initf(void)
{
	return ipl_istep_via_sbe(2, 11);
}

static int ipl_proc_sbe_setup_tp_abist(void)
{
	return ipl_istep_via_sbe(2, 12);
}

static int ipl_proc_sbe_tp_arrayinit(void)
{
	return ipl_istep_via_sbe(2, 13);
}

static int ipl_proc_sbe_tp_initf(void)
{
	return ipl_istep_via_sbe(2, 14);
}

static int ipl_proc_sbe_dft_probe_setup_2(void)
{
	return ipl_istep_via_sbe(2, 15);
}

static int ipl_proc_sbe_tp_chiplet_init(void)
{
	return ipl_istep_via_sbe(2, 16);
}

static struct ipl_step ipl2[] = {
    {IPL_DEF(proc_sbe_ld_image), 2, 1, true, true},
    {IPL_DEF(proc_sbe_attr_setup), 2, 2, true, true},
    {IPL_DEF(proc_sbe_tp_dpll_bypass), 2, 3, true, true},
    {IPL_DEF(proc_sbe_tp_chiplet_reset), 2, 4, true, true},
    {IPL_DEF(proc_sbe_tp_gptr_time_initf), 2, 5, true, true},
    {IPL_DEF(proc_sbe_dft_probe_setup_1), 2, 6, true, true},
    {IPL_DEF(proc_sbe_npll_initf), 2, 7, true, true},
    {IPL_DEF(proc_sbe_rcs_setup), 2, 8, true, true},
    {IPL_DEF(proc_sbe_tp_switch_gears), 2, 9, true, true},
    {IPL_DEF(proc_sbe_npll_setup), 2, 10, true, true},
    {IPL_DEF(proc_sbe_tp_repr_initf), 2, 11, true, true},
    {IPL_DEF(proc_sbe_setup_tp_abist), 2, 12, true, true},
    {IPL_DEF(proc_sbe_tp_arrayinit), 2, 13, true, true},
    {IPL_DEF(proc_sbe_tp_initf), 2, 14, true, true},
    {IPL_DEF(proc_sbe_dft_probe_setup_2), 2, 15, true, true},
    {IPL_DEF(proc_sbe_tp_chiplet_init), 2, 16, true, true},
    {NULL, NULL, -1, -1, false, false},
};

__attribute__((constructor)) static void ipl_register_ipl2(void)
{
	ipl_register(2, ipl2, ipl_pre2);
}
