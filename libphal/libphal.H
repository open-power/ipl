#pragma once

#include "phal_exception.H"

extern "C" {
#include <libpdbg.h>
}

#include <filesystem>

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
void validateSBEState(const struct pdbg_target *proc);

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
 * @brief Execute enter mpipl on the pib
 *
 * @param[in] proc processor target to operate on
 *
 * Exceptions: SbeError with failure reason code, File name and fd.
 *             std::runtime_error - for file operation failure.
 */
void mpiplEnter(struct pdbg_target *proc);

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

namespace pdbg
{
/**
 * @brief wrapper function to Initialise the pdbg library targeting system
 *        based on user provided back-end and device tree.
 * @param[in] pdbgBackend  PDBG back-end type (optional)
 * @param[in] logLevel PDBG log level (optional)
 * @param[in] pdbgDtbPath  PDBG_DTB environment variable path (optional)
 *
 * Exceptions: PDBG_INIT_FAIL for any pdbg init related failure.
 */
void init(pdbg_backend pdbgBackend = PDBG_BACKEND_SBEFIFO,
	  const int32_t logLevel = PDBG_ERROR,
	  std::string pdbgDtbPath =
	      "/var/lib/phosphor-software-manager/pnor/rw/DEVTREE");
/**
 * @brief Determine whether the target is present or not
 *
 * param[in] target target to operate on
 *
 * @return true if the target is present, false otherwise
 *
 * Exceptions: pdbgError with failure reason code
 */
bool isTgtPresent(struct pdbg_target *target);

/**
 * @brief Determine whether the target is functional or not
 *
 * param[in] target target to operate on
 *
 * @return true if the target is functional, false otherwise
 *
 * Exceptions: pdbgError with failure reason code
 */
bool isTgtFunctional(struct pdbg_target *target);

/**
 *  @brief  Check if primary processor or not
 *
 *  param[in] proc processor target to operate on
 *
 *  @return True/False
 *
 *  Exceptions: pdbgError with failure reason code
 */
bool isPrimaryProc(struct pdbg_target *proc);

/**
 *  @brief Get primary processor pdbg target
 *
 *  @return primary processor pdbg target.
 *
 *  Exceptions: pdbgError with failure reason code
 */
struct pdbg_target *getPrimaryProc();

} // namespace pdbg

namespace dump
{
/**
 * @brief Execute HWPs to collect SBE dump
 * @param[in] id Id of the dump
 * @param[in] failingUnit Id of proc containing failing SBE
 * @param[in] dumpPath Path to stored the dump files
 *
 * Exceptions: PDBG_INIT_FAIL for any pdbg init related failure.
 */
void collectSBEDump(uint32_t id, uint32_t failingUnit,
		    const std::filesystem::path &dumpPath);
} // namespace dump
} // namespace openpower::phal
