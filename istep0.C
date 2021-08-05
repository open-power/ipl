extern "C"
{
#include <libpdbg.h>
#include <stdio.h>
#include <stdlib.h>
}

#include <libekb.H>
#include <libipl/libipl.H>

int main(void)
{
    if (!pdbg_targets_init(NULL))
        exit(1);

    if (libekb_init())
        exit(1);

    if (ipl_init(IPL_AUTOBOOT))
        exit(1);

    return ipl_run_major(0);
}
