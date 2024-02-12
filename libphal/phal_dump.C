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
#include <ekb/chips/ocmb/odyssey/procedures/hwp/utils/ody_sbe_localreg_dump.H>
#include <ekb/chips/ocmb/odyssey/procedures/hwp/utils/ody_pibms_regs2dump.H>
#include <ekb/chips/ocmb/odyssey/procedures/hwp/utils/ody_pibmem_dump.H>
#include <ekb/chips/ocmb/odyssey/procedures/hwp/utils/ody_pibms_reg_dump.H>
#include <ekb/chips/ocmb/odyssey/procedures/hwp/utils/ody_ppe_state.H>
#include <ekb/chips/ocmb/odyssey/procedures/hwp/perv/ody_extract_sbe_rc.H>

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

// Define the chip types numbers
static constexpr int PROC_SBE_DUMP = 0xA;
static constexpr int ODYSSEY_SBE_DUMP = 0xB;

/**
 * @brief Get the Fapi Unit Pos object for OCMB
 *
 * @param target The target
 * @return uint8_t The position
 */
uint32_t getFapiUnitPos(pdbg_target* target)
{
	uint32_t fapiUnitPos = 0; // chip unit position
	if (!pdbg_target_get_attribute(target, "ATTR_FAPI_POS", 4, 1,
				       &fapiUnitPos))
		log(level::ERROR, "ATTR_FAPI_POS Attribute get failed");

	return fapiUnitPos;
}

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
					   const int sbeTypeId)
{
	struct pdbg_target* chip = nullptr;
	struct pdbg_target* target;
	std::string chipTypeString;

	if (sbeTypeId == ODYSSEY_SBE_DUMP)
		chipTypeString = "ocmb";
	else if (sbeTypeId == PROC_SBE_DUMP)
		chipTypeString = "proc";

	pdbg_for_each_class_target(chipTypeString.c_str(), target)
	{
		uint8_t targetIdx = 0;
		if (sbeTypeId == ODYSSEY_SBE_DUMP)
			targetIdx = getFapiUnitPos(target);
		else if (sbeTypeId == PROC_SBE_DUMP)
			targetIdx = pdbg_target_index(target);

		if (targetIdx != failingUnit) {
			continue;
		}
		if (sbeTypeId == ODYSSEY_SBE_DUMP) {
			if (!is_ody_ocmb_chip(target)) {
				log(level::ERROR,
				    "No ocmb target found to execute the dump "
				    "for failing unit=%d",
				    failingUnit);
				throw sbeError_t(
				    exception::PDBG_TARGET_NOT_OPERATIONAL);
			}
		}
		if (pdbg_target_probe(target) != PDBG_TARGET_ENABLED) {
			break;
		}
		chip = target;
		break;
	}
	if (!chip) {
		log(level::ERROR,
		    "No chip target found to execute the dump for failing "
		    "unit=%d",
		    failingUnit);
		throw sbeError_t(exception::PDBG_TARGET_NOT_OPERATIONAL);
	}
	return chip;
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
				const char* targetType, const int sbeTypeId)
{
	if (sbeTypeId == PROC_SBE_DUMP) {
		char path[16];
		sprintf(path, "/proc%d/%s", pdbg_target_index(proc),
			targetType);
		struct pdbg_target* target = pdbg_target_from_path(NULL, path);
		if (!target ||
		    pdbg_target_probe(target) != PDBG_TARGET_ENABLED) {
			log(level::ERROR,
			    "Failed to get or probe %s target for (%s)",
			    targetType, pdbg_target_path(proc));
			throw dumpError_t(
			    exception::PDBG_TARGET_NOT_OPERATIONAL);
		}
		return target;
	} else if (sbeTypeId == ODYSSEY_SBE_DUMP) {
		struct pdbg_target* target = nullptr;
		if (0 == std::string("pib").compare(targetType))
			target = get_ody_pib_target(proc);
		else
			target = get_ody_fsi_target(proc);

		if (!target) {
			log(level::ERROR, "Failed to get %s target for(%s)",
			    targetType, pdbg_target_path(target));
			throw dumpError_t(
			    exception::PDBG_TARGET_NOT_OPERATIONAL);
		}
		// Probe target for HWP execution
		if (pdbg_target_probe(target) != PDBG_TARGET_ENABLED) {
			log(level::ERROR, "Failed to prob %s", targetType);
			throw dumpError_t(
			    exception::PDBG_TARGET_NOT_OPERATIONAL);
		}
		return target;
	} else {
		return nullptr;
	}
}

/**
 * @brief Checks the state of the SBE on the given PIB target.
 * @param[in] pib_fsi The PIB or FSI target to check the SBE state of.
 * @param[in] sbeTypeId The chip type ID
 *
 * @throw sbeError_t Throws if the SBE state is 'FAILED' (indicating a dump
 *        has already been collected) or if there is a failure in reading the
 * SBE state.
 */
void checkSbeState(struct pdbg_target* pib_fsi, const int sbeTypeId)
{
	enum sbe_state state;
	auto rv = -1;
	if (PROC_SBE_DUMP == sbeTypeId)
		rv = sbe_get_state(pib_fsi, &state);
	else if (ODYSSEY_SBE_DUMP == sbeTypeId)
		rv = sbe_ody_get_state(pib_fsi, &state);

	if (rv) {
		log(level::ERROR, "Failed to read SBE state information (%s)",
		    pdbg_target_path(pib_fsi));
		throw sbeError_t(exception::SBE_STATE_READ_FAIL);
	}
	if (state == SBE_STATE_FAILED) {
		log(level::ERROR,
		    "Dump is already collected from the SBE on (%s)",
		    pdbg_target_path(pib_fsi));
		throw sbeError_t(exception::SBE_DUMP_IS_ALREADY_COLLECTED);
	}
}

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
			 const int sbeTypeId)
{
	// Execute SBE extract rc to set up sdb bit for pibmem dump to work
	P10_EXTRACT_SBE_RC::RETURN_ACTION recovAction = {};
	std::string targetTypeString;
	ReturnCode fapiRc;

	if (ODYSSEY_SBE_DUMP == sbeTypeId) {
		// ody_extract_sbe_rc is returning the error.
		fapiRc = ody_extract_sbe_rc(target, true, false);
		targetTypeString = "ocmb";
	} else if (PROC_SBE_DUMP == sbeTypeId) {
		// p10_extract_sbe_rc is returning the error along with
		// recovery action, so not checking the fapirc.
		fapiRc = p10_extract_sbe_rc(target, recovAction, true);
		targetTypeString = "proc";
	}
	log(level::INFO,
	    "extract_sbe_rc for target=%s returned rc=0x%08X and SBE "
	    "Recovery Action=0x%08X",
	    pdbg_target_path(target), fapiRc, recovAction);
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
			 const std::string& baseFilename, const int sbeTypeId)
{
	// Collect SBE local register dump
	std::vector<SBESCOMRegValue_t> sbeScomRegValue;
	std::vector<SBE_SCOMReg_Value_t> sbeScomRegValueOdy;
	std::string hwpName;
	ReturnCode fapiRc;

	if (ODYSSEY_SBE_DUMP == sbeTypeId) {
		hwpName = "ody_sbe_localreg_dump";
		fapiRc =
		    ody_sbe_localreg_dump(target, true, sbeScomRegValueOdy);
	} else if (PROC_SBE_DUMP == sbeTypeId) {
		hwpName = "p10_sbe_localreg_dump";
		fapiRc = p10_sbe_localreg_dump(target, true, sbeScomRegValue);
	}
	if (fapiRc != FAPI2_RC_SUCCESS) {
		log(level::ERROR, "Failed in %s for proc=%s, rc=0x%08X", target,
		    pdbg_target_path(target), fapiRc);
		throw std::runtime_error(hwpName + " failed");
	}
	std::vector<DumpSBERegVal> dumpRegs;
	// As both sbeScomRegValueOdy and sbeScomRegValue can not have data
	// at the same time thus
	if (!sbeScomRegValueOdy.empty()) {
		for (auto& reg : sbeScomRegValueOdy)
			dumpRegs.emplace_back(reg.reg.number, reg.reg.name,
					      reg.value);
	} else if (!sbeScomRegValue.empty()) {
		for (auto& reg : sbeScomRegValue)
			dumpRegs.emplace_back(reg.reg.number, reg.reg.name,
					      reg.value);
	}
	std::string dumpFilename = baseFilename + hwpName;
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
			 const std::string& baseFilename, const int sbeTypeId)
{
	std::vector<sRegV> pibmsRegSet;
	std::string hwpName;
	std::vector<sRegVOdy> pibmsRegSetOdy;
	ReturnCode fapiRc;

	if (PROC_SBE_DUMP == sbeTypeId) {
		for (auto& reg : pibms_regs_2dump) {
			sRegV regv;
			regv.reg = reg;
			pibmsRegSet.emplace_back(regv);
		}
		fapiRc = p10_pibms_reg_dump(target, pibmsRegSet);
		hwpName = "p10_pibms_reg_dump";
	} else if (ODYSSEY_SBE_DUMP == sbeTypeId) {
		for (auto& reg : pibms_regs_2dump_ody) {
			sRegVOdy regv;
			regv.reg = reg;
			pibmsRegSetOdy.emplace_back(regv);
		}
		fapiRc = ody_pibms_reg_dump(target, pibmsRegSetOdy);
		hwpName = "ody_pibms_reg_dump";
	}
	if (fapiRc != FAPI2_RC_SUCCESS) {
		log(level::ERROR, "Failed in %s for proc=%s, rc=0x%08X",
		    hwpName, pdbg_target_path(target), fapiRc);
		throw std::runtime_error(hwpName + " failed");
	}
	std::vector<DumpPIBMSRegVal> dumpRegs;
	// As both pibmsRegSetOdy and pibmsRegSet can not have data at the
	// same time thus
	if (!pibmsRegSet.empty()) {
		for (auto& reg : pibmsRegSet) {
			dumpRegs.emplace_back(reg.reg.addr, reg.reg.name,
					      reg.reg.attr, reg.value);
		}
	} else if (!pibmsRegSetOdy.empty()) {
		for (auto& regs : pibmsRegSetOdy) {
			dumpRegs.emplace_back(regs.reg.addr, regs.reg.name,
					      regs.reg.attr, regs.value);
		}
	}
	std::string dumpFilename = baseFilename + hwpName;
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
		       const std::string& baseFilename, const int sbeTypeId)
{
	// Define these constants and types as per your requirement
	const uint32_t pibmemDumpStartByte = 0;	      // Starting byte for dump
	const uint32_t pibmemDumpNumOfByte = 0x7D400; // Number of bytes to read
	// Number of bytes (512KB) to read for Odyssey
	constexpr uint32_t pibmemDumpNumOfByteOdy = 0x80000;
	const user_options userOptions =
	    INTERMEDIATE_TILL_INTERMEDIATE; // User options, define as per your
	const usr_options userOptionsOdy = INTERMEDIATE_TO_INTERMEDIATE;
	// code
	const bool eccEnable = false; // ECC enabled/disabled

	std::vector<array_data_t>
	    pibmemContents; // Type array_data_t should be defined in your code
	std::vector<pibmem_array_data_t> pibmemContentsOdy;
	ReturnCode fapiRc;
	std::string hwpName;

	if (PROC_SBE_DUMP == sbeTypeId) {
		fapiRc = p10_pibmem_dump(target, pibmemDumpStartByte,
					 pibmemDumpNumOfByte, userOptions,
					 pibmemContents, eccEnable);
		hwpName = "p10_pibmem_dump";
	} else if (ODYSSEY_SBE_DUMP == sbeTypeId) {
		fapiRc = ody_pibmem_dump(target, pibmemDumpStartByte,
					 pibmemDumpNumOfByteOdy, userOptionsOdy,
					 eccEnable, pibmemContentsOdy);
		hwpName = "ody_pibmem_dump";
	}
	if (fapiRc != FAPI2_RC_SUCCESS) {
		log(level::ERROR, "Failed in %s for proc=%s, rc=0x%08X",
		    hwpName, pdbg_target_path(target), fapiRc);
		throw std::runtime_error(hwpName + " failed");
	}
	std::vector<uint64_t> dumpData;
	// As both pibmemContentsOdy and pibmemContents can not have data at
	// the same time thus
	if (!pibmemContents.empty()) {
		for (auto& data : pibmemContents)
			dumpData.push_back(data.read_data);
	} else if (!pibmemContentsOdy.empty()) {
		for (auto& data : pibmemContentsOdy)
			dumpData.push_back(data.rd_data);
	}
	std::string dumpFilename = baseFilename + hwpName;
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
		     const std::string& baseFilename, const int sbeTypeId)
{
	PPE_DUMP_MODE mode = SNAPSHOT; // Define as per your requirement
	ODY_PPE_DUMP_MODE modeOdy = O_SNAPSHOT;
	uint32_t instanceNum = 0; // Instance number, set as needed
	PPE_TYPES type =
	    PPE_TYPE_SBE; // Set the PPE type as per your requirement

	std::vector<Reg32Value_t> ppeGprsValue, ppeSprsValue, ppeXirsValue;
	std::vector<Reg32Val_t> ppeGprsValueOdy, ppeSprsValueOdy,
	    ppeXirsValueOdy;
	std::string hwpName;
	ReturnCode fapiRc;

	if (sbeTypeId == PROC_SBE_DUMP) {
		fapiRc =
		    p10_ppe_state(target, type, instanceNum, mode, ppeGprsValue,
				  ppeSprsValue, ppeXirsValue);
		hwpName = "p10_ppe_state";
	} else if (sbeTypeId == ODYSSEY_SBE_DUMP) {
		fapiRc = ody_ppe_state(target, type, instanceNum, modeOdy,
				       ppeGprsValueOdy, ppeSprsValueOdy,
				       ppeXirsValueOdy);
		hwpName = "ody_ppe_state";
	}

	if (fapiRc != FAPI2_RC_SUCCESS) {
		log(level::ERROR, "Failed in %s for proc=%s, rc=0x%08X",
		    hwpName, pdbg_target_path(target), fapiRc);
		throw std::runtime_error(hwpName + " failed");
	}

	std::vector<DumpPPERegValue> ppeState;
	if (sbeTypeId == PROC_SBE_DUMP) {
		for (auto& spr : ppeSprsValue) {
			ppeState.emplace_back(spr.number, spr.value);
		}
		for (auto& xir : ppeXirsValue) {
			ppeState.emplace_back(xir.number, xir.value);
		}
		for (auto& gpr : ppeGprsValue) {
			ppeState.emplace_back(gpr.number, gpr.value);
		}
	} else if (sbeTypeId == ODYSSEY_SBE_DUMP) {
		for (auto& spr : ppeSprsValueOdy) {
			ppeState.emplace_back(spr.number, spr.value);
		}
		for (auto& xir : ppeXirsValueOdy) {
			ppeState.emplace_back(xir.number, xir.value);
		}
		for (auto& gpr : ppeGprsValueOdy) {
			ppeState.emplace_back(gpr.number, gpr.value);
		}
	}

	std::string dumpFilename = baseFilename + hwpName;
	std::filesystem::path basePath = dumpPath / dumpFilename;

	// Writing the PPE state data to a file
	writePPEStateToFile(ppeState, basePath);
}

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
			const int sbeTypeId)
{
	// Attempt to set the SBE state to the final state (e.g., FAILED)
	auto rv = -1;
	if (PROC_SBE_DUMP == sbeTypeId)
		rv = sbe_set_state(pib_fsi, SBE_STATE_FAILED);
	else if (ODYSSEY_SBE_DUMP == sbeTypeId)
		rv = sbe_ody_set_state(pib_fsi, SBE_STATE_FAILED);

	if (0 > rv) {
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
	log(level::INFO,
	    "Collecting SBE dump: path=%s, id=%d, chip position=%d",
	    dumpPath.string().c_str(), id, failingUnit);

	std::stringstream ss;
	ss << std::setw(8) << std::setfill('0') << id;

	std::string sbeChipType;
	if (PROC_SBE_DUMP == sbeTypeId)
		sbeChipType = "_p10_";
	else if (ODYSSEY_SBE_DUMP == sbeTypeId)
		sbeChipType = "_ody_";

	std::string baseFilename = ss.str() + ".0_" +
				   std::to_string(failingUnit) + "_SbeData" +
				   sbeChipType;

	struct pdbg_target* proc_ody = nullptr;
	struct pdbg_target* pib = nullptr;
	struct pdbg_target* fsi = nullptr;

	try {
		// Execute pre-collection steps and get the proc target
		initializePdbgLibEkb();

		proc_ody = getTargetFromFailingId(failingUnit, sbeTypeId);
		pib = probeTarget(proc_ody, "pib", sbeTypeId);
		fsi = probeTarget(proc_ody, "fsi", sbeTypeId);

		if (PROC_SBE_DUMP == sbeTypeId)
			checkSbeState(pib, sbeTypeId);
		else if (ODYSSEY_SBE_DUMP == sbeTypeId)
			checkSbeState(fsi, sbeTypeId);

		executeSbeExtractRc(proc_ody, dumpPath, sbeTypeId);

		// Collect various dumps
		collectLocalRegDump(proc_ody, dumpPath, baseFilename,
				    sbeTypeId);
		collectPIBMSRegDump(proc_ody, dumpPath, baseFilename,
				    sbeTypeId);
		collectPIBMEMDump(proc_ody, dumpPath, baseFilename, sbeTypeId);
		collectPPEState(proc_ody, dumpPath, baseFilename, sbeTypeId);

		// Finalize the collection process
		if (PROC_SBE_DUMP == sbeTypeId)
			finalizeCollection(
			    pib, dumpPath, true,
			    sbeTypeId); // Indicate successful completion
		else if (ODYSSEY_SBE_DUMP == sbeTypeId)
			finalizeCollection(
			    fsi, dumpPath, true,
			    sbeTypeId); // Indicate successful completion

		log(level::INFO, "SBE dump collection completed successfully");
	} catch (const std::exception& e) {
		log(level::ERROR, "Failed to collect the SBE dump: %s",
		    e.what());
		// In case of any exception, attempt to finalize with a failure
		// state
		if (proc_ody) {
			if (PROC_SBE_DUMP == sbeTypeId) {
				pib = probeTarget(proc_ody, "pib", sbeTypeId);
				finalizeCollection(pib, dumpPath, false,
						   sbeTypeId);
			} else if (ODYSSEY_SBE_DUMP == sbeTypeId) {
				fsi = probeTarget(proc_ody, "fsi", sbeTypeId);
				finalizeCollection(fsi, dumpPath, false,
						   sbeTypeId);
			}
		}
		throw;
	}
}

} // namespace dump
} // namespace openpower::phal
