extern "C" {
#include <libpdbg.h>
}

#include <attributes_info.H>

#include <vector>

namespace openpower::phal::utils
{
namespace pdbg
{

/**
 *  @brief  Helper function to find PIB target needed for PIB operations
 *
 *  @param[in]  proc - Processor target to find the PIB target on
 *
 *  @return Valid pointer to PIB target on success
 *
 *  Exceptions: PdbgError
 */
struct pdbg_target *getPibTarget(struct pdbg_target *proc);

/**
 *  @brief  Helper function to find FSI target needed for FSI operations
 *
 *  @param[in]  proc - Processor target to find the FSI target on
 *
 *  @return Valid pointer to FSI target on success
 *
 *  Exceptions: PdbgError
 */
struct pdbg_target *getFsiTarget(struct pdbg_target *proc);

/**
 * @brief Used to get target based on bin path from phal device tree
 *
 * @param[in] binPath PHYS_BIN_PATH value
 *
 * @return pdbg target associated to given entity path.
 **/
struct pdbg_target *getTgtFromBinPath(const ATTR_PHYS_BIN_PATH_Type &binPath);

/**
 *  @brief  Helper function to validate the input target is processor type
 *
 *  @param[in]  tgt - target to check
 *
 *  Exceptions: PdbgError for non processor or invalid targets
 */
void validateProcTgt(struct pdbg_target *tgt);

/**
 * @brief Check whether the input target is an Odyssey OCMB
 *
 * @param target Pointer to the chip tartget
 *
 * @return True if Odyssey OCMB chip else false
*/
bool isOdysseyChip(struct pdbg_target *target);

} // namespace pdbg
} // namespace openpower::phal::utils
