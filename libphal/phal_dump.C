extern "C" {
#include <libgen.h>
#include <libpdbg.h>
#include <libpdbg_sbe.h>
#include <sys/stat.h>
#include <unistd.h>
}

#include "libphal.H"
#include "log.H"
#include "phal_exception.H"
#include "utils_pdbg.H"

#include <fapi2.H>
#include <ekb/hwpf/fapi2/include/return_code_defs.H>
#include <ekb/chips/p10/procedures/hwp/lib/pibms_regs2dump.H>
#include <ekb/chips/p10/procedures/hwp/lib/p10_pibmem_dump.H>
#include <ekb/chips/p10/procedures/hwp/lib/p10_pibms_reg_dump.H>
#include <ekb/chips/p10/procedures/hwp/lib/p10_ppe_state.H>
#include <ekb/chips/p10/procedures/hwp/lib/p10_sbe_localreg_dump.H>
#include <ekb/chips/p10/procedures/hwp/perv/p10_extract_sbe_rc.H>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace openpower::phal
{
namespace dump
{

using namespace openpower::phal::logging;
using namespace openpower::phal::utils::pdbg;
using namespace fapi2;

/* SBE register number and name for writing
 * to the file */
struct DumpSBEReg {
	uint16_t number; // Register number
	char name[32];	 // Register name
} __attribute__((packed));

/* SBE register details with value */
struct DumpSBERegVal {
	DumpSBEReg reg; // Register details
	uint64_t value; // Value extracted

	DumpSBERegVal(const uint16_t num, const std::string& nm,
		      const uint64_t val)
	{
		reg.number = num;
// Adding to avoid Wstringop-truncation warning
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
		strncpy(reg.name, nm.c_str(), 32);
#pragma GCC diagnostic pop
		value = val;
	}
} __attribute__((packed));

/* PIBMEM register number, name and attribute for writing
 * in to the file */
struct DumpPIBMSReg {
	uint64_t addr; // Register address
	char name[32]; // Register name
	uint32_t attr; // Attribute
} __attribute__((packed));

/* PIBMEM register detailes with value */
struct DumpPIBMSRegVal {
	DumpPIBMSReg reg; // Register details
	uint64_t value;	  // Value extracted

	DumpPIBMSRegVal(const uint64_t addrs, const std::string& nm,
			const uint32_t attrib, const uint64_t val)
	{
		reg.addr = addrs;
// Adding to avoid Wstringop-truncation warning
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
		strncpy(reg.name, nm.c_str(), 32);
#pragma GCC diagnostic pop
		reg.attr = attrib;
		value = val;
	}
} __attribute__((packed));

// PPE register details
struct DumpPPERegValue {
	uint16_t number; // Register number
	uint32_t value;	 // Extracted value
	char* name;	 // Name of the register(Unused)

	DumpPPERegValue(uint16_t num, uint32_t val)
	{
		number = num;
		value = val;
	}
} __attribute__((packed));

/**
 * @brief Return the proc target corresponding to failing unit
 * @param[in] failingUnit Position of the proc containing the failed SBE
 *
 * @return Pointer to the pdbg target for proc
 *
 * Exceptions: PDBG_TARGET_NOT_OPERATIONAL if target is not available
 */
struct pdbg_target* getProcFromFailingId(const uint32_t failingUnit)
{
	struct pdbg_target* proc = NULL;
	struct pdbg_target* target;
	pdbg_for_each_class_target("proc", target)
	{
		if (pdbg_target_index(target) != failingUnit) {
			continue;
		}

		if (pdbg_target_probe(target) != PDBG_TARGET_ENABLED) {
			break;
		}
		proc = target;
	}
	if (proc == NULL) {
		log(level::ERROR,
		    "No Proc target found to execute the dump for failing "
		    "unit=%d",
		    failingUnit);
		throw sbeError_t(exception::PDBG_TARGET_NOT_OPERATIONAL);
	}
	return proc;
}

/**
 * @brief Function will write reason code and recovery action in a file
 * @param[in] dumpPath Path of the file
 * @param[in] reasonCode Reason code of sbe failure
 * @param[in] recoveryAction Reovery action code for sbe failure
 *
 * Throws FILE_OPERATION_FAILED in the case of error
 *
 */
void writeSbeData(const std::filesystem::path& dumpPath, uint32_t reasonCode,
		  uint32_t recoveryAction)
{
	// Writing data to file
	std::string dumpFilename = "sbe_data_file";
	std::string additionalPath = "additional_data";
	std::filesystem::path dirPath = dumpPath / additionalPath;
	std::filesystem::path basePath = dirPath / dumpFilename;
	try {
		std::filesystem::create_directories(dirPath);
		std::ofstream sbefile(basePath);
		if (sbefile.is_open()) {
			sbefile << "sbe-reason-code: " << std::hex << "0x"
				<< reasonCode << std::endl;
			sbefile << "sbe-recovery-action: " << std::hex << "0x"
				<< recoveryAction << std::endl;
			sbefile.close();
		}
	} catch (const std::exception& oe) {
		log(level::ERROR,
		    "Failed to write sbe contents in path:%s errno:%d error "
		    "message:%s",
		    basePath.string().c_str(), errno, oe.what());
		throw dumpError_t(exception::FILE_OPERATION_FAILED);
	}
}

/**
 * @brief Initializes the PDBG and LIBEKB environments.
 *
 * @throw std::runtime_error Throws if either PDBG or LIBEKB initialization
 * fails.
 */
void initializePdbgLibEkb()
{
	pdbg::init(PDBG_BACKEND_KERNEL);
	if (libekb_init()) {
		log(level::ERROR, "libekb_init failed");
		throw std::runtime_error("libekb initialization failed");
	}
}

/**
 * @brief Probes a target (FSI or PIB) for the given processor target.
 * @param[in] proc The processor target to probe the FSI/PIB target for.
 * @param[in] targetType The type of target to probe ("fsi" or "pib").
 * @return Pointer to the probed pdbg target. If the target is not operational,
 *         the function throws an exception.
 *
 * @throw dumpError_t Throws if the target is not found or not operational.
 */
struct pdbg_target* probeTarget(struct pdbg_target* proc,
				const char* targetType)
{
	char path[16];
	sprintf(path, "/proc%d/%s", pdbg_target_index(proc), targetType);
	struct pdbg_target* target = pdbg_target_from_path(NULL, path);
	if (!target || pdbg_target_probe(target) != PDBG_TARGET_ENABLED) {
		log(level::ERROR, "Failed to get or probe %s target for (%s)",
		    targetType, pdbg_target_path(proc));
		throw dumpError_t(exception::PDBG_TARGET_NOT_OPERATIONAL);
	}
	return target;
}

/**
 * @brief Checks the state of the SBE on the given PIB target.
 * @param[in] pib The PIB target to check the SBE state of.
 *
 * @throw sbeError_t Throws if the SBE state is 'FAILED' (indicating a dump
 *        has already been collected) or if there is a failure in reading the
 * SBE state.
 */
void checkSbeState(struct pdbg_target* pib)
{
	enum sbe_state state;
	if (sbe_get_state(pib, &state)) {
		log(level::ERROR, "Failed to read SBE state information (%s)",
		    pdbg_target_path(pib));
		throw sbeError_t(exception::SBE_STATE_READ_FAIL);
	}
	if (state == SBE_STATE_FAILED) {
		log(level::ERROR,
		    "Dump is already collected from the SBE on (%s)",
		    pdbg_target_path(pib));
		throw sbeError_t(exception::SBE_DUMP_IS_ALREADY_COLLECTED);
	}
}

/**
 * @brief Executes the SBE extract RC and writes recovery action data.
 * @param[in] proc The processor target on which to execute the SBE extract RC.
 * @param[in] dumpPath The filesystem path where the SBE data and recovery
 * action information will be written.
 *
 * @throw std::runtime_error Throws if the SBE extract RC operation fails.
 */
void executeSbeExtractRc(struct pdbg_target* proc,
			 const std::filesystem::path& dumpPath)
{
	P10_EXTRACT_SBE_RC::RETURN_ACTION recovAction;
	fapi2::ReturnCode fapiRc = p10_extract_sbe_rc(proc, recovAction, true);
	log(level::INFO,
	    "p10_extract_sbe_rc for proc=%s returned rc=0x%08X and SBE "
	    "Recovery Action=0x%08X",
	    pdbg_target_path(proc), fapiRc, recovAction);
	writeSbeData(dumpPath, fapiRc, recovAction);
}

/**
 * @brief Writes SBE register values to a file.
 * @param[in] dumpRegs A vector of `DumpSBERegVal` structures containing the SBE
 * register values to be written.
 * @param[in] basePath The filesystem path of the file to which the data is to
 * be written.
 *
 * @throw std::runtime_error Throws if there is an error opening the file or
 * during the file write operation.
 */
void writeSBERegValuesToFile(const std::vector<DumpSBERegVal>& dumpRegs,
			     const std::filesystem::path& basePath)
{
	try {
		std::ofstream outfile{basePath,
				      std::ios::out | std::ios::binary};
		if (!outfile.good()) {
			throw std::runtime_error(
			    "Failed to open file for writing");
		}
		outfile.write(reinterpret_cast<const char*>(dumpRegs.data()),
			      sizeof(DumpSBERegVal) * dumpRegs.size());
	} catch (const std::exception& e) {
		log(level::ERROR,
		    "Error writing SBE register values to file: %s", e.what());
		throw;
	}
}

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
 * @param[in] proc The processor target from which to collect the local register
 * dump.
 * @param[in] dumpPath The filesystem path where the dump file will be written.
 * @param[in] baseFilename The base filename to use for the dump file. This name
 * will be used to form the complete file path along with the dumpPath.
 *
 * @throw std::runtime_error Throws if there is an error in collecting the local
 * register dump or writing to the file.
 */
void collectLocalRegDump(struct pdbg_target* proc,
			 const std::filesystem::path& dumpPath,
			 const std::string& baseFilename)
{
	std::vector<SBESCOMRegValue_t> sbeScomRegValue;
	ReturnCode fapiRc = p10_sbe_localreg_dump(proc, true, sbeScomRegValue);

	if (fapiRc != FAPI2_RC_SUCCESS) {
		log(level::ERROR,
		    "Failed in p10_sbe_localreg_dump for proc=%s, rc=0x%08X",
		    pdbg_target_path(proc), fapiRc);
		throw std::runtime_error("p10_sbe_localreg_dump failed");
	}

	std::vector<DumpSBERegVal> dumpRegs;
	for (auto& reg : sbeScomRegValue) {
		dumpRegs.emplace_back(reg.reg.number, reg.reg.name, reg.value);
	}
	std::string dumpFilename = baseFilename + "p10_sbe_localreg_dump";
	std::filesystem::path basePath = dumpPath / dumpFilename;

	// Writing the SBE register values to a file
	writeSBERegValuesToFile(dumpRegs, basePath);
}

/**
 * @brief Writes PIBMS register values to a file.
 * @param[in] dumpRegs A vector of `DumpPIBMSRegVal` structures containing the
 * PIBMS register values to be written.
 * @param[in] basePath The filesystem path of the file to which the data is to
 * be written.
 *
 * @throw std::runtime_error Throws if there is an error opening the file or
 * during the file write operation.
 */
void writePIBMSRegValuesToFile(const std::vector<DumpPIBMSRegVal>& dumpRegs,
			       const std::filesystem::path& basePath)
{
	try {
		std::ofstream outfile{basePath,
				      std::ios::out | std::ios::binary};
		if (!outfile.good()) {
			throw std::runtime_error(
			    "Failed to open file for writing");
		}
		outfile.write(reinterpret_cast<const char*>(dumpRegs.data()),
			      sizeof(DumpPIBMSRegVal) * dumpRegs.size());
	} catch (const std::exception& e) {
		log(level::ERROR,
		    "Error writing PIBMS register values to file: %s",
		    e.what());
		throw;
	}
}

/**
 * @brief Collects and writes PIBMS register dump data for a given processor
 * target.
 * @param[in] proc The processor target from which to collect the PIBMS register
 * dump.
 * @param[in] dumpPath The filesystem path where the dump file will be written.
 * @param[in] baseFilename The base filename to use for the dump file. This name
 * will be used to form the complete file path along with the dumpPath.
 *
 * @throw std::runtime_error Throws if there is an error in collecting the PIBMS
 * register dump or writing to the file.
 */
void collectPIBMSRegDump(struct pdbg_target* proc,
			 const std::filesystem::path& dumpPath,
			 const std::string& baseFilename)
{
	std::vector<sRegV> pibmsRegSet;
	for (auto& reg : pibms_regs_2dump) {
		sRegV regv;
		regv.reg = reg;
		pibmsRegSet.emplace_back(regv);
	}

	ReturnCode fapiRc = p10_pibms_reg_dump(proc, pibmsRegSet);
	if (fapiRc != FAPI2_RC_SUCCESS) {
		log(level::ERROR,
		    "Failed in p10_pibms_reg_dump for proc=%s, rc=0x%08X",
		    pdbg_target_path(proc), fapiRc);
		throw std::runtime_error("p10_pibms_reg_dump failed");
	}
	std::vector<DumpPIBMSRegVal> dumpRegs;
	for (auto& reg : pibmsRegSet) {
		dumpRegs.emplace_back(reg.reg.addr, reg.reg.name, reg.reg.attr,
				      reg.value);
	}

	std::string dumpFilename = baseFilename + "p10_pibms_reg_dump";
	std::filesystem::path basePath = dumpPath / dumpFilename;

	// Writing the PIBMS register values to a file
	writePIBMSRegValuesToFile(dumpRegs, basePath);
}

/**
 * @brief Writes PIBMEM data to a file.
 * @param[in] dumpData A vector of uint64_t values representing the PIBMEM data
 * to be written.
 * @param[in] basePath The filesystem path of the file to which the data will be
 * written.
 *
 * @throw std::runtime_error Throws if there is an error opening the file or
 * during the file write operation.
 */
void writePIBMEMDataToFile(const std::vector<uint64_t>& dumpData,
			   const std::filesystem::path& basePath)
{
	try {
		std::ofstream outfile{basePath,
				      std::ios::out | std::ios::binary};
		if (!outfile.good()) {
			throw std::runtime_error(
			    "Failed to open file for writing");
		}
		outfile.write(reinterpret_cast<const char*>(dumpData.data()),
			      sizeof(uint64_t) * dumpData.size());
	} catch (const std::exception& e) {
		log(level::ERROR, "Error writing PIBMEM data to file: %s",
		    e.what());
		throw;
	}
}

/**
 * @brief Collects and writes PIBMEM dump data for a given processor target.
 * @param[in] proc The processor target from which to collect the PIBMEM dump
 * data.
 * @param[in] dumpPath The filesystem path where the dump file will be written.
 * @param[in] baseFilename The base filename to use for the dump file. This name
 * will be used to form the complete file path along with the dumpPath.
 *
 * @throw std::runtime_error Throws if there is an error in collecting the
 * PIBMEM dump data or writing to the file.
 */
void collectPIBMEMDump(struct pdbg_target* proc,
		       const std::filesystem::path& dumpPath,
		       const std::string& baseFilename)
{
	// Define these constants and types as per your requirement
	const uint32_t pibmemDumpStartByte = 0;	      // Starting byte for dump
	const uint32_t pibmemDumpNumOfByte = 0x7D400; // Number of bytes to read
	const user_options userOptions =
	    INTERMEDIATE_TILL_INTERMEDIATE; // User options, define as per your
					    // code
	const bool eccEnable = false; // ECC enabled/disabled

	std::vector<array_data_t>
	    pibmemContents; // Type array_data_t should be defined in your code

	ReturnCode fapiRc =
	    p10_pibmem_dump(proc, pibmemDumpStartByte, pibmemDumpNumOfByte,
			    userOptions, pibmemContents, eccEnable);
	if (fapiRc != FAPI2_RC_SUCCESS) {
		log(level::ERROR,
		    "Failed in p10_pibmem_dump for proc=%s, rc=0x%08X",
		    pdbg_target_path(proc), fapiRc);
		throw std::runtime_error("p10_pibmem_dump failed");
	}
	std::vector<uint64_t> dumpData;
	for (auto& data : pibmemContents) {
		dumpData.push_back(data.read_data);
	}

	std::string dumpFilename = baseFilename + "p10_pibmem_dump";
	std::filesystem::path basePath = dumpPath / dumpFilename;

	// Writing the PIBMEM data to a file
	writePIBMEMDataToFile(dumpData, basePath);
}

/**
 * @brief Writes PPE (Programmable Processing Element) state data to a file.
 * @param[in] ppeState A vector of `DumpPPERegValue` structures containing the
 * PPE state data to be written.
 * @param[in] basePath The filesystem path of the file to which the data will be
 * written.
 *
 * @throw std::runtime_error Throws if there is an error opening the file or
 * during the file write operation.
 */
void writePPEStateToFile(const std::vector<DumpPPERegValue>& ppeState,
			 const std::filesystem::path& basePath)
{
	try {
		std::ofstream outfile{basePath,
				      std::ios::out | std::ios::binary};
		if (!outfile.good()) {
			throw std::runtime_error(
			    "Failed to open file for writing");
		}
		outfile.write(reinterpret_cast<const char*>(ppeState.data()),
			      sizeof(DumpPPERegValue) * ppeState.size());
	} catch (const std::exception& e) {
		log(level::ERROR, "Error writing PPE state data to file: %s",
		    e.what());
		throw;
	}
}

/**
 * @brief Collects and writes the state of the Programmable Processing Element
 * (PPE).
 * @param[in] proc The processor target from which to collect the PPE state.
 * @param[in] dumpPath The filesystem path where the dump file will be written.
 * @param[in] baseFilename The base filename to use for the dump file. This name
 * will be used to form the complete file path along with the dumpPath.
 *
 * @throw std::runtime_error Throws if there is an error in collecting the PPE
 * state or writing to the file.
 */
void collectPPEState(struct pdbg_target* proc,
		     const std::filesystem::path& dumpPath,
		     const std::string& baseFilename)
{
	PPE_DUMP_MODE mode = SNAPSHOT; // Define as per your requirement
	uint32_t instanceNum = 0;      // Instance number, set as needed
	PPE_TYPES type =
	    PPE_TYPE_SBE; // Set the PPE type as per your requirement

	std::vector<Reg32Value_t> ppeGprsValue, ppeSprsValue, ppeXirsValue;

	ReturnCode fapiRc =
	    p10_ppe_state(proc, type, instanceNum, mode, ppeGprsValue,
			  ppeSprsValue, ppeXirsValue);
	if (fapiRc != FAPI2_RC_SUCCESS) {
		log(level::ERROR,
		    "Failed in p10_ppe_state for proc=%s, rc=0x%08X",
		    pdbg_target_path(proc), fapiRc);
		throw std::runtime_error("p10_ppe_state failed");
	}

	std::vector<DumpPPERegValue> ppeState;
	for (auto& spr : ppeSprsValue) {
		ppeState.emplace_back(spr.number, spr.value);
	}
	for (auto& xir : ppeXirsValue) {
		ppeState.emplace_back(xir.number, xir.value);
	}
	for (auto& gpr : ppeGprsValue) {
		ppeState.emplace_back(gpr.number, gpr.value);
	}

	std::string dumpFilename = baseFilename + "p10_ppe_state";
	std::filesystem::path basePath = dumpPath / dumpFilename;

	// Writing the PPE state data to a file
	writePPEStateToFile(ppeState, basePath);
}

/**
 * @brief Finalizes the collection process.
 * @param[in] pib The PIB target for which the collection is being finalized.
 * @param[in] dumpPath The filesystem path where the dump was written. Used for
 * cleanup in case of failure.
 * @param[in] isSuccess A boolean flag indicating whether the preceding
 * collection steps were successful.
 */
void finalizeCollection(struct pdbg_target* pib,
			const std::filesystem::path& dumpPath, bool isSuccess)
{
	// Attempt to set the SBE state to the final state (e.g., FAILED)
	if (0 > sbe_set_state(pib, SBE_STATE_FAILED)) {
		log(level::ERROR, "Failed to set SBE state to FAILED");
		// Handle error if needed
	}

	// If the collection was not successful, clean up partial data
	if (!isSuccess) {
		try {
			std::filesystem::remove_all(dumpPath);
			log(level::INFO, "Cleaned up partial dump data due to "
					 "collection failure");
		} catch (const std::exception& e) {
			log(level::ERROR,
			    "Error cleaning up partial dump data: %s",
			    e.what());
		}
	}

	log(level::INFO, "Collection process completed");
}

void collectSBEDump(uint32_t id, uint32_t failingUnit,
		    const std::filesystem::path& dumpPath, const int sbeTypeId)
{
	if (sbeTypeId != static_cast<uint16_t>(10)) {
		throw dumpError_t(exception::HWP_EXECUTION_FAILED);
	}

	log(level::INFO,
	    "Collecting SBE dump: path=%s, id=%d, chip position=%d",
	    dumpPath.string().c_str(), id, failingUnit);

	std::stringstream ss;
	ss << std::setw(8) << std::setfill('0') << id;
	std::string baseFilename =
	    ss.str() + ".0_" + std::to_string(failingUnit) + "_SbeData_p10_";

	struct pdbg_target* proc = nullptr;
	struct pdbg_target* pib = nullptr;

	try {
		// Execute pre-collection steps and get the proc target
		initializePdbgLibEkb();

		proc = getProcFromFailingId(failingUnit);
		probeTarget(proc, "fsi");
		pib = probeTarget(proc, "pib");

		checkSbeState(pib);
		executeSbeExtractRc(proc, dumpPath);

		// Collect various dumps
		collectLocalRegDump(proc, dumpPath, baseFilename);
		collectPIBMSRegDump(proc, dumpPath, baseFilename);
		collectPIBMEMDump(proc, dumpPath, baseFilename);
		collectPPEState(proc, dumpPath, baseFilename);

		// Finalize the collection process
		finalizeCollection(pib, dumpPath,
				   true); // Indicate successful completion
		log(level::INFO, "SBE dump collection completed successfully");
	} catch (const std::exception& e) {
		log(level::ERROR, "Failed to collect the SBE dump: %s",
		    e.what());
		// In case of any exception, attempt to finalize with a failure
		// state
		if (proc) {
			pib = getPibTarget(proc);
			finalizeCollection(pib, dumpPath, false);
		}
		throw;
	}
}

} // namespace dump
} // namespace openpower::phal
