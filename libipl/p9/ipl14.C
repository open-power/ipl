extern "C" {
#include <stdio.h>

#include <libpdbg.h>
}

#include "libipl.H"
#include "libipl_internal.H"

static void ipl_pre14(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib) pdbg_target_probe(pib);
}

static int ipl_mss_memdiag(void)
{
	return -1;
}

static int ipl_mss_thermal_init(void)
{
	return -1;
}

static int ipl_proc_pcie_config(void)
{
	return -1;
}

static int ipl_mss_power_cleanup(void)
{
	return -1;
}

static int ipl_proc_setup_bars(void)
{
	return -1;
}

static int ipl_proc_htm_setup(void)
{
	return -1;
}

static int ipl_proc_exit_cache_contained(void)
{
	return -1;
}

static int ipl_host_mpipl_service(void)
{
	return -1;
}

static struct ipl_step ipl14[] = {
    {IPL_DEF(mss_memdiag), 14, 1, false, true},
    {IPL_DEF(mss_thermal_init), 14, 2, true, true},
    {IPL_DEF(proc_pcie_config), 14, 3, true, true},
    {IPL_DEF(mss_power_cleanup), 14, 4, true, true},
    {IPL_DEF(proc_setup_bars), 14, 5, true, true},
    {IPL_DEF(proc_htm_setup), 14, 6, true, true},
    {IPL_DEF(proc_exit_cache_contained), 14, 7, false, true},
    {IPL_DEF(host_mpipl_service), 14, 8, false, true},
    {NULL, NULL, -1, -1, false, false},
};

__attribute__((constructor)) static void ipl_register_ipl14(void)
{
	ipl_register(14, ipl14, ipl_pre14);
}
