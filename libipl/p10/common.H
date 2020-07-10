#ifndef __LIBIPL_P10_COMMON_H__
#define __LIBIPL_P10_COMMON_H__

extern "C" {
#include <stdbool.h>
#include <libpdbg.h>
}

bool ipl_is_master_proc(struct pdbg_target *proc);
int ipl_istep_via_sbe(int major, int minor);
int ipl_istep_via_hostboot(int major, int minor);

/**
 * @Brief Check every second if sbe is booted or not
 *
 * param[in] proc pdbg_target for processor target
 * param[in] wait_time_seconds time in seconds
 *
 * return true if SBE booted, false otherwise
 */
bool ipl_sbe_booted(struct pdbg_target *proc, uint32_t wait_time_seconds);

#endif /* __LIBIPL_P10_COMMON_H__ */
