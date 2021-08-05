extern "C"
{
#include <libpdbg.h>
#include <stdio.h>
}

#include "libipl.H"
#include "libipl_internal.H"

static void ipl_pre18(void)
{
    struct pdbg_target* pib;

    pdbg_for_each_class_target("pib", pib) pdbg_target_probe(pib);
}

static int ipl_proc_tod_setup(void)
{
    return -1;
}

static int ipl_proc_tod_init(void)
{
    return -1;
}

static struct ipl_step ipl18[] = {
    {IPL_DEF(proc_tod_setup), 18, 11, true, true},
    {IPL_DEF(proc_tod_init), 18, 12, true, true},
    {NULL, NULL, -1, -1, false, false},
};

__attribute__((constructor)) static void ipl_register_ipl18(void)
{
    ipl_register(18, ipl18, ipl_pre18);
}
