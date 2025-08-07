#pragma once

#include "phal_exception.H"

extern "C" {
#include <libpdbg.h>
#include <libpdbg_sbe.h>
}

#include <attributes_info.H>
#include <vector>
#include <optional>
#include <expected>

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
 * @param[in] proc processor target to operate on
 * @param[in] coSuccess flag to inidicate if the chipop is sucess
 *
 * return  sbeError_t sbe error exception type object which
 *         includes chip-op failure reason code and, file which includes
 *         SBE FFDC response data if any
 *
 * Exceptions: SbeError for pdbg lib function failure
 *             std::runtime_error - for file operation failure.
 */
sbeError_t captureFFDC(struct pdbg_target *proc, bool coSuccess = false);

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
} // namespace sbe

namespace pdbg
{
using LocationCode = std::string;
using FRUType = uint8_t;

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
 * @brief Check if Hostboot has completed and the control transistioned to
 * Host/PHYP
 *
 * @return true when we have moved to PHYP
 * @return false if there is any error in reading the scom address,
 * consider control still in hostboot
 */
bool hasControlTransitionedToHost();

/**
 * @brief Get unexpanded location code
 *
 * @details An api to get an unexpanded location code corresponding to
 *          a given expanded location code. Unexpanded location codes
 *          give the location of the FRU in the system.
 *
 * @param[in] locCode - Location code in expanded format.
 *
 * @return Location code as string which will be in un-expanded format or
 *         empty optional on failure.
 */
std::optional<LocationCode> getUnexpandedLocCode(const LocationCode &locCode);

/**
 * @brief Retrieves FRU information for the provided location codes.
 *
 * Location codes can be in either expanded or unexpanded format.
 *
 * @param[in] locationCode A FRU location code to fetch its type.
 *
 * @return FRUType or error code
 */
std::expected<FRUType, openpower::phal::exception::ERR_TYPE>
    getFRUType(const LocationCode &locationCode) noexcept;

} // namespace pdbg

namespace dump
{
// Define the chip types numbers
static constexpr int PROC_SBE_DUMP = 0xA;
static constexpr int ODYSSEY_SBE_DUMP = 0xB;

/**
 * @brief Initializes the PDBG and LIBEKB environments.
 *
 * @throw std::runtime_error Throws if either PDBG or LIBEKB initialization
 * fails.
 */
void initializePdbgLibEkb();

/**
 * @brief Return the chip target corresponding to failing unit
 * @param[in] failingUnit Position of the ocmb containing the failed SBE
 * @param sbeTypeId The chip type number
 *
 * @return Pointer to the pdbg target for retreived chip
 *
 * Exceptions: PDBG_TARGET_NOT_OPERATIONAL if target is not available
 */
struct pdbg_target* getTargetFromFailingId(const uint32_t failingUnit,
					   const int sbeTypeId);

/**
 * @brief Probes a target (FSI or PIB) for the given processor target.
 * @param[in] proc The processor or Odyssey target to probe the FSI/PIB target
 * for.
 * @param[in] targetType The type of target to probe ("fsi" or "pib").
 * @param[in] sbeTypeId The chip type ID
 * @return Pointer to the probed pdbg target. If the target is not operational,
 *         the function throws an exception.
 *
 * @throw dumpError_t Throws if the target is not found or not operational.
 */
struct pdbg_target* probeTarget(struct pdbg_target* proc,
				const char* targetType, const int sbeTypeId);

/**
 * @brief Checks the state of the SBE on the given PIB target.
 * @param[in] pib_fsi The PIB or FSI target to check the SBE state of.
 * @param[in] sbeTypeId The chip type ID
 *
 * @throw sbeError_t Throws if the SBE state is 'FAILED' (indicating a dump
 *        has already been collected) or if there is a failure in reading the
 * SBE state.
 */
void checkSbeState(struct pdbg_target* pib_fsi, const int sbeTypeId);

/**
 * @brief Executes the SBE extract RC and writes recovery action data.
 * @param[in] target The processor/odyssey target on which to execute the SBE
 * extract RC.
 * @param[in] dumpPath The filesystem path where the SBE data and recovery
 * action information will be written.
 * @param sbeTypeId[in] The chip type, i.e.; proc or OCMB
 *
 * @throw std::runtime_error Throws if the SBE extract RC operation fails.
 */
void executeSbeExtractRc(struct pdbg_target* target,
			 const std::filesystem::path& dumpPath,
			 const int sbeTypeId);

/**
 * @brief Collects and writes local register dump data for a given processor
 * target.
 *
 * This function is responsible for collecting the local register dump from the
 * specified processor target and writing this data to a file. It forms the
 * complete file path using the provided base filename and dump path. If the
 * function encounters any errors during the data collection or file writing
 * process, it throws an exception.
 *
 * @param[in] target The processor or Odyssey target from which to collect the
 * local register dump.
 * @param[in] dumpPath The filesystem path where the dump file will be written.
 * @param[in] baseFilename The base filename to use for the dump file. This name
 * will be used to form the complete file path along with the dumpPath.
 * @param sbeTypeId[in] Chip type ID
 *
 * @throw std::runtime_error Throws if there is an error in collecting the local
 * register dump or writing to the file.
 */
void collectLocalRegDump(struct pdbg_target* target,
			 const std::filesystem::path& dumpPath,
			 const std::string& baseFilename, const int sbeTypeId);

/**
 * @brief Collects and writes PIBMS register dump data for a given processor
 * target.
 * @param[in] target The processor or odyssey target from which to collect the
 * PIBMS register dump.
 * @param[in] dumpPath The filesystem path where the dump file will be written.
 * @param[in] baseFilename The base filename to use for the dump file. This name
 * @param sbeTypeId[in] Chip type ID
 * will be used to form the complete file path along with the dumpPath.
 *
 * @throw std::runtime_error Throws if there is an error in collecting the PIBMS
 * register dump or writing to the file.
 */
void collectPIBMSRegDump(struct pdbg_target* target,
			 const std::filesystem::path& dumpPath,
			 const std::string& baseFilename, const int sbeTypeId);

/**
 * @brief Collects and writes PIBMEM dump data for a given processor target.
 * @param[in] target The processor target from which to collect the PIBMEM dump
 * data.
 * @param[in] dumpPath The filesystem path where the dump file will be written.
 * @param[in] baseFilename The base filename to use for the dump file. This name
 * will be used to form the complete file path along with the dumpPath.
 * @param sbeTypeId[in] The chip type, i.e.; proc or OCMB
 *
 * @throw std::runtime_error Throws if there is an error in collecting the
 * PIBMEM dump data or writing to the file.
 */
void collectPIBMEMDump(struct pdbg_target* target,
		       const std::filesystem::path& dumpPath,
		       const std::string& baseFilename, const int sbeTypeId);

/**
 * @brief Collects and writes the state of the Programmable Processing Element
 * (PPE).
 * @param[in] target The processor or Odyssey target from which to collect the
 * PPE state.
 * @param[in] dumpPath The filesystem path where the dump file will be written.
 * @param[in] baseFilename The base filename to use for the dump file. This name
 * will be used to form the complete file path along with the dumpPath.
 * @param sbeTypeId[in] The chip type, i.e.; proc or OCMB
 *
 * @throw std::runtime_error Throws if there is an error in collecting the PPE
 * state or writing to the file.
 */
void collectPPEState(struct pdbg_target* target,
		     const std::filesystem::path& dumpPath,
		     const std::string& baseFilename, const int sbeTypeId);

/**
 * @brief Finalizes the collection process.
 * @param[in] pib_fsi The PIB (For PROC) or FSI (For Odyssey) target for which
 * the collection is being finalized.
 * @param[in] dumpPath The filesystem path where the dump was written. Used for
 * cleanup in case of failure.
 * @param[in] isSuccess A boolean flag indicating whether the preceding
 * collection steps were successful.
 * @param sbeTypeId The chip type ID
 */
void finalizeCollection(struct pdbg_target* pib_fsi,
			const std::filesystem::path& dumpPath, bool isSuccess,
			const int sbeTypeId);	
} // namespace dump
} // namespace openpower::phal
