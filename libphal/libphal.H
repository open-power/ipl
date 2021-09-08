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

} // namespace sbe
} // namespace libphal
