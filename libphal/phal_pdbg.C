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

} // namespace openpower::phal::pdbg
