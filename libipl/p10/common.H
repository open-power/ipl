#ifndef __LIBIPL_P10_COMMON_H__
#define __LIBIPL_P10_COMMON_H__

extern "C" {
#include <stdbool.h>
#include <libpdbg.h>
}

bool ipl_is_master_proc(struct pdbg_target *proc);
int ipl_istep_via_sbe(int major, int minor);
int ipl_istep_via_hostboot(int major, int minor);

#endif /* __LIBIPL_P10_COMMON_H__ */
