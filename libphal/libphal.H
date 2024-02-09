#pragma once

#include "phal_exception.H"

extern "C" {
#include <libpdbg.h>
#include <libpdbg_sbe.h>
}

#include <attributes_info.H>

#include <filesystem>
#include <vector>

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
 * @brief utility function to set SBE State
 *
 * Note: Caller has to make sure processor is functional
 *
 * @param[in] proc processor target to operate on
 * @param[in] state sbe state
 *
 * Exceptions: SbeError
 */
void setState(struct pdbg_target *proc, enum sbe_state state);

/**
 * @brief utility function to get SBE State
 *
 * Note: Caller has to make sure processor is functional
 *
 * @param[in] proc processor target to operate on
 *
 * return sbe state value
 *
 * Exceptions: SbeError
 */
enum sbe_state getState(struct pdbg_target *proc);

/**
 * @brief utility function to check Primary processor SBE IPL completed or not
 *
 * @return true for primary processor SBE IPL completed
 *
 * Exceptions: SbeError
 */
bool isPrimaryIplDone();

/**
 * @brief utility function to check SBE Dump collection is allowed
 *        on requested processor.
 *        SBE Dump is not allowed in SBE_STATE_DEBUG_MODE. This mode
 *        indicates another dump collection is already in progress.
 *
 * @param[in] proc processor target to operate on
 *
 * @return  true if dump is allowed.
 *
 * Exceptions: SbeError
 */
bool isDumpAllowed(struct pdbg_target *proc);

/**
 * @brief utility function to capture SBE ffdc data using
 *        pdbg function sbe_ffdc_get().
 *
 * @param[in] target processor/ocmb target to operate on
 *
 * return  sbeError_t sbe error exception type object which
 *         includes chip-op failure reason code and, file which includes
 *         SBE FFDC response data if any
 *
 * Exceptions: SbeError for pdbg lib function failure
 *             std::runtime_error - for file operation failure.
 */
sbeError_t captureFFDC(struct pdbg_target *target);

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

/**
 * @brief Wrapper function sbe get dump chipop.
 *
 * The dump data must be freed by caller. It is allocated using malloc() and
 * must be freed using free().
 *
 * @param[in] chip proc or OCMB target to operate on
 * @param[in] type Type of dump
 * @param[in] clock Clock on or off
 * @param[in] faCollect Fast Array collection (0 off, 1 on)
 * @param[out] data Dump information
 * @param[out] dataLen length of the data
 *
 * Exceptions: SbeError with failure reason code
 */
void getDump(struct pdbg_target *chip, const uint8_t type, const uint8_t clock,
	     const uint8_t faCollect, uint8_t **data, uint32_t *dataLen);

/**
 * @brief Execute stop all instructions on processor.
 *
 * @param[in] proc processor target to operate on
 *
 * Exceptions: SbeError with failure reason code, file name and fd.
 *             std::runtime_error - for file operation failure.
 */
void threadStopProc(struct pdbg_target *proc);


/**
 * @brief Check whether the input target is an Odyssey OCMB
 * 
 * @param target Pointer to the chip tartget
 * 
 * @return True if Odyssey OCMB chip else false
*/
bool is_ody_ocmb_chip(struct pdbg_target *target);

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
	  const std::string pdbgDtbPath =
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

/**
 * @brief Deconfigures a Target.
 *
 * This will Deconfigure the Target (set state to non-functional,
 * do any Deconfigure actions and create a Deconfigure Record).
 *
 * @param[in] physBinPath PHYS_BIN_PATH value
 * @param[in] logId  Error log ID to store in Deconfigure Record.
 *
 *  Exceptions: pdbgError with failure reason code
 */
void deconfigureTgt(const ATTR_PHYS_BIN_PATH_Type &physBinPath,
		    const uint32_t logId);

/**
 * @brief get Loaction code
 *
 * Function to get location code attribute from devtree or
 * or from parent target.
 * Incase user provided target doesn't support location code
 * attribute used special algorithm to get parent's location code.
 * Refer implementation code  comment's for details related to
 * the algorithm used.
 *
 * param[in] target target to operate on
 * @param[out] ATTR_LOCATION_CODE_Type Location code value
 *
 * Exceptions: pdbgError with failure reason code
 */
void getLocationCode(struct pdbg_target *proc,
		     ATTR_LOCATION_CODE_Type &locationCode);
/**
 * @brief Read a CFAM FSI register
 * @param[in] proc processor target to operate on
 * @param[in] addr the CFAM address offset
 *
 * @return the CFAM register data.
 *
 * Exceptions: pdbgError with failure reason code
 */
uint32_t getCFAM(struct pdbg_target *proc, const uint32_t addr);

/**
 * @brief update a CFAM FSI register
 * @param[in] proc processor target to operate on
 * @param[in] addr the CFAM address offset
 * @param[in] val the write data
 *
 * Exceptions: pdbgError with failure reason code
 */
void putCFAM(struct pdbg_target *proc, const uint32_t addr, const uint32_t val);

/**
 * @brief Check SBE vital attention is active
 *
 * @param[in] proc processor target to operate on
 *
 * @return true/false base on vital attention status
 */
bool isSbeVitalAttnActive(struct pdbg_target *proc);

/**
 * @brief Check if Hostboot has completed and the control transistioned to Host/PHYP
 *
 * @return true when we have moved to PHYP
 * @return false if there is any error in reading the scom address, 
 * consider control still in hostboot
 */
bool hasControlTransitionedToHost();

} // namespace pdbg

namespace dump
{
/**
 * @brief Execute HWPs to collect SBE dump
 * @param[in] id Id of the dump
 * @param[in] failingUnit Id of proc containing failing SBE
 * @param[in] dumpPath Path to stored the dump files
 * @param[in] sbeTypeId ID for SBE type i.e.; Odyssey or normal memory chip
 * 						0xA-->Normal SBE type, 0xB-->Odyssey SBE type
 * Exceptions: PDBG_INIT_FAIL for any pdbg init related failure.
 */
void collectSBEDump(uint32_t id, uint32_t failingUnit,
		    const std::filesystem::path &dumpPath, const int sbeTypeId);
} // namespace dump
} // namespace openpower::phal
