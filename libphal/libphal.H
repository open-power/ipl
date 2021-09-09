#pragma once

extern "C" {
#include <libpdbg.h>
}

namespace libphal
{
namespace sbe
{

/**
 * @brief utility function to check SBE state to allow chipops.
 *
 * Note: Caller has to make sure pib belong to functional/dump functional
 *       processor
 *
 * @param[in] target pib target to operate on
 *
 * Exceptions: SbeError
 */
void validateSBEState(struct pdbg_target *pib);

/**
 * @brief utility function to capture SBE ffdc data using
 *        pdbg function sbe_ffdc_get().
 *        For regular chip-op failure, SBE FFDC response data copied
 *        into a temporary file.
 *
 * Exceptions: SbeError with failure reason code, File name and fd.
 *             std::runtime_error - for file operation failure.
 */
void captureFFDC(struct pdbg_target *pib);

/**
 * @brief Execute continue mpipl on the pib
 *
 * @param[in] pib target to operate on
 *
 * Exceptions: SbeError with failure reason code, File name and fd.
 *             std::runtime_error - for file operation failure.
 */
void mpiplContinue(struct pdbg_target *pib);

/**
 * @brief Execute enter mpipl on the pib
 *
 * @param[in] pib target to operate on
 *
 * Exceptions: SbeError with failure reason code, File name and fd.
 *             std::runtime_error - for file operation failure.
 */
void mpiplEnter(struct pdbg_target *pib);

/**
 * @brief Get ti info
 *
 * @param[in] pib pib target to operate on
 * @param[out] data TI information
 * @param[out] dataLen length of the data
 *
 * Exceptions: SbeError with failure reason code, File name and fd.
 *             std::runtime_error - for file operation failure.
 */
void getTiInfo(struct pdbg_target *pib, uint8_t **data, uint32_t *dataLen);

} // namespace sbe
} // namespace libphal
