extern "C"
{
#include <libpdbg.h>
#include <stdio.h>
}

#include "libipl.H"
#include "libipl_internal.H"

static void ipl_pre3(void)
{
    struct pdbg_target* pib;

    pdbg_for_each_class_target("pib", pib) pdbg_target_probe(pib);
}

static int ipl_proc_sbe_chiplet_reset(void)
{
    struct pdbg_target* pib;
    int rc = 0;

    pdbg_for_each_class_target("pib", pib) rc |= sbe_istep(pib, 3, 1);

    return rc;
}

static int ipl_proc_sbe_gptr_time_initf(void)
{
    struct pdbg_target* pib;
    int rc = 0;

    pdbg_for_each_class_target("pib", pib) rc |= sbe_istep(pib, 3, 2);

    return rc;
}

static int ipl_proc_sbe_chiplet_pll_initf(void)
{
    struct pdbg_target* pib;
    int rc = 0;

    pdbg_for_each_class_target("pib", pib) rc |= sbe_istep(pib, 3, 3);

    return rc;
}

static int ipl_proc_sbe_chiplet_pll_setup(void)
{
    struct pdbg_target* pib;
    int rc = 0;

    pdbg_for_each_class_target("pib", pib) rc |= sbe_istep(pib, 3, 4);

    return rc;
}

static int ipl_proc_sbe_repr_initf(void)
{
    struct pdbg_target* pib;
    int rc = 0;

    pdbg_for_each_class_target("pib", pib) rc |= sbe_istep(pib, 3, 5);

    return rc;
}

static int ipl_proc_sbe_chiplet_init(void)
{
    struct pdbg_target* pib;
    int rc = 0;

    pdbg_for_each_class_target("pib", pib) rc |= sbe_istep(pib, 3, 6);

    return rc;
}

static int ipl_proc_sbe_abist_setup(void)
{
    struct pdbg_target* pib;
    int rc = 0;

    pdbg_for_each_class_target("pib", pib) rc |= sbe_istep(pib, 3, 7);

    return rc;
}

static int ipl_proc_sbe_arrayinit(void)
{
    struct pdbg_target* pib;
    int rc = 0;

    pdbg_for_each_class_target("pib", pib) rc |= sbe_istep(pib, 3, 8);

    return rc;
}

static int ipl_proc_sbe_lbist(void)
{
    struct pdbg_target* pib;
    int rc = 0;

    pdbg_for_each_class_target("pib", pib) rc |= sbe_istep(pib, 3, 9);

    return rc;
}

static int ipl_proc_sbe_tp_enable_ridi(void)
{
    struct pdbg_target* pib;
    int rc = 0;

    pdbg_for_each_class_target("pib", pib) rc |= sbe_istep(pib, 3, 10);

    return rc;
}

static int ipl_proc_sbe_setup_boot_frequency(void)
{
    struct pdbg_target* pib;
    int rc = 0;

    pdbg_for_each_class_target("pib", pib) rc |= sbe_istep(pib, 3, 11);

    return rc;
}

static int ipl_proc_sbe_initf(void)
{
    struct pdbg_target* pib;
    int rc = 0;

    pdbg_for_each_class_target("pib", pib) rc |= sbe_istep(pib, 3, 12);

    return rc;
}

static int ipl_proc_sbe_nest_startclocks(void)
{
    struct pdbg_target* pib;
    int rc = 0;

    pdbg_for_each_class_target("pib", pib) rc |= sbe_istep(pib, 3, 13);

    return rc;
}

static int ipl_proc_sbe_nest_enable_ridi(void)
{
    struct pdbg_target* pib;
    int rc = 0;

    pdbg_for_each_class_target("pib", pib) rc |= sbe_istep(pib, 3, 14);

    return rc;
}

static int ipl_proc_sbe_io_initf(void)
{
    struct pdbg_target* pib;
    int rc = 0;

    pdbg_for_each_class_target("pib", pib) rc |= sbe_istep(pib, 3, 15);

    return rc;
}

static int ipl_proc_sbe_startclock_chiplets(void)
{
    struct pdbg_target* pib;
    int rc = 0;

    pdbg_for_each_class_target("pib", pib) rc |= sbe_istep(pib, 3, 16);

    return rc;
}

static int ipl_proc_sbe_scominit(void)
{
    struct pdbg_target* pib;
    int rc = 0;

    pdbg_for_each_class_target("pib", pib) rc |= sbe_istep(pib, 3, 17);

    return rc;
}

static int ipl_proc_sbe_lpc(void)
{
    struct pdbg_target* pib;
    int rc = 0;

    pdbg_for_each_class_target("pib", pib) rc |= sbe_istep(pib, 3, 18);

    return rc;
}

static int ipl_proc_sbe_fabricinit(void)
{
    struct pdbg_target* pib;
    int rc = 0;

    pdbg_for_each_class_target("pib", pib) rc |= sbe_istep(pib, 3, 19);

    return rc;
}

static int ipl_proc_sbe_check_master(void)
{
    struct pdbg_target* pib;
    int rc = 0;

    pdbg_for_each_class_target("pib", pib) rc |= sbe_istep(pib, 3, 20);

    return rc;
}

static int ipl_proc_sbe_mcs_setup(void)
{
    struct pdbg_target* pib;
    int rc = 0;

    pdbg_for_each_class_target("pib", pib) rc |= sbe_istep(pib, 3, 21);

    return rc;
}

static int ipl_proc_sbe_select_ex(void)
{
    struct pdbg_target* pib;
    int rc = 0;

    pdbg_for_each_class_target("pib", pib) rc |= sbe_istep(pib, 3, 22);

    return rc;
}

static struct ipl_step ipl3[] = {
    {IPL_DEF(proc_sbe_chiplet_reset), 3, 1, true, true},
    {IPL_DEF(proc_sbe_gptr_time_initf), 3, 2, true, true},
    {IPL_DEF(proc_sbe_chiplet_pll_initf), 3, 3, true, true},
    {IPL_DEF(proc_sbe_chiplet_pll_setup), 3, 4, true, true},
    {IPL_DEF(proc_sbe_repr_initf), 3, 5, true, true},
    {IPL_DEF(proc_sbe_chiplet_init), 3, 6, true, true},
    {IPL_DEF(proc_sbe_abist_setup), 3, 7, true, false},
    {IPL_DEF(proc_sbe_arrayinit), 3, 8, true, true},
    {IPL_DEF(proc_sbe_lbist), 3, 9, true, false},
    {IPL_DEF(proc_sbe_tp_enable_ridi), 3, 10, true, true},
    {IPL_DEF(proc_sbe_setup_boot_frequency), 3, 11, true, true},
    {IPL_DEF(proc_sbe_initf), 3, 12, true, true},
    {IPL_DEF(proc_sbe_nest_startclocks), 3, 13, true, true},
    {IPL_DEF(proc_sbe_nest_enable_ridi), 3, 14, true, true},
    {IPL_DEF(proc_sbe_io_initf), 3, 15, true, true},
    {IPL_DEF(proc_sbe_startclock_chiplets), 3, 16, true, true},
    {IPL_DEF(proc_sbe_scominit), 3, 17, true, true},
    {IPL_DEF(proc_sbe_lpc), 3, 18, true, true},
    {IPL_DEF(proc_sbe_fabricinit), 3, 19, true, true},
    {IPL_DEF(proc_sbe_check_master), 3, 20, true, true},
    {IPL_DEF(proc_sbe_mcs_setup), 3, 21, true, true},
    {IPL_DEF(proc_sbe_select_ex), 3, 22, true, true},
    {NULL, NULL, -1, -1, false, false},
};

__attribute__((constructor)) static void ipl_register_ipl3(void)
{
    ipl_register(3, ipl3, ipl_pre3);
}
