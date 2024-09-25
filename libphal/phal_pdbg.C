#include "libphal.H"
#include "log.H"
#include "phal_exception.H"
#include "utils_pdbg.H"

#include <attributes_info.H>

#include <cstring>
#include <optional>
#include <set>
#include <expected>

namespace openpower::phal::pdbg
{

using namespace openpower::phal::logging;
using namespace openpower::phal::utils::pdbg;
namespace phal_exception = openpower::phal::exception;

void init(pdbg_backend pdbgBackend, const int32_t logLevel,
	  const std::string pdbgDtbPath)
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
/*
bool is_ody_ocmb_chip(struct pdbg_target *target)
{
	const uint16_t ODYSSEY_CHIP_ID = 0x60C0;
	const uint8_t ATTR_TYPE_OCMB_CHIP = 75;
	ATTR_CHIP_ID_type ODYSSEY_CHIP_ID =
	ATTR_TYPE_type type;
	DT_GET_PROP(ATTR_TYPE, target, type);
	if(type != ATTR_TYPE_OCMB_CHIP) {
				return false;
	}
	ATTR_CHIP_ID_type chipId = 0;
	pdbg_target_get_attribute(ATTR_CHIP_ID, target,chipId);
	if(chipId == ODYSSEY_CHIP_ID) {
		return true;
	}
	return false;
}*/

bool isPrimaryProc(struct pdbg_target *proc)
{
	// Validate input target and only allow for proc type input
	validateProcTgt(proc);

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

void deconfigureTgt(const ATTR_PHYS_BIN_PATH_Type &physBinPath,
		    const uint32_t logId)
{
	struct pdbg_target *target = getTgtFromBinPath(physBinPath);
	if (target == nullptr) {
		log(level::ERROR, "deconfigureTgt: Failed to get Target");
		throw pdbgError_t(exception::PDBG_TARGET_NOT_FOUND);
	}

	// Skip deconfig request for primary proc.
	const char *tgtClass = pdbg_target_class_name(target);
	if (!tgtClass) {
		log(level::ERROR,
		    "deconfigureTgt: pdbg_target_class_name returns "
		    "empty class name");
		throw pdbgError_t(exception::PDBG_TARGET_INVALID);
	}
	if (strcmp("proc", tgtClass) == 0) {
		if (isPrimaryProc(target)) {
			log(level::WARNING,
			    "deconfigureTgt: Skipping primary proc((%s)) "
			    "deconfig by "
			    "policy",
			    pdbg_target_path(target));
			return;
		}
	}

	ATTR_HWAS_STATE_Type hwasState;
	if (DT_GET_PROP(ATTR_HWAS_STATE, target, hwasState)) {
		log(level::ERROR, "Could not read(%s) HWAS_STATE attribute",
		    pdbg_target_path(target));
		throw pdbgError_t(exception::DEVTREE_ATTR_READ_FAIL);
	}

	if (!hwasState.present) {
		log(level::ERROR,
		    "deconfigureTgt: Skipping Target((%s)) is not "
		    "present",
		    pdbg_target_path(target));
		return;
	}
	// Update HWAS_STATE attribute with deconfig/EID information
	hwasState.functional = 0;
	hwasState.deconfiguredByEid = logId;
	if (DT_SET_PROP(ATTR_HWAS_STATE, target, hwasState)) {
		log(level::ERROR, "Could not write(%s) HWAS_STATE attribute",
		    pdbg_target_path(target));
		throw pdbgError_t(exception::DEVTREE_ATTR_WRITE_FAIL);
	}
}

void getLocationCode(struct pdbg_target *target,
		     ATTR_LOCATION_CODE_Type &locationCode)
{
	// check target is valid.
	if (target == nullptr) {
		log(level::ERROR, "getLocationCode: Invalid pdbg Target");
		throw pdbgError_t(exception::PDBG_TARGET_INVALID);
	}

	// If the target supports ATTR_LOCATION_CODE, return same.
	if (!DT_GET_PROP(ATTR_LOCATION_CODE, target, locationCode)) {
		return;
	}

	std::string tgtPath{pdbg_target_path(target)};
	std::string tgtClass{pdbg_target_class_name(target)};

	struct pdbg_target *parentFruTarget = nullptr;
	if ((tgtClass == "ocmb") || (tgtClass == "mem_port")) {
		/**
		 * Note: The assumption is, dimm is parent fru for "ocmb" and
		 * "mem_port" and each "ocmb" or "mem_port" will have one
		 * "dimm" so if something is changed then need to fix
		 * this logic.
		 * In phal cec device tree dimm is placed under
		 * ocmb->mem_port based on dimm pervasive path.
		 */
		auto dimmCount = 0;
		struct pdbg_target *lastDimmTgt = nullptr;
		pdbg_for_each_target("dimm", target, lastDimmTgt)
		{
			parentFruTarget = lastDimmTgt;
			++dimmCount;
		}

		if (dimmCount == 0) {
			log(level::ERROR,
			    "Failed to get the parent dimm target for (%s)",
			    tgtPath.c_str());
			throw pdbgError_t(exception::PDBG_TARGET_NOT_FOUND);
		} else if (dimmCount > 1) {
			log(level::ERROR,
			    "More [%d] dimm targets are present for (%s)",
			    dimmCount, tgtPath.c_str());
			throw pdbgError_t(exception::PDBG_TARGET_NOT_FOUND);
		}
	} else {
		/**
		 * Note:  All FRU parts (units - both(chiplet and  non-chiplet))
		 * are modelled under the respective processor in cec device
		 * tree so, if something changed then, need to revisit the
		 * logic which is used to get the FRU details of FRU unit.
		 */
		parentFruTarget = pdbg_target_parent("proc", target);
		if (parentFruTarget == nullptr) {
			log(level::ERROR,
			    "Failed to get the parent processor target for "
			    "(%s)",
			    "target for (%s)", tgtPath.c_str());
			throw pdbgError_t(
			    exception::PDBG_TARGET_PARENT_NOT_FOUND);
		}
	}
	if (DT_GET_PROP(ATTR_LOCATION_CODE, parentFruTarget, locationCode)) {
		log(level::ERROR, "Failed to get ATTR_LOCATION_CODE for (%s)",
		    tgtPath.c_str());
		throw pdbgError_t(exception::DEVTREE_ATTR_READ_FAIL);
	}
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

bool hasControlTransitionedToHost()
{
	// Read the scratch register to find if the control has moved to phyp
	// (Host)
	auto pibTarget = pdbg_target_from_path(nullptr, "/proc0/pib");

	uint64_t l_coreScratchRegData = 0xFFFFFFFFFFFFFFFFull;
	// HB changes the below core scratch reg as one of the last instructions
	// that is run, so if that is zero then we're in phyp
	uint64_t l_coreScratchRegAddr = 0x4602F489;

	// Is there any error in reading the scom address, consider control is
	// in hostboot
	if (pib_read(pibTarget, l_coreScratchRegAddr, &l_coreScratchRegData)) {
		// If unable to read the register, by default consider the
		// control is in hostboot
		log(level::ERROR, "scom read error: 0x%X",
		    l_coreScratchRegAddr);
		return false;
	}

	// If the register reads zero, return control moved to host.
	return (l_coreScratchRegData == 0);
}

std::optional<LocationCode> getUnexpandedLocCode(const LocationCode &locCode)
{
	constexpr uint8_t EXP_LOCATION_CODE_MIN_LENGTH = 17;
	// Location code should start with "U"
	if (locCode[0] != 'U') {
		log(level::ERROR,
		    "Location code should start with \"U\""
		    " but, given location code %s",
		    locCode.c_str());
		return std::nullopt;
	}

	// Given location code length should be need to match with minimum
	// length
	// to drop expanded format in given location code.
	if (locCode.length() < EXP_LOCATION_CODE_MIN_LENGTH) {
		log(level::ERROR,
		    "Given location code %s is not meet "
		    "with minimum length %d",
		    locCode.c_str(), EXP_LOCATION_CODE_MIN_LENGTH);
		return std::nullopt;
	}

	// "-" should be there to seggregate all (FC, Node number and SE values)
	// together.
	// Note: Each (FC, Node number and SE) value can be seggregate by "."
	// but, cec device tree just have unexpand format so, just skipping
	// still first occurrence "-" and replacing with "fcs".
	auto endPosOfFcs = locCode.find('-', EXP_LOCATION_CODE_MIN_LENGTH);
	if (endPosOfFcs == std::string::npos) {
		log(level::ERROR,
		    "Given location code %s is not valid i.e could not find "
		    "dash",
		    locCode.c_str());
		return std::nullopt;
	}

	std::string unExpandedLocCode("Ufcs");

	if (locCode.length() > EXP_LOCATION_CODE_MIN_LENGTH) {
		unExpandedLocCode.append(
		    locCode.substr(endPosOfFcs, std::string::npos));
	}

	return unExpandedLocCode;
}

/**
 * @brief Fetch FRU type from device tree.
 *
 * @details An api to get FRU ATTR_TYPE corresponding to
 *          a given unexpanded location code.
 *
 * @param[in] locationCode - Location code in unexpanded format.
 * @param[out] fruType - Fru type of a given location code.
 *
 * @return 0 if fru type is found.
 * @return -1 if unable to find a fru with the given location code.
 * @return DEVTREE_ATTR_READ_FAIL if unable to fetch ATTR_TYPE from device tree.
 */
static int fetchFruTypeFromDevTree(const LocationCode &locationCode,
				   ATTR_TYPE_Type &fruType)
{
	ATTR_LOCATION_CODE_Type locCode;
	// Defining a list of existing frus
	// Note: We need to update the list if a new fru is added
	std::vector<const char *> frus{"proc", "tpm", "dimm"};
	struct pdbg_target *target;

	for (auto fru : frus) {
		pdbg_for_each_class_target(fru, target)
		{
			if (DT_GET_PROP(ATTR_LOCATION_CODE, target, locCode)) {
				continue;
			} else if (locCode == locationCode) {
				if (DT_GET_PROP(ATTR_TYPE, target, fruType)) {
					return phal_exception::
					    DEVTREE_ATTR_READ_FAIL;
				}
				return 0; // success
			}
		}
	}
	return -1;
}

std::expected<FRUType, phal_exception::ERR_TYPE>
    getFRUType(const LocationCode &locationCode) noexcept
{
	LocationCode unExpandedLocCode = locationCode;
	if (!unExpandedLocCode.starts_with("Ufcs-")) {

		auto loc = getUnexpandedLocCode(locationCode);
		if (loc.has_value()) {
			unExpandedLocCode = loc.value();
		} else {
			log(level::ERROR, "Given location code %s is not valid",
			    unExpandedLocCode.c_str());
			return std::unexpected(
			    phal_exception::ATTR_LOC_CODE_NOT_VALID);
		}
	}
	ATTR_TYPE_Type fruType = ENUM_ATTR_TYPE_NA;
	auto ret = fetchFruTypeFromDevTree(unExpandedLocCode, fruType);
	if (ret == 0) {
		return fruType;
	} else if (ret == phal_exception::DEVTREE_ATTR_READ_FAIL) {
		log(level::ERROR, "Attribute Type not found for the "
				  "target at given location code");
		return std::unexpected(phal_exception::DEVTREE_ATTR_READ_FAIL);
	}
	log(level::DEBUG, "Given location code %s is not found ",
	    unExpandedLocCode.c_str());
	return std::unexpected(phal_exception::ATTR_LOC_CODE_NOT_FOUND);
}
} // namespace openpower::phal::pdbg
