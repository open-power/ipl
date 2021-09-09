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

} // namespace sbe
} // namespace openpower::phal
