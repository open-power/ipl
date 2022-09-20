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
 * @brief Initialize the PDBG and SBE to start collection
 * @param[in] failingUnit Position of the proc containing the failed SBE
 *
 * Exceptions: PDBG_INIT_FAIL for any pdbg init related failure.
 *             PDBG_TARGET_NOT_OPERATIONAL for invalid failing unit
 *             HWP_EXECUTION_FAILED if the extract rc procedure is failing
 */
struct pdbg_target* preCollection(const uint32_t failingUnit,
				  const std::filesystem::path& dumpPath)
{
	// Initialize PDBG with KERNEL backend
	pdbg::init(PDBG_BACKEND_KERNEL);

	if (libekb_init()) {
		log(level::ERROR, "libekb_init failed");
		throw std::runtime_error("libekb initialization failed");
	}

	// FSI and PIB targets for executing HWP
	struct pdbg_target *fsi, *pib;
	char path[16];

	// Find the proc target from the failing unit id
	struct pdbg_target* proc = getProcFromFailingId(failingUnit);

	// Probe FSI for HWP execution
	sprintf(path, "/proc%d/fsi", pdbg_target_index(proc));
	fsi = pdbg_target_from_path(NULL, path);
	if (fsi == nullptr) {
		log(level::ERROR, "Failed to get FSI target for(%s)",
		    pdbg_target_path(proc));
		throw dumpError_t(exception::PDBG_TARGET_NOT_OPERATIONAL);
	}

	// Probe PIB for HWP execution
	sprintf(path, "/proc%d/pib", pdbg_target_index(proc));
	pib = pdbg_target_from_path(NULL, path);
	if (pib == nullptr) {
		log(level::ERROR, "Failed to get PIB target for(%s)",
		    pdbg_target_path(proc));
		throw dumpError_t(exception::PDBG_TARGET_NOT_OPERATIONAL);
	}

	if ((pdbg_target_probe(fsi) != PDBG_TARGET_ENABLED ||
	     pdbg_target_probe(pib) != PDBG_TARGET_ENABLED)) {
		log(level::ERROR, "Failed to prob PIB or FSI");
		throw dumpError_t(exception::PDBG_TARGET_NOT_OPERATIONAL);
	}

	// Execute SBE extract rc to set up sdb bit for pibmem dump to work
	// TODO Add error handling or revisit procedure later
	fapi2::ReturnCode fapiRc;
	P10_EXTRACT_SBE_RC::RETURN_ACTION recovAction;

	// p10_extract_sbe_rc is returning the error along with
	// recovery action, so not checking the fapirc.
	fapiRc = p10_extract_sbe_rc(proc, recovAction, true);
	log(level::INFO,
	    "p10_extract_sbe_rc for proc=%s returned rc=0x%08X and SBE "
	    "Recovery Action=0x%08X",
	    pdbg_target_path(proc), fapiRc, recovAction);

	writeSbeData(dumpPath, fapiRc, recovAction);

	return proc;
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

void collectSBEDump(uint32_t id, uint32_t failingUnit,
		    const std::filesystem::path& dumpPath)
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

	// Execute pre-collection and get proc corresponding to failing unit
	struct pdbg_target* proc = preCollection(failingUnit, dumpPath);

	// Get pib for the proc
	struct pdbg_target* pib = getPibTarget(proc);

	// Set SBE state to SBE_STATE_DEBUG_MODE
	if (0 > sbe_set_state(pib, SBE_STATE_DEBUG_MODE)) {
		log(level::ERROR, "Setting SBE state to debug mode failed");
		throw std::runtime_error(
		    "Setting SBE state to debug mode failed");
	}

	fapi2::ReturnCode fapiRc;

	try {
		// Collect SBE local register dump
		std::vector<SBESCOMRegValue_t> sbeScomRegValue;
		fapiRc = p10_sbe_localreg_dump(proc, true, sbeScomRegValue);
		if (fapiRc != fapi2::FAPI2_RC_SUCCESS) {
			log(level::ERROR,
			    "Failed in p10_sbe_localreg_dump for proc=%s, "
			    "rc=0x%08X",
			    pdbg_target_path(proc), fapiRc);
		} else {
			std::vector<DumpSBERegVal> dumpRegs;
			for (auto& reg : sbeScomRegValue) {
				dumpRegs.emplace_back(reg.reg.number,
						      reg.reg.name, reg.value);
			}
			std::string dumpFilename =
			    baseFilename + "p10_sbe_localreg_dump";
			std::filesystem::path basePath =
			    dumpPath / dumpFilename;

			try {
				writeDumpFile(
				    reinterpret_cast<char*>(&dumpRegs[0]),
				    sizeof(DumpSBERegVal) * dumpRegs.size(),
				    basePath);
			} catch (const dumpError_t& e) {
				log(level::ERROR,
				    "Error in writing p10_sbe_localreg_dump "
				    "file "
				    "errorMsg=%s",
				    e.what());
				throw;
			}
		}

		// Dump contents of various PIB Masters and Slaves internal
		// registers
		std::vector<sRegV> pibmsRegSet;
		for (auto& reg : pibms_regs_2dump) {
			sRegV regv;
			regv.reg = reg;
			pibmsRegSet.emplace_back(regv);
		}

		fapiRc = p10_pibms_reg_dump(proc, pibmsRegSet);
		if (fapiRc != fapi2::FAPI2_RC_SUCCESS) {
			log(level::ERROR,
			    "Failed in p10_pibms_reg_dump for proc=%s, "
			    "rc=0x%08X",
			    pdbg_target_path(proc), fapiRc);
		} else {
			std::vector<DumpPIBMSRegVal> dumpRegs;
			for (sRegV& regs : pibmsRegSet) {
				dumpRegs.emplace_back(
				    regs.reg.addr, regs.reg.name, regs.reg.attr,
				    regs.value);
			}
			std::string dumpFilename =
			    baseFilename + "p10_pibms_reg_dump";
			std::filesystem::path basePath =
			    dumpPath / dumpFilename;
			try {
				writeDumpFile(
				    reinterpret_cast<char*>(&dumpRegs[0]),
				    sizeof(DumpPIBMSRegVal) * dumpRegs.size(),
				    basePath);
			} catch (const dumpError_t& e) {
				log(level::ERROR,
				    "Error in writing p10_pibms_reg_dump file, "
				    "errorMsg=%s",
				    e.what());
				throw;
			}
		}

		// Dump the PIBMEM Array based on starting and number of address
		std::vector<array_data_t> pibmemContents;
		bool eccEnable = false;
		uint32_t pibmemDumpStartByte = 0;
		// Number of bytes to be read from PIBMEM
		static constexpr uint32_t pibmemDumpNumOfByte = 0x7D400;
		user_options userOptions = INTERMEDIATE_TILL_INTERMEDIATE;

		fapiRc = p10_pibmem_dump(proc, pibmemDumpStartByte,
					 pibmemDumpNumOfByte, userOptions,
					 pibmemContents, eccEnable);
		if (fapiRc != fapi2::FAPI2_RC_SUCCESS) {
			log(level::ERROR,
			    "Failed in p10_pibmem_dump for proc=%s, rc=0x%08X",
			    pdbg_target_path(proc), fapiRc);
		} else {
			std::vector<uint64_t> dumpData;
			for (auto& data : pibmemContents) {
				dumpData.push_back(data.read_data);
			}
			std::string dumpFilename =
			    baseFilename + "p10_pibmem_dump";
			std::filesystem::path basePath =
			    dumpPath / dumpFilename;
			try {
				writeDumpFile(
				    reinterpret_cast<char*>(&dumpData[0]),
				    sizeof(uint64_t) * dumpData.size(),
				    basePath);
			} catch (const dumpError_t& e) {
				log(level::ERROR,
				    "Error in writing p10_pibmem_dump file "
				    "errorMsg=%s",
				    e.what());
				throw;
			}
		}

		// Dump the PPE state based on the based base address
		PPE_DUMP_MODE mode = SNAPSHOT;
		uint32_t instanceNum = 0;
		std::vector<Reg32Value_t> ppeGprsValue;
		std::vector<Reg32Value_t> ppeSprsValue;
		std::vector<Reg32Value_t> ppeXirsValue;
		PPE_TYPES type = PPE_TYPE_SBE;

		fapiRc =
		    p10_ppe_state(proc, type, instanceNum, mode, ppeGprsValue,
				  ppeSprsValue, ppeXirsValue);
		if (fapiRc != fapi2::FAPI2_RC_SUCCESS) {
			log(level::ERROR,
			    "Failed in p10_ppe_state for proc=%s, rc=0x%08X",
			    pdbg_target_path(proc), fapiRc);
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
			    baseFilename + "p10_ppe_state";
			std::filesystem::path basePath =
			    dumpPath / dumpFilename;
			try {
				writeDumpFile(
				    reinterpret_cast<char*>(&ppeState[0]),
				    sizeof(DumpPPERegValue) * ppeState.size(),
				    basePath);
			} catch (const dumpError_t& e) {
				log(level::ERROR,
				    "Error in writing p10_ppe_state file, "
				    "errorMsg=%s",
				    e.what());
				throw;
			}
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
