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

} // namespace sbe
} // namespace libphal
