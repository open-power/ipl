extern "C"
{
#include <libpdbg.h>
#include <stdio.h>
}

#include "libipl.H"
#include "libipl_internal.H"

static void ipl_pre12(void)
{
    struct pdbg_target* pib;

    pdbg_for_each_class_target("pib", pib) pdbg_target_probe(pib);
}

static int ipl_mss_getecid(void)
{
    return -1;
}

static int ipl_dmi_attr_update(void)
{
    return -1;
}

static int ipl_proc_dmi_scom_init(void)
{
    return -1;
}

static int ipl_cen_dmi_scominit(void)
{
    return -1;
}

static int ipl_dmi_erepair(void)
{
    return -1;
}

static int ipl_dmi_io_dccal(void)
{
    return -1;
}

static int ipl_dmi_pre_trainadv(void)
{
    return -1;
}

static int ipl_dmi_io_run_training(void)
{
    return -1;
}

static int ipl_dmi_post_trainadva(void)
{
    return -1;
}

static int ipl_proc_cen_framelock(void)
{
    return -1;
}

static int ipl_host_startprd_dmi(void)
{
    return -1;
}

static int ipl_host_attnlisten_memb(void)
{
    return -1;
}

static int ipl_cen_set_inband_addr(void)
{
    return -1;
}

static struct ipl_step ipl12[] = {
    {IPL_DEF(mss_getecid), 12, 1, true, true},
    {IPL_DEF(dmi_attr_update), 12, 2, true, true},
    {IPL_DEF(proc_dmi_scom_init), 12, 3, true, true},
    {IPL_DEF(cen_dmi_scominit), 12, 4, true, true},
    {IPL_DEF(dmi_erepair), 12, 5, true, true},
    {IPL_DEF(dmi_io_dccal), 12, 6, true, true},
    {IPL_DEF(dmi_pre_trainadv), 12, 7, true, true},
    {IPL_DEF(dmi_io_run_training), 12, 8, true, true},
    {IPL_DEF(dmi_post_trainadva), 12, 9, true, true},
    {IPL_DEF(proc_cen_framelock), 12, 10, true, true},
    {IPL_DEF(host_startprd_dmi), 12, 11, false, true},
    {IPL_DEF(host_attnlisten_memb), 12, 12, false, true},
    {IPL_DEF(cen_set_inband_addr), 12, 13, true, true},
    {NULL, NULL, -1, -1, false, false},
};

__attribute__((constructor)) static void ipl_register_ipl12(void)
{
    ipl_register(12, ipl12, ipl_pre12);
}
