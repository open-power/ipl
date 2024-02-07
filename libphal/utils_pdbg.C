#include "log.H"
#include "phal_exception.H"
#include "utils_pdbg.H"

#include <cstring>
#include <sstream>

namespace openpower::phal::utils
{
namespace pdbg
{

using namespace openpower::phal::logging;
using namespace openpower::phal;

/**
 * Used to return in pdbg tarverse callback function
 *
 * The value for constexpr defined based on pdbg_target_traverse function usage.
 */
constexpr int continueTgtTraversal = 0;
constexpr int requireTgtFound = 1;

/**
 * Used to pass buffer to pdbg callback api to get required target
 * info based on given data (attribute).
 */
struct TargetInfo {
	ATTR_PHYS_BIN_PATH_Type physBinPath;
	struct pdbg_target *target;

	TargetInfo()
	{
		memset(&physBinPath, 0, sizeof(physBinPath));
		target = nullptr;
	}
};

struct pdbg_target *getPibTarget(struct pdbg_target *proc)
{
	char path[16];
	sprintf(path, "/proc%d/pib", pdbg_target_index(proc));
	struct pdbg_target *pib = pdbg_target_from_path(nullptr, path);
	if (pib == nullptr) {
		log(level::ERROR, "Failed to get PIB target for(%s)",
		    pdbg_target_path(proc));
		throw pdbgError_t(exception::PDBG_TARGET_NOT_OPERATIONAL);
	}
	// Probe the target
	if (pdbg_target_probe(pib) != PDBG_TARGET_ENABLED) {
		log(level::ERROR, "PIB(%s) probe: fail to enable",
		    pdbg_target_path(pib));
		throw pdbgError_t(exception::PDBG_TARGET_NOT_OPERATIONAL);
	}
	return pib;
}

struct pdbg_target *getFsiTarget(struct pdbg_target *proc)
{
	char path[16];
	sprintf(path, "/proc%d/fsi", pdbg_target_index(proc));
	struct pdbg_target *fsi = pdbg_target_from_path(nullptr, path);
	if (fsi == nullptr) {
		log(level::ERROR, "Failed to get FSI target for(%s)",
		    pdbg_target_path(proc));
		throw pdbgError_t(exception::PDBG_TARGET_NOT_OPERATIONAL);
	}
	// Probe the target
	if (pdbg_target_probe(fsi) != PDBG_TARGET_ENABLED) {
		log(level::ERROR, "FSI(%s) probe: fail to enable",
		    pdbg_target_path(fsi));
		throw pdbgError_t(exception::PDBG_TARGET_NOT_OPERATIONAL);
	}
	return fsi;
}

/**
 * @brief callback function, which are used to get pdbg target
 * based on binary path.
 *
 * @param[in] target current device tree target
 * @param[out] appPrivData used for accessing|storing from|to application
 *
 * @return 0 to continue traverse, non-zero to stop traverse
 **/
int pdbgCallbackToGetTgt(struct pdbg_target *tgt, void *appPrivData)
{
	TargetInfo *targetInfo = static_cast<TargetInfo *>(appPrivData);

	ATTR_PHYS_BIN_PATH_Type physBinPath;
	if (!pdbg_target_get_attribute(
		tgt, "ATTR_PHYS_BIN_PATH",
		std::stoi(dtAttr::fapi2::ATTR_PHYS_BIN_PATH_Spec),
		dtAttr::fapi2::ATTR_PHYS_BIN_PATH_ElementCount, physBinPath)) {
		return continueTgtTraversal;
	}

	if (std::memcmp(physBinPath, targetInfo->physBinPath,
			dtAttr::fapi2::ATTR_PHYS_BIN_PATH_ElementCount) != 0) {
		return continueTgtTraversal;
	}

	targetInfo->target = tgt;

	return requireTgtFound;
}

struct pdbg_target *getTgtFromBinPath(const ATTR_PHYS_BIN_PATH_Type &binPath)
{

	TargetInfo targetInfo;
	std::memcpy(&targetInfo.physBinPath, binPath,
		    dtAttr::fapi2::ATTR_PHYS_BIN_PATH_ElementCount);

	int ret = pdbg_target_traverse(NULL, pdbgCallbackToGetTgt, &targetInfo);

	if (ret != requireTgtFound) {
		std::stringstream ss;
		for (uint32_t a = 0; a < sizeof(ATTR_PHYS_BIN_PATH_Type); a++) {
			ss << " 0x" << std::hex << static_cast<int>(binPath[a]);
		}
		std::string s = ss.str();
		log(level::ERROR,
		    "Given PATH(%s) not found in phal device tree", s.c_str());
		return nullptr;
	}

	return targetInfo.target;
}

bool isOdysseyChip(struct pdbg_target *target)
{
    const uint16_t ODYSSEY_CHIP_ID = 0x60C0;
    const uint8_t ATTR_TYPE_OCMB_CHIP = 75;
    ATTR_TYPE_Type type;
    DT_GET_PROP(ATTR_TYPE, target, type);
    if(type != ATTR_TYPE_OCMB_CHIP) {
        return false;
    }
    ATTR_CHIP_ID_Type chipId = 0;
    DT_GET_PROP(ATTR_CHIP_ID, target,chipId);
    if(chipId == ODYSSEY_CHIP_ID) {
        return true;
        }
    return false;
}

void validateProcTgt(struct pdbg_target *tgt)
{
	const char *tgtClass = pdbg_target_class_name(tgt);
	if (!tgtClass) {
		log(level::ERROR,
		    "validateProcTgt: pdbg_target_class_name returns "
		    "empty class name");
		throw pdbgError_t(exception::PDBG_TARGET_INVALID);
	}
	if (strcmp("proc", tgtClass)) {
		log(level::ERROR,
		    "validateProcTgt: Target is not processor type");
		throw pdbgError_t(exception::PDBG_TARGET_INVALID);
	}
}

void validateChipTgt(struct pdbg_target *tgt)
{
	if (isOdysseyChip(tgt))
	{
		return;
	}
	validateProcTgt(tgt);
}

} // namespace pdbg
} // namespace openpower::phal::utils
