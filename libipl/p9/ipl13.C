extern "C" {
#include <stdio.h>

#include <libpdbg.h>
}

#include "libipl.H"
#include "libipl_internal.H"

static void ipl_pre13(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib) pdbg_target_probe(pib);
}

static int ipl_host_disable_memvolt(void)
{
	return -1;
}

static int ipl_mem_pll_reset(void)
{
	return -1;
}

static int ipl_mem_pll_initf(void)
{
	return -1;
}

static int ipl_mem_pll_setup(void)
{
	return -1;
}

static int ipl_proc_mcs_skewadjust(void)
{
	return -1;
}

static int ipl_mem_startclocks(void)
{
	return -1;
}

static int ipl_host_enable_memvolt(void)
{
	return -1;
}

static int ipl_mss_scominit(void)
{
	return -1;
}

static int ipl_mss_ddr_phy_reset(void)
{
	return -1;
}

static int ipl_mss_draminit(void)
{
	return -1;
}

static int ipl_mss_draminit_training(void)
{
	return -1;
}

static int ipl_mss_draminit_trainadv(void)
{
	return -1;
}

static int ipl_mss_draminit_mc(void)
{
	return -1;
}

static struct ipl_step ipl13[] = {
    {IPL_DEF(host_disable_memvolt), 13, 1, true, true},
    {IPL_DEF(mem_pll_reset), 13, 2, true, true},
    {IPL_DEF(mem_pll_initf), 13, 3, true, true},
    {IPL_DEF(mem_pll_setup), 13, 4, true, true},
    {IPL_DEF(proc_mcs_skewadjust), 13, 5, true, true},
    {IPL_DEF(mem_startclocks), 13, 6, true, true},
    {IPL_DEF(host_enable_memvolt), 13, 7, true, true},
    {IPL_DEF(mss_scominit), 13, 8, true, true},
    {IPL_DEF(mss_ddr_phy_reset), 13, 9, true, true},
    {IPL_DEF(mss_draminit), 13, 10, true, true},
    {IPL_DEF(mss_draminit_training), 13, 11, true, true},
    {IPL_DEF(mss_draminit_trainadv), 13, 12, true, true},
    {IPL_DEF(mss_draminit_mc), 13, 13, true, true},
    {NULL, NULL, -1, -1, false, false},
};

__attribute__((constructor)) static void ipl_register_ipl13(void)
{
	ipl_register(13, ipl13, ipl_pre13);
}
