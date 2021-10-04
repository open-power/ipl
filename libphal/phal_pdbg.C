#include "libphal.H"
#include "log.H"
#include "phal_exception.H"

namespace openpower::phal::pdbg
{

using namespace openpower::phal::logging;

/**
 * @brief wrapper function to Initialises the pdbg targeting system based on
 *        user provided back-end and device tree.
 *
 */
void init(pdbg_backend pdbgBackend, const int32_t logLevel,
	  std::string pdbgDtbPath)
{
	log(level::INFO, "PDBG Initilization started");

	// set PDBG Back-end
	if (!pdbg_set_backend(pdbgBackend, NULL)) {
		log(level::ERROR, "Failed to set pdbg back-end(%d)",
		    pdbgBackend);
		throw sbeError_t(exception::PDBG_INIT_FAIL);
	}

	// Set PDBG_DTB environment variable
	if (setenv("PDBG_DTB", pdbgDtbPath.c_str(), 1)) {
		log(level::ERROR, "Failed to set pdbg DTB path(%s)",
		    pdbgDtbPath.c_str());
		throw sbeError_t(exception::PDBG_INIT_FAIL);
	}

	// PDBG Target initialisation
	if (!pdbg_targets_init(NULL)) {
		log(level::ERROR, "Failed to do pdbg target init");
		throw sbeError_t(exception::PDBG_INIT_FAIL);
	}

	// Set PDBG log level
	pdbg_set_loglevel(logLevel);
}

} // namespace openpower::phal::pdbg
