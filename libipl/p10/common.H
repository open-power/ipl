#ifndef __LIBIPL_P10_COMMON_H__
#define __LIBIPL_P10_COMMON_H__

extern "C" {
#include <stdbool.h>
#include <libpdbg.h>
#include <libpdbg_sbe.h>
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

/**
 * @brief Determine whether the target is present or not
 *
 * param[in] target pdbg_target
 *
 * @return true if the target is present, false otherwise
 */

bool ipl_is_present(struct pdbg_target *target);
/**
 * @brief Determine whether the target is functional or not
 *
 * param[in] target pdbg_target
 *
 * @return true if the target is functional, false otherwise
 */
bool ipl_is_functional(struct pdbg_target *target);

/**
 * @brief Check if functional master processor is available.
 *
 * @return true if function master available, false otherwise
 */
bool ipl_check_functional_master(void);

/**
 * @brief Fetch and log SBE FFDC data if available
 *
 * param[in] pib pdbg_target
 */
void ipl_log_sbe_ffdc(struct pdbg_target *pib);

/**
 * @brief Set sbe state
 *
 * Set the current state of SBE
 *
 * @param[in] target proc target to operate on
 * @param[in] state sbe state
 *
 * @return 0 on success, 1 on failure
 */
int ipl_set_sbe_state(struct pdbg_target *proc, enum sbe_state state);

/**
 * @brief Set sbe state on all present processors
 *
 * Set the current state of all the present processors
 *
 * @param[in] state sbe state
 *
 * @return 0 on success, 1 on failure
 * Note : continue all the present processors for failures.
 */
int ipl_set_sbe_state_all(enum sbe_state state);

/**
 * @brief Set sbe state on all present secondary processors
 *
 * Set the current state of all the present secondary processors
 *
 * @param[in] state sbe state
 *
 * @return 0 on success, 1 on failure
 * Note : continue all the present processors for failures.
 */
int ipl_set_sbe_state_all_sec(enum sbe_state state);

#endif /* __LIBIPL_P10_COMMON_H__ */
