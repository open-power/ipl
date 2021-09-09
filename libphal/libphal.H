#pragma once

#include "phal_exception.H"

extern "C" {
#include <libpdbg.h>
}

namespace openpower::phal
{
namespace sbe
{

/**
 * @brief utility function to check SBE state to allow chipops.
 *
 * Note: Caller has to make sure processor should functional/dump functional
 *
 * @param[in] proc processor target to operate on
 *
 * Exceptions: SbeError
 */
void validateSBEState(struct pdbg_target *proc);

/**
 * @brief utility function to capture SBE ffdc data using
 *        pdbg function sbe_ffdc_get().
 *
 * @param[in] proc processor target to operate on
 *
 * return  sbeError_t sbe error exception type object which
 *         includes chip-op failure reason code and, file which includes
 *         SBE FFDC response data if any
 *
 * Exceptions: SbeError for pdbg lib function failure
 *             std::runtime_error - for file operation failure.
 */
sbeError_t captureFFDC(struct pdbg_target *proc);

/**
 * @brief Execute continue mpipl on the pib
 *
 * @param[in] proc processor target to operate on
 *
 * Exceptions: SbeError with failure reason code, File name and fd.
 *             std::runtime_error - for file operation failure.
 */
void mpiplContinue(struct pdbg_target *proc);

/**
 * @brief Get ti info
 *
 * @param[in] proc processor target to operate on
 * @param[out] data TI information
 * @param[out] dataLen length of the data
 *
 * Exceptions: SbeError with failure reason code, File name and fd.
 *             std::runtime_error - for file operation failure.
 */
void getTiInfo(struct pdbg_target *proc, uint8_t **data, uint32_t *dataLen);

} // namespace sbe
} // namespace openpower::phal
