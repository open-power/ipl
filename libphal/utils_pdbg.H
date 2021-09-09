extern "C" {
#include <libpdbg.h>
}

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
 *  Exceptions: SbeError
 */
pdbg_target *getPibTarget(pdbg_target *proc);

/**
 *  @brief  Helper function to find FSI target needed for FSI operations
 *
 *  @param[in]  proc - Processor target to find the FSI target on
 *
 *  @return Valid pointer to FSI target on success
 *
 *  Exceptions: SbeError
 */
pdbg_target *getFsiTarget(pdbg_target *proc);

} // namespace pdbg
} // namespace openpower::phal::utils