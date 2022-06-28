extern "C" {
#include <stdio.h>

#include <libpdbg.h>
}

#include "libipl.H"
#include "libipl_internal.H"
#include "common.H"

static void ipl_pre5(void)
{
	ipl_pre();
}

static int ipl_proc_sbe_load_bootloader(void)
{
	return ipl_istep_via_sbe(5, 1);
}

static int ipl_proc_sbe_core_spr_setup(void)
{
	return ipl_istep_via_sbe(5, 2);
}

static int ipl_proc_sbe_instruct_start(void)
{
	return ipl_istep_via_sbe(5, 3);
}

static struct ipl_step ipl5[] = {
    {IPL_DEF(proc_sbe_load_bootloader), 5, 1, true, true},
    {IPL_DEF(proc_sbe_core_spr_setup), 5, 2, true, true},
    {IPL_DEF(proc_sbe_instruct_start), 5, 3, true, true},
    {NULL, NULL, -1, -1, false, false},
};

__attribute__((constructor)) static void ipl_register_ipl5(void)
{
	ipl_register(5, ipl5, ipl_pre5);
}
