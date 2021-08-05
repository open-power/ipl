extern "C"
{
#include <libpdbg.h>
#include <stdio.h>
}

#include "libipl.H"
#include "libipl_internal.H"

static void ipl_pre21(void)
{
    struct pdbg_target* pib;

    pdbg_for_each_class_target("pib", pib) pdbg_target_probe(pib);
}

static int ipl_host_runtime_setup(void)
{
    return -1;
}

static int ipl_host_verify_hdat(void)
{
    return -1;
}

static int ipl_host_start_payload(void)
{
    return -1;
}

static struct ipl_step ipl21[] = {
    {IPL_DEF(host_runtime_setup), 21, 1, false, true},
    {IPL_DEF(host_verify_hdat), 21, 2, false, true},
    {IPL_DEF(host_start_payload), 21, 3, false, true},
    {NULL, NULL, -1, -1, false, false},
};

__attribute__((constructor)) static void ipl_register_ipl21(void)
{
    ipl_register(21, ipl21, ipl_pre21);
}
