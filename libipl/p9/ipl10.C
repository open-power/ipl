extern "C" {
#include <stdio.h>

#include <libpdbg.h>
}

#include "libipl.H"
#include "libipl_internal.H"

static void ipl_pre10(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib) pdbg_target_probe(pib);
}

static int ipl_proc_build_smp(void)
{
	return -1;
}

static int ipl_host_slave_sbe_update(void)
{
	return -1;
}

static int ipl_host_set_voltages(void)
{
	return -1;
}

static int ipl_proc_cen_ref_clk_enable(void)
{
	return -1;
}

static int ipl_proc_enable_osclite(void)
{
	return -1;
}

static int ipl_proc_chiplet_scominit(void)
{
	return -1;
}

static int ipl_proc_abus_scominit(void)
{
	return -1;
}

static int ipl_proc_obus_scominit(void)
{
	return -1;
}

static int ipl_proc_npu_scominit(void)
{
	return -1;
}

static int ipl_proc_pcie_scominit(void)
{
	return -1;
}

static int ipl_proc_scomoverride_chiplets(void)
{
	return -1;
}

static int ipl_proc_chiplet_enable_ridi(void)
{
	return -1;
}

static int ipl_host_rng_bist(void)
{
	return -1;
}

static int ipl_host_update_redundant_tpm(void)
{
	return -1;
}

static struct ipl_step ipl10[] = {
    {IPL_DEF(proc_build_smp), 10, 1, true, true},
    {IPL_DEF(host_slave_sbe_update), 10, 2, false, true},
    {IPL_DEF(host_set_voltages), 10, 3, true, true},
    {IPL_DEF(proc_cen_ref_clk_enable), 10, 4, true, true},
    {IPL_DEF(proc_enable_osclite), 10, 5, false, true},
    {IPL_DEF(proc_chiplet_scominit), 10, 6, true, true},
    {IPL_DEF(proc_abus_scominit), 10, 7, true, true},
    {IPL_DEF(proc_obus_scominit), 10, 8, true, true},
    {IPL_DEF(proc_npu_scominit), 10, 9, true, true},
    {IPL_DEF(proc_pcie_scominit), 10, 10, true, true},
    {IPL_DEF(proc_scomoverride_chiplets), 10, 11, true, true},
    {IPL_DEF(proc_chiplet_enable_ridi), 10, 12, true, true},
    {IPL_DEF(host_rng_bist), 10, 13, true, true},
    {IPL_DEF(host_update_redundant_tpm), 10, 14, false, true},
    {NULL, NULL, -1, -1, false, false},
};

__attribute__((constructor)) static void ipl_register_ipl10(void)
{
	ipl_register(10, ipl10, ipl_pre10);
}
