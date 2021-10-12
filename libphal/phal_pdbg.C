#include "libphal.H"
#include "log.H"
#include "phal_exception.H"
#include "utils_pdbg.H"

#include <attributes_info.H>

namespace openpower::phal::pdbg
{

using namespace openpower::phal::logging;
using namespace openpower::phal::utils::pdbg;

void init(pdbg_backend pdbgBackend, const int32_t logLevel,
	  std::string pdbgDtbPath)
{
	log(level::INFO, "PDBG Initilization started");

	// set PDBG Back-end
	if (!pdbg_set_backend(pdbgBackend, NULL)) {
		log(level::ERROR, "Failed to set pdbg back-end(%d)",
		    pdbgBackend);
		throw pdbgError_t(exception::PDBG_INIT_FAIL);
	}

	// Set PDBG_DTB environment variable
	if (setenv("PDBG_DTB", pdbgDtbPath.c_str(), 1)) {
		log(level::ERROR, "Failed to set pdbg DTB path(%s)",
		    pdbgDtbPath.c_str());
		throw pdbgError_t(exception::PDBG_INIT_FAIL);
	}

	// PDBG Target initialisation
	if (!pdbg_targets_init(NULL)) {
		log(level::ERROR, "Failed to do pdbg target init");
		throw pdbgError_t(exception::PDBG_INIT_FAIL);
	}

	// Set PDBG log level
	pdbg_set_loglevel(logLevel);
}

bool isTgtPresent(struct pdbg_target *target)
{
	ATTR_HWAS_STATE_Type hwasState;
	if (DT_GET_PROP(ATTR_HWAS_STATE, target, hwasState)) {
		log(level::ERROR, "Attribute [ATTR_HWAS_STATE] read failed");
		throw pdbgError_t(exception::DEVTREE_ATTR_READ_FAIL);
	}

	return hwasState.present;
}

bool isTgtFunctional(struct pdbg_target *target)
{
	ATTR_HWAS_STATE_Type hwasState;
	if (DT_GET_PROP(ATTR_HWAS_STATE, target, hwasState)) {
		log(level::ERROR, "Attribute [ATTR_HWAS_STATE] read failed");
		throw pdbgError_t(exception::DEVTREE_ATTR_READ_FAIL);
	}

	return hwasState.functional;
}

bool isPrimaryProc(struct pdbg_target *proc)
{
	ATTR_PROC_MASTER_TYPE_Type type;

	// Get processor type (Primary or Secondary)
	if (DT_GET_PROP(ATTR_PROC_MASTER_TYPE, proc, type)) {
		log(level::ERROR,
		    "Attribute [ATTR_PROC_MASTER_TYPE] get failed");
		throw pdbgError_t(exception::DEVTREE_ATTR_READ_FAIL);
	}
	if (type == ENUM_ATTR_PROC_MASTER_TYPE_ACTING_MASTER) {
		return true;
	} else {
		return false;
	}
}

struct pdbg_target *getPrimaryProc()
{
	struct pdbg_target *procTarget;
	pdbg_for_each_class_target("proc", procTarget)
	{
		if (isPrimaryProc(procTarget)) {
			return procTarget;
		}
		procTarget = nullptr;
	}
	// check valid primary processor is available
	if (procTarget == nullptr) {
		log(level::ERROR, "fail to get primary processor");
		throw pdbgError_t(exception::DEVTREE_PRI_PROC_NOT_FOUND);
	}
	return procTarget;
}

uint32_t getCFAM(struct pdbg_target *proc, const uint32_t addr)
{
	// Get fsi target.
	struct pdbg_target *fsi = getFsiTarget(proc);

	uint32_t val;
	if (fsi_read(fsi, addr, &val)) {
		log(level::ERROR, "CFAM(0x%X) on %s failed", addr,
		    pdbg_target_path(fsi));
		throw pdbgError_t(exception::PDBG_FSI_READ_FAIL);
	}
	return val;
}

void putCFAM(struct pdbg_target *proc, const uint32_t addr, const uint32_t val)
{
	// Get fsi target.
	struct pdbg_target *fsi = getFsiTarget(proc);

	if (fsi_write(fsi, addr, val)) {
		log(level::ERROR, "putCFAM(0x%X) update on %s failed", addr,
		    pdbg_target_path(fsi));
		throw pdbgError_t(exception::PDBG_FSI_WRITE_FAIL);
	}
}

bool isSbeVitalAttnActive(struct pdbg_target *proc)
{
	bool validAttn = false;
	uint32_t isrVal = 0xFFFFFFFF;  // Invalid isr value
	uint32_t isrMask = 0xFFFFFFFF; // Invalid isr mask
	constexpr uint32_t SBE_ATTN = 0x00000002;

	// get active attentions on processor
	isrVal = getCFAM(proc, 0x1007);
	if (isrVal == 0xFFFFFFFF) {
		log(level::ERROR, "Error: cfam read 0x1007 INVALID");
		throw pdbgError_t(exception::PDBG_FSI_READ_FAIL);
	}
	// get interrupt enabled special attentions mask
	isrMask = getCFAM(proc, 0x100D);
	if (isrMask == 0xFFFFFFFF) {
		log(level::ERROR, "Error: cfam read 0x1007 INVALID");
		throw pdbgError_t(exception::PDBG_FSI_READ_FAIL);
	}
	log(level::INFO, "ISR(%s):Value:0x%X Mask:0x%X", pdbg_target_path(proc),
	    isrVal, isrMask);

	// check attention active and not masked
	if ((isrVal & SBE_ATTN) && (isrMask & SBE_ATTN)) {
		//  attention active and not masked
		validAttn = true;
	}
	return validAttn;
}

} // namespace openpower::phal::pdbg
