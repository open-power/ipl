extern "C" {
#include <libgen.h>
#include <libpdbg.h>
#include <libpdbg_sbe.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
}

#include "libphal.H"
#include "log.H"

#include <ekb/chips/p10/procedures/hwp/lib/p10_pibmem_dump.H>
#include <ekb/chips/p10/procedures/hwp/lib/p10_pibms_reg_dump.H>
#include <ekb/chips/p10/procedures/hwp/lib/p10_ppe_state.H>
#include <ekb/chips/p10/procedures/hwp/lib/p10_sbe_localreg_dump.H>
#include <ekb/chips/p10/procedures/hwp/lib/pibms_regs2dump.H>
#include <ekb/hwpf/fapi2/include/return_code_defs.H>
#include <fapi2.H>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace openpower::phal
{
namespace dump
{

using namespace openpower::phal::logging;

void preCollection(struct pdbg_target* proc)
{
	struct pdbg_target *fsi, *pib;
	char path[16];
	sprintf(path, "/proc%d/fsi", pdbg_target_index(proc));
	fsi = pdbg_target_from_path(NULL, path);
	assert(fsi);
	sprintf(path, "/proc%d/pib", pdbg_target_index(proc));
	pib = pdbg_target_from_path(NULL, path);
	assert(pib);

	if ((pdbg_target_probe(fsi) != PDBG_TARGET_ENABLED ||
	     pdbg_target_probe(pib) != PDBG_TARGET_ENABLED)) {
		pdbg_target_status_set(proc, PDBG_TARGET_DISABLED);
	}
}

int writeDumpFile(char* data, size_t len, std::filesystem::path& dumpPath)
{
	int rc = -1;
	std::ofstream outfile{dumpPath, std::ios::out | std::ios::binary};
	if (!outfile.good()) {
		int err = errno;
		log(level::INFO,
		    "Failed to open the dump file for writing err=%d", err);
		return rc;
	}
	outfile.exceptions(std::ifstream::failbit | std::ifstream::badbit |
			   std::ifstream::eofbit);
	try {
		outfile.write(data, len);
	} catch (std::ofstream::failure& oe) {
		int err = errno;
		log(level::INFO,
		    "Failed to write to dump file err=%d error message=%s", err,
		    oe.what());
		return rc;
	}
	outfile.close();
	return 0;
}

int collectSBEDump(uint32_t id, uint8_t failingUnit,
		   std::filesystem::path& dumpPath)
{

	int rc = -1;
	log(level::INFO,
	    "Collecting SBE dump: path=%s, id=%d, chip position=%d",
	    dumpPath.string().c_str(), id, failingUnit);
	pdbg_set_backend(PDBG_BACKEND_KERNEL, NULL);

	if (!pdbg_targets_init(NULL)) {
		log(level::ERROR, "pdbg_targets_init failed");
		throw std::runtime_error("pdbg target initialization failed");
	}
	pdbg_set_loglevel(PDBG_INFO);

	struct pdbg_target* target;
	struct pdbg_target* proc = NULL;
	pdbg_for_each_class_target("proc", target)
	{
		if (pdbg_target_probe(target) != PDBG_TARGET_ENABLED) {
			continue;
		}

		if (pdbg_target_index(proc) == failingUnit) {
			proc = target;
		}
	}
	if (proc == NULL) {
		log(level::ERROR, "No Proc target found to execute the dump");
		return rc;
	}

	std::stringstream ss;
	ss << std::setw(8) << std::setfill('0') << id;
	std::string idStr = ss.str();

	std::string baseFilename =
	    idStr + ".0_" + std::to_string(failingUnit) + "_SbeData_p10_";
	preCollection(proc);
	fapi2::ReturnCode fapirc;

	// Collect SBE local register dump
	std::vector<SBESCOMRegValue_t> sbeScomRegValue;
	fapirc = p10_sbe_localreg_dump(proc, true, sbeScomRegValue);
	if (fapirc != fapi2::FAPI2_RC_SUCCESS) {
		log(level::ERROR,
		    "Failed in p10_sbe_localreg_dump for proc=%s, rc=0x%08X",
		    pdbg_target_path(proc), fapirc);
	}

	std::string dumpFilename = baseFilename + "p10_sbe_localreg_dump";
	std::filesystem::path basePath = dumpPath / dumpFilename;
	rc = writeDumpFile(reinterpret_cast<char*>(&sbeScomRegValue[0]),
			   sizeof(SBESCOMRegValue_t) * sbeScomRegValue.size(),
			   basePath);
	if (rc < 0) {
		log(level::ERROR,
		    "Error in writing p10_sbe_localreg_dump file");
	}

	// Dump contents of various PIB Masters and Slaves internal registers
	std::vector<sRegV> l_pibms_reg_set;
	std::vector<sReg> regs2dump;
	if (pibms_regs_2dump.size()) {
		regs2dump.insert(regs2dump.end(), pibms_regs_2dump.begin(),
				 pibms_regs_2dump.end());
	}
	if (regs2dump.size()) {
		sRegV regv;
		for (auto& reg : regs2dump) {
			regv.reg.attr = reg.attr;
			regv.reg.addr = reg.addr;
			regv.reg.name = reg.name;
			l_pibms_reg_set.push_back(regv);
		}
	}

	fapirc = p10_pibms_reg_dump(proc, l_pibms_reg_set);
	if (fapirc != fapi2::FAPI2_RC_SUCCESS) {
		log(level::ERROR,
		    "Failed in p10_pibms_reg_dump for proc=%s, rc=0x%08X",
		    pdbg_target_path(proc), fapirc);
	}

	dumpFilename = baseFilename + "p10_pibms_reg_dump";
	basePath = dumpPath / dumpFilename;
	rc = writeDumpFile(reinterpret_cast<char*>(&l_pibms_reg_set[0]),
			   sizeof(sRegV) * l_pibms_reg_set.size(), basePath);
	if (rc < 0) {
		log(level::ERROR, "Error in writing p10_pibms_reg_dump file");
	}

	//
	std::vector<array_data_t> l_pibmem_contents;
	bool l_ecc_enable = false;
	uint32_t pibmem_dump_start_byte = 0;
	uint32_t pibmem_dump_num_of_byte = 0x7D400;
	user_options l_useroptions = INTERMEDIATE_TILL_INTERMEDIATE;

	fapirc = p10_pibmem_dump(proc, pibmem_dump_start_byte,
				 pibmem_dump_num_of_byte, l_useroptions,
				 l_pibmem_contents, l_ecc_enable);
	if (fapirc != fapi2::FAPI2_RC_SUCCESS) {
		log(level::ERROR,
		    "Failed in p10_pibmem_dump for proc=%s, rc=0x%08X",
		    pdbg_target_path(proc), fapirc);
	}

	dumpFilename = baseFilename + "p10_pibmem_dump";
	basePath = dumpPath / dumpFilename;
	writeDumpFile(reinterpret_cast<char*>(&l_pibmem_contents[0]),
		      sizeof(array_data_t) * l_pibmem_contents.size(),
		      basePath);
	if (rc < 0) {
		log(level::ERROR, "Error in writing p10_pibmem_dump file");
	}

	//
	PPE_DUMP_MODE mode = SNAPSHOT;
	uint32_t instanceNum = 0;
	std::vector<Reg32Value_t> l_ppe_gprs_value;
	std::vector<Reg32Value_t> l_ppe_sprs_value;
	std::vector<Reg32Value_t> l_ppe_xirs_value;
	std::vector<Reg32Value_t> l_ppe_state;
	PPE_TYPES type = PPE_TYPE_SBE;

	fapirc = p10_ppe_state(proc, type, instanceNum, mode, l_ppe_gprs_value,
			       l_ppe_sprs_value, l_ppe_xirs_value);
	if (fapirc != fapi2::FAPI2_RC_SUCCESS) {
		log(level::ERROR,
		    "Failed in p10_ppe_state for proc=%s, rc=0x%08X",
		    pdbg_target_path(proc), fapirc);
	}

	l_ppe_state.insert(l_ppe_state.end(), l_ppe_gprs_value.begin(),
			   l_ppe_gprs_value.end());
	l_ppe_state.insert(l_ppe_state.end(), l_ppe_sprs_value.begin(),
			   l_ppe_sprs_value.end());
	l_ppe_state.insert(l_ppe_state.end(), l_ppe_xirs_value.begin(),
			   l_ppe_xirs_value.end());

	dumpFilename = baseFilename + "p10_ppe_state";
	basePath = dumpPath / dumpFilename;
	writeDumpFile(reinterpret_cast<char*>(&l_ppe_state[0]),
		      sizeof(Reg32Value_t) * l_ppe_state.size(), basePath);
	if (rc < 0) {
		log(level::ERROR, "Error in writing p10_ppe_state file");
	}

	return 0;
}
} // namespace dump
} // namespace openpower::phal
