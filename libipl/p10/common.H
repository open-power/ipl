#ifndef __LIBIPL_P10_COMMON_H__
#define __LIBIPL_P10_COMMON_H__

extern "C" {
#include <stdbool.h>
#include <libpdbg.h>
#include <libpdbg_sbe.h>
}

#include <ekb/hwpf/fapi2/include/return_code.H>

#define NUM_CLOCK_FOR_REDUNDANT_MODE 2

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
 * @brief Get functional primary processor
 *
 * @return On success return pointer to functional primary proc, else NULL
 */
struct pdbg_target *ipl_get_functional_primary_proc(void);

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

/*
 * @brief Process HWP/PLAT error to add callout/deconfig and callback
 * details
 * @param fapirc[in] - FAPI return code
 * @param target[in] - target pointer
 * @param deconfig[in] - true deconfig the target
 */
void ipl_process_fapi_error(const fapi2::ReturnCode &fapirc,
			    struct pdbg_target *target, bool deconfig = true);

/**
 * @brief Process PLAT clock error to add callout/deconfig and callback
 *
 * On clock failure, planar need to be called out. Also, it has to
 * include ffdc data provided by caller. And then call ipl_error_callback()
 *
 * @param[in] ffdcs_data list of internal ffdc values
 * @param[in] clk_pos    position of failed clock
 */
void ipl_plat_clock_error_handler(
    const std::vector<std::pair<std::string, std::string>> &ffdcs_data,
    uint8_t clk_pos);

/**
 * @brief Process PLAT software error to add procedure callout and call callback
 *
 * On software errors, BMC0001 procedure need to be called out. Also, it has to
 * include ffdc data provided by caller. And then call ipl_error_callback()
 *
 * @param[in] err_type   type of error happened
 * @param[in] ffdcs_data list of internal ffdc values
 */
void ipl_plat_procedure_error_handler(
    const ipl_error_type &err_type,
    const std::vector<std::pair<std::string, std::string>> &ffdcs_data = {});

#endif /* __LIBIPL_P10_COMMON_H__ */
