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
	struct pdbg_target* target = nullptr;
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
			if (!openpower::phal::sbe::is_ody_ocmb_chip(target)) {
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
 * @brief Initializes PDBG backend with KERNEL mode and libekb
 * Exceptions: PDBG_INIT_FAIL for any pdbg init related failure.
 */
void initializePdbgLibEkb()
{
	// Initialize PDBG with KERNEL backend
	pdbg::init(PDBG_BACKEND_KERNEL);

	if (libekb_init()) {
		log(level::ERROR, "libekb_init failed");
		throw std::runtime_error("libekb initialization failed");
	}
}

/**
 * @brief Checks the SBE state
 *
 * @param pib The pib target
 * @param target The target (OCMB or proc)
 */
void checkSbeState(struct pdbg_target* pib, struct pdbg_target* target)
{
	enum sbe_state state;

	// If the SBE dump is already collected return error
	if (sbe_get_state(pib, &state)) {
		log(level::ERROR, "Failed to read SBE state information (%s)",
		    pdbg_target_path(target));
		throw sbeError_t(exception::SBE_STATE_READ_FAIL);
	}

	if (state == SBE_STATE_FAILED) {
		log(level::ERROR,
		    "Dump is already collected from the "
		    "SBE on (%s)",
		    pdbg_target_path(target));
		throw sbeError_t(exception::SBE_DUMP_IS_ALREADY_COLLECTED);
	}
}

/**
 * @brief Extracts SBE RC from the corrupted chip and writes the info into a
 * dump file
 *
 * @param target The chip target
 * @param dumpPath The path of the dump file
 * @param sbeTypeId The chip type, i.e.; proc or OCMB
 */
void extractSbeRc(struct pdbg_target* target,
		  const std::filesystem::path& dumpPath, const int sbeTypeId)
{
	// Execute SBE extract rc to set up sdb bit for pibmem dump to work
	fapi2::ReturnCode fapiRc;
	P10_EXTRACT_SBE_RC::RETURN_ACTION recovAction = {};
	std::string targetTypeString;

	if (ODYSSEY_SBE_DUMP == sbeTypeId) {
		// ody_extract_sbe_rc is returning the error.
		fapiRc = ody_extract_sbe_rc(target);
		targetTypeString = "ocmb";
	} else if (PROC_SBE_DUMP == sbeTypeId) {
		// p10_extract_sbe_rc is returning the error along with
		// recovery action, so not checking the fapirc.
		fapiRc = p10_extract_sbe_rc(target, recovAction, true);
		targetTypeString = "proc";
	}
	log(level::INFO, "extract_sbe_rc for %s=%s returned rc=0x%08X ",
	    targetTypeString, pdbg_target_path(target), fapiRc);

	writeSbeData(dumpPath, fapiRc, recovAction);
}

/**
 * @brief Probes and returns the corresponding PIB target of the chip
 *
 * @param target The target chip
 * @param sbeTypeId The chip type number
 * @return struct pdbg_target* The pib target obtained
 */
struct pdbg_target* probePibTarget(struct pdbg_target* target,
				   const int sbeTypeId)
{
	// PIB target for executing HWP
	struct pdbg_target* pib = nullptr;
	if (sbeTypeId == PROC_SBE_DUMP) {
		char path[16];
		sprintf(path, "/proc%d/pib", pdbg_target_index(target));
		pib = pdbg_target_from_path(nullptr, path);
	} else if (sbeTypeId == ODYSSEY_SBE_DUMP) {
		pib = get_ody_pib_target(target);
	}
	if (!pib) {
		log(level::ERROR, "Failed to get PIB target for(%s)",
		    pdbg_target_path(target));
		throw dumpError_t(exception::PDBG_TARGET_NOT_OPERATIONAL);
	}
	// Probe PIB for HWP execution
	if (pdbg_target_probe(pib) != PDBG_TARGET_ENABLED) {
		log(level::ERROR, "Failed to prob PIB");
		throw dumpError_t(exception::PDBG_TARGET_NOT_OPERATIONAL);
	}
	return pib;
}

/**
 * @brief Initialize the PDBG and SBE to start collection
 * @param[in] failingUnit Position of the proc containing the failed SBE
 * @param[in] sbeTypeId The chip type number
 *
 * Exceptions: PDBG_INIT_FAIL for any pdbg init related failure.
 *             PDBG_TARGET_NOT_OPERATIONAL for invalid failing unit
 *             HWP_EXECUTION_FAILED if the extract rc procedure is failing
 */
struct pdbg_target* preCollection(const uint32_t failingUnit,
				  const std::filesystem::path& dumpPath,
				  const int sbeTypeId)
{
	initializePdbgLibEkb();
	// Find the proc target from the failing unit id
	auto chip = getTargetFromFailingId(failingUnit, sbeTypeId);
	auto pib = probePibTarget(chip, sbeTypeId);
	checkSbeState(pib, chip);
	extractSbeRc(chip, dumpPath, sbeTypeId);
	return chip;
}

/**
 * @brief Write the dump file to the path
 * @param[in] data Pointer to the data buffer
 * @param[in] len Length of the buffer
 * @param[in] dumpPath Path of the file
 *
 * Throws FILE_OPERATION_FAILED in the case of error
 */
void writeDumpFile(char* data, size_t len, std::filesystem::path& dumpPath)
{
	log(level::INFO, "writng dump data to file");
	std::ofstream outfile{dumpPath, std::ios::out | std::ios::binary};
	if (!outfile.good()) {
		int err = errno;
		log(level::ERROR,
		    "Failed to open the dump file for writing err=%d", err);
		throw dumpError_t(exception::FILE_OPERATION_FAILED);
	}
	outfile.exceptions(std::ifstream::failbit | std::ifstream::badbit |
			   std::ifstream::eofbit);
	try {
		outfile.write(data, len);
	} catch (const std::ofstream::failure& oe) {
		int err = errno;
		log(level::ERROR,
		    "Failed to write to dump file err=%d error message=%s", err,
		    oe.what());
		throw dumpError_t(exception::FILE_OPERATION_FAILED);
	}
	outfile.close();
}

/**
 * @brief Writes the dump data contents in the register into the dump file
 *
 * @tparam DumpRegVal A templated vector of various possible dump type registers
 * @param dumpRegs Dump register
 * @param basePath File base path
 * @param dumpType Type of dump, e.g; pibms/pibmem/sbe local reg etc
 */
template <typename DumpRegVal>
void writeDumpFileForDumpContents(std::vector<DumpRegVal>& dumpRegs,
				  std::filesystem::path& basePath,
				  const std::string_view dumpType)
{
	try {
		writeDumpFile(reinterpret_cast<char*>(&dumpRegs[0]),
			      sizeof(DumpRegVal) * dumpRegs.size(), basePath);
	} catch (const dumpError_t& e) {
		log(level::ERROR,
		    "Error in writing %s "
		    "file "
		    "errorMsg=%s",
		    dumpType.data(), e.what());
		throw;
	}
}

/**
 * @brief Collects the PPE state data for both p10 and Odyssey
 *
 * @param failingUnit Position of the proc containing the failed SBE
 * @param baseFilename Base file name of the dump
 * @param dumpPath The dump path
 * @param sbeTypeId The type of failed SBE chip, i.e.; p10 or Odyssey
 */
void collectPPEStateData(struct pdbg_target* target,
			 const std::string& baseFilename,
			 const std::filesystem::path& dumpPath,
			 const int sbeTypeId)
{
	// Dump the PPE state based on the based base address
	PPE_DUMP_MODE mode = SNAPSHOT;
	std::vector<Reg32Value_t> ppeGprsValue;
	std::vector<Reg32Value_t> ppeSprsValue;
	std::vector<Reg32Value_t> ppeXirsValue;
	PPE_TYPES type = PPE_TYPE_SBE; // By default it is for p10 chip type
	uint32_t instanceNum = 0;      // By default it is for p10 chip type
	auto chip = target;	       // By default it is for p10 chip type

	if (sbeTypeId == ODYSSEY_SBE_DUMP) {
		type = PPE_TYPE_SPPE;
		chip = pdbg_target_parent("proc", target);
		instanceNum = getFapiUnitPos(target);
	}

	// The processor chip should be of proc type while collecting
	// PPE dump data for both p10 and Odyssey as per the HWP team
	auto fapiRc = p10_ppe_state(chip, type, instanceNum, mode, ppeGprsValue,
				    ppeSprsValue, ppeXirsValue);
	if (fapiRc != fapi2::FAPI2_RC_SUCCESS) {
		if (sbeTypeId == ODYSSEY_SBE_DUMP)
			log(level::ERROR,
			    "Failed in ody_ppe_state for proc=%s, rc=0x%08X",
			    pdbg_target_path(chip), fapiRc);
		else
			log(level::ERROR,
			    "Failed in p10_ppe_state for proc=%s, rc=0x%08X",
			    pdbg_target_path(chip), fapiRc);
	} else {
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

		std::string dumpFilename =
		    baseFilename + ((sbeTypeId == ODYSSEY_SBE_DUMP)
					? "ody_ppe_state"
					: "p10_ppe_state");
		std::filesystem::path basePath = dumpPath / dumpFilename;
		auto dumpTypeName = (sbeTypeId == ODYSSEY_SBE_DUMP)
					? "ody_ppe_state"
					: "p10_ppe_state";
		writeDumpFileForDumpContents(ppeState, basePath, dumpTypeName);
	}
}

void collectSBEDumpForOdy(struct pdbg_target* ocmb, uint32_t id,
			  uint32_t failingUnit,
			  const std::filesystem::path& dumpPath)
{
	std::stringstream ss;
	ss << std::setw(8) << std::setfill('0') << id;

	// Filename format
	// <dumpID>.<nodeNUM>_<ocmbNum>.Sbedata_p10_<HWP name>
	std::string baseFilename =
	    ss.str() + ".0_" + std::to_string(failingUnit) + "_SbeData_ody_";

	// Collect SBE local register dump
	std::vector<SBE_SCOMReg_Value_t> sbeScomRegValue;
	auto fapiRc = ody_sbe_localreg_dump(ocmb, true, sbeScomRegValue);
	if (fapiRc != fapi2::FAPI2_RC_SUCCESS) {
		log(level::ERROR,
		    "Failed in ody_sbe_localreg_dump for ocmb=%s, "
		    "rc=0x%08X",
		    pdbg_target_path(ocmb), fapiRc);
	} else {
		std::vector<DumpSBERegVal> dumpRegs;
		for (auto& reg : sbeScomRegValue) {
			dumpRegs.emplace_back(reg.reg.number, reg.reg.name,
					      reg.value);
		}
		std::string dumpFilename =
		    baseFilename + "ody_sbe_localreg_dump";
		std::filesystem::path basePath = dumpPath / dumpFilename;

		writeDumpFileForDumpContents(dumpRegs, basePath,
					     "ody_sbe_localreg_dump");
	}

	// Dump contents of various PIB Masters and Slaves internal
	// registers
	std::vector<sRegVOdy> pibmsRegSet;
	for (auto& reg : pibms_regs_2dump_ody) {
		sRegVOdy regv;
		regv.reg = reg;
		pibmsRegSet.emplace_back(regv);
	}

	fapiRc = ody_pibms_reg_dump(ocmb, pibmsRegSet);
	if (fapiRc != fapi2::FAPI2_RC_SUCCESS) {
		log(level::ERROR,
		    "Failed in ody_pibms_reg_dump for ocmb=%s, "
		    "rc=0x%08X",
		    pdbg_target_path(ocmb), fapiRc);
	} else {
		std::vector<DumpPIBMSRegVal> dumpRegs;
		for (auto& regs : pibmsRegSet) {
			dumpRegs.emplace_back(regs.reg.addr, regs.reg.name,
					      regs.reg.attr, regs.value);
		}
		std::string dumpFilename = baseFilename + "ody_pibms_reg_dump";
		std::filesystem::path basePath = dumpPath / dumpFilename;
		writeDumpFileForDumpContents(dumpRegs, basePath,
					     "ody_pibms_reg_dump");
	}

	// Dump the PIBMEM Array based on starting and number of address
	std::vector<pibmem_array_data_t> pibmemContents;
	auto eccEnable = false;
	uint32_t pibmemDumpStartByte = 0;
	// Number of bytes to be read from PIBMEM
	static constexpr uint32_t pibmemDumpNumOfByte = 0x7D400;
	usr_options userOptions = INTERMEDIATE_TO_INTERMEDIATE;

	fapiRc = ody_pibmem_dump(ocmb, pibmemDumpStartByte, pibmemDumpNumOfByte,
				 userOptions, eccEnable, pibmemContents);
	if (fapiRc != fapi2::FAPI2_RC_SUCCESS) {
		log(level::ERROR,
		    "Failed in ody_pibmem_dump for ocmb=%s, rc=0x%08X",
		    pdbg_target_path(ocmb), fapiRc);
	} else {
		std::vector<uint64_t> dumpData;
		for (auto& data : pibmemContents) {
			dumpData.push_back(data.rd_data);
		}
		std::string dumpFilename = baseFilename + "ody_pibmem_dump";
		std::filesystem::path basePath = dumpPath / dumpFilename;
		writeDumpFileForDumpContents(dumpData, basePath,
					     "ody_pibmem_dump");
	}
	// Dump the PPE state based on the based base address
	collectPPEStateData(ocmb, baseFilename, dumpPath, ODYSSEY_SBE_DUMP);
}

void collectSBEDump(uint32_t id, uint32_t failingUnit,
		    const std::filesystem::path& dumpPath, const int sbeTypeId)
{
	log(level::INFO,
	    "Collecting SBE dump: path=%s, id=%d, chip position=%d",
	    dumpPath.string().c_str(), id, failingUnit);

	std::stringstream ss;
	ss << std::setw(8) << std::setfill('0') << id;

	// Filename format
	// <dumpID>.<nodeNUM>_<procNum>.Sbedata_p10_<HWP name>
	std::string baseFilename =
	    ss.str() + ".0_" + std::to_string(failingUnit) + "_SbeData_p10_";

	// Execute pre-collection and get chip corresponding to failing unit
	auto chip = preCollection(failingUnit, dumpPath, sbeTypeId);

	// Get pib for the chip
	auto pib = probePibTarget(chip, sbeTypeId);

	// Set SBE state to SBE_STATE_DEBUG_MODE
	if (0 > sbe_set_state(pib, SBE_STATE_DEBUG_MODE)) {
		log(level::ERROR, "Setting SBE state to debug mode failed");
		throw std::runtime_error(
		    "Setting SBE state to debug mode failed");
	}

	fapi2::ReturnCode fapiRc;

	try {
		if (ODYSSEY_SBE_DUMP == sbeTypeId) {
			collectSBEDumpForOdy(chip, id, failingUnit, dumpPath);
		} else {
			// Collect SBE local register dump
			std::vector<SBESCOMRegValue_t> sbeScomRegValue;
			fapiRc =
			    p10_sbe_localreg_dump(chip, true, sbeScomRegValue);
			if (fapiRc != fapi2::FAPI2_RC_SUCCESS) {
				log(level::ERROR,
				    "Failed in p10_sbe_localreg_dump for "
				    "proc=%s, "
				    "rc=0x%08X",
				    pdbg_target_path(chip), fapiRc);
			} else {
				std::vector<DumpSBERegVal> dumpRegs;
				for (auto& reg : sbeScomRegValue) {
					dumpRegs.emplace_back(reg.reg.number,
							      reg.reg.name,
							      reg.value);
				}
				std::string dumpFilename =
				    baseFilename + "p10_sbe_localreg_dump";
				std::filesystem::path basePath =
				    dumpPath / dumpFilename;

				writeDumpFileForDumpContents(
				    dumpRegs, basePath,
				    "p10_sbe_localreg_dump");
			}

			// Dump contents of various PIB Masters and Slaves
			// internal registers
			std::vector<sRegV> pibmsRegSet;
			for (auto& reg : pibms_regs_2dump) {
				sRegV regv;
				regv.reg = reg;
				pibmsRegSet.emplace_back(regv);
			}

			fapiRc = p10_pibms_reg_dump(chip, pibmsRegSet);
			if (fapiRc != fapi2::FAPI2_RC_SUCCESS) {
				log(level::ERROR,
				    "Failed in p10_pibms_reg_dump for proc=%s, "
				    "rc=0x%08X",
				    pdbg_target_path(chip), fapiRc);
			} else {
				std::vector<DumpPIBMSRegVal> dumpRegs;
				for (sRegV& regs : pibmsRegSet) {
					dumpRegs.emplace_back(
					    regs.reg.addr, regs.reg.name,
					    regs.reg.attr, regs.value);
				}
				std::string dumpFilename =
				    baseFilename + "p10_pibms_reg_dump";
				std::filesystem::path basePath =
				    dumpPath / dumpFilename;

				writeDumpFileForDumpContents(
				    dumpRegs, basePath, "p10_pibms_reg_dump");
			}

			// Dump the PIBMEM Array based on starting and number of
			// address
			std::vector<array_data_t> pibmemContents;
			bool eccEnable = false;
			uint32_t pibmemDumpStartByte = 0;
			// Number of bytes to be read from PIBMEM
			static constexpr uint32_t pibmemDumpNumOfByte = 0x7D400;
			user_options userOptions =
			    INTERMEDIATE_TILL_INTERMEDIATE;

			fapiRc = p10_pibmem_dump(
			    chip, pibmemDumpStartByte, pibmemDumpNumOfByte,
			    userOptions, pibmemContents, eccEnable);
			if (fapiRc != fapi2::FAPI2_RC_SUCCESS) {
				log(level::ERROR,
				    "Failed in p10_pibmem_dump for proc=%s, "
				    "rc=0x%08X",
				    pdbg_target_path(chip), fapiRc);
			} else {
				std::vector<uint64_t> dumpData;
				for (auto& data : pibmemContents) {
					dumpData.push_back(data.read_data);
				}
				std::string dumpFilename =
				    baseFilename + "p10_pibmem_dump";
				std::filesystem::path basePath =
				    dumpPath / dumpFilename;

				writeDumpFileForDumpContents(dumpData, basePath,
							     "p10_pibmem_dump");
			}
			// Dump the PPE state based on the based base address
			collectPPEStateData(chip, baseFilename, dumpPath,
					    PROC_SBE_DUMP);
		}
	} catch (const std::exception& e) {
		log(level::ERROR, "Failed to collect the SBE dump");
		if (0 > sbe_set_state(pib, SBE_STATE_FAILED)) {
			log(level::ERROR, "Failed to set SBE state to FAILED");
		}
		// Dump collection failed remove the directory
		// and partial contents
		std::filesystem::remove_all(dumpPath);
		// Rethrow the exception
		throw;
	}

	// Set SBE state to SBE_STATE_FAILED
	if (0 > sbe_set_state(pib, SBE_STATE_FAILED)) {
		log(level::ERROR, "Failed to set SBE state to FAILED");
	}
	log(level::INFO, "SBE dump collected");
}
} // namespace dump
} // namespace openpower::phal
