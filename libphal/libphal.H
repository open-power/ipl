#pragma once

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

} // namespace sbe
} // namespace openpower::phal
