#include "libphal.H"
#include "log.H"
#include "phal_exception.H"

#include <attributes_info.H>

namespace openpower::phal::pdbg
{

using namespace openpower::phal::logging;

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

} // namespace openpower::phal::pdbg
