extern "C" {
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>

#include <libpdbg.h>
#include <libpdbg_sbe.h>
}

#include "libipl.H"
#include "libipl_internal.H"
#include "common.H"
#include <attributes_info.H>

#include <ekb/chips/p10/procedures/hwp/perv/p10_start_cbs.H>
#include <ekb/chips/p10/procedures/hwp/perv/p10_setup_ref_clock.H>
#include <ekb/chips/p10/procedures/hwp/perv/p10_clock_test.H>
#include <ekb/chips/p10/procedures/hwp/perv/p10_setup_sbe_config.H>
#include <ekb/chips/p10/procedures/hwp/perv/p10_select_boot_master.H>

#include <libguard/guard_interface.hpp>
#include <libguard/guard_entity.hpp>
#include <libguard/include/guard_record.hpp>
#include <libguard/guard_exception.hpp>
#include <filesystem>
#include <fstream>
#include <array>
#include <chrono>
#include <thread>

#define TGT_TYPE_PROC 0x05
#define FRU_TYPE_CORE 0x07
#define FRU_TYPE_MC 0x44
#define FRU_TYPE_FC 0x53

#define OSC_CTL_OFFSET 0x06
#define OSC_RESET_CMD 0x04
#define OSC_RESET_CMD_LEN 0x01

#define OSC_STAT_OFFSET 0x07
#define OSC_STAT_REG_LEN 0x01
#define OSC_STAT_ERR_MASK 0x80

#define CLOCK_RESTART_DELAY_IN_MS 100

#define GUARD_CONTINUE_TGT_TRAVERSAL 0
#define GUARD_TGT_FOUND 1
#define GUARD_TGT_NOT_FOUND 2
#define GUARD_PRIMARY_PROC_NOT_APPLIED 3

constexpr auto BOOTTIME_GUARD_INDICATOR = "/tmp/phal/boottime_guard_indicator";

struct guard_target {
	uint8_t path[21];
	bool set_hwas_state;
	uint8_t guardType;

	guard_target()
	{
		memset(&path, 0, sizeof(path));
		set_hwas_state = false;
		guardType = 0; // GARD_NULL
	}
};

static void ipl_pre0(void)
{
	ipl_pre();
}

static int ipl_poweron(void)
{
	return -1;
}

static int ipl_startipl(void)
{
	return -1;
}

static int ipl_DisableAttns(void)
{
	return -1;
}

static bool small_core_enabled(void)
{
	struct stat statbuf;
	int ret;

	/* If /tmp/small_core file exists, boot in small core mode */
	ret = stat("/tmp/small_core", &statbuf);
	if (ret == -1)
		return false;

	if (S_ISREG(statbuf.st_mode)) {
		ipl_log(IPL_INFO, "Booting in small core mode\n");
		return true;
	}

	return false;
}

static bool set_or_clear_state(struct pdbg_target *target, bool do_set)
{
	uint8_t buf[5];
	uint8_t flag_present = 0x40;
	uint8_t flag_functional = 0x20;

	if (!pdbg_target_get_attribute_packed(target, "ATTR_HWAS_STATE", "41",
					      1, buf)) {
		ipl_log(IPL_ERROR, "Attribute [ATTR_HWAS_STATE] read failed\n");
		return false;
	}

	if (do_set)
		buf[4] |= (flag_present | flag_functional);
	else
		buf[4] &= (uint8_t)(~flag_functional);

	if (!pdbg_target_set_attribute_packed(target, "ATTR_HWAS_STATE", "41",
					      1, buf)) {
		ipl_log(IPL_ERROR,
			"Attribute [ATTR_HWAS_STATE] write failed\n");
		return false;
	}
	return true;
}

/*
 * Helper function set the clock functional state based on
 * ATTR_SYS_CLOCK_DECONFIG_STATE values
 * ATTR_SYS_CLOCK_DECONFIG_STATE contains the functional state
 * of the system clocks/oscillators. This is the way that Hostboot
 * communicates to the BMC which clocks have failed.
 *
 * @return true on success, false on failure
 */
static bool update_clock_func_state(void)
{
	ipl_log(
	    IPL_INFO,
	    "Updating ref clock target HWAS state based on Hostboot value \n");

	ATTR_SYS_CLOCK_DECONFIG_STATE_Type clk_state =
	    ENUM_ATTR_SYS_CLOCK_DECONFIG_STATE_NO_DECONFIG;

	struct pdbg_target *clock_target;
	struct pdbg_target *root = pdbg_target_root();
	if (!pdbg_target_get_attribute(root, "ATTR_SYS_CLOCK_DECONFIG_STATE", 4,
				       1, &clk_state)) {
		ipl_log(
		    IPL_ERROR,
		    "Attribute [ATTR_SYS_CLOCK_DECONFIG_STATE] read failed \n");
		ipl_plat_procedure_error_handler(IPL_ERR_ATTR_READ_FAIL);
		return false;
	}

	if (clk_state == ENUM_ATTR_SYS_CLOCK_DECONFIG_STATE_NO_DECONFIG) {
		// No HWAS state update required
		ipl_log(IPL_INFO, "update_clock_func_state : No updates \n");
		return true;
	}
	pdbg_for_each_class_target("oscrefclk", clock_target)
	{
		if (clk_state ==
		    ENUM_ATTR_SYS_CLOCK_DECONFIG_STATE_ALL_DECONFIG) {
			// Update HWAS state to non-functional
			ipl_log(IPL_INFO,
				"Clock(%s) setting to non functional \n",
				pdbg_target_path(clock_target));
			if (!set_or_clear_state(clock_target, false)) {
				return false;
			}
			continue;
		}
		// Get Clock position
		ATTR_POSITION_Type clk_pos;
		if (!pdbg_target_get_attribute(clock_target, "ATTR_POSITION", 2,
					       1, &clk_pos)) {
			ipl_log(IPL_ERROR,
				"Attribute ATTR_POSITION read failed"
				" for clock '%s' \n",
				pdbg_target_path(clock_target));
			ipl_plat_procedure_error_handler(
			    IPL_ERR_ATTR_READ_FAIL);
			return false;
		}
		// Assumption: Clock A linked to ATTR_POSITION value 0 and
		// Clock B linked to ATTR_POSITION value 1
		if (((clk_pos == 0) &&
		     (clk_state ==
		      ENUM_ATTR_SYS_CLOCK_DECONFIG_STATE_A_DECONFIG)) ||
		    ((clk_pos == 1) &&
		     (clk_state ==
		      ENUM_ATTR_SYS_CLOCK_DECONFIG_STATE_B_DECONFIG))) {
			ipl_log(IPL_INFO,
				"deconfig state(%d) Clock(%s) setting to non "
				"functional \n",
				clk_state, pdbg_target_path(clock_target));
			if (!set_or_clear_state(clock_target, false)) {
				return false;
			}
		}
	}
	return true;
}

static int update_hwas_state_callback(struct pdbg_target *target, void *priv)
{
	guard_target *target_info = static_cast<guard_target *>(priv);
	uint8_t path[21] = {0};
	uint8_t type;
	char tgtPhysDevPath[64];
	std::string guard_action(target_info->set_hwas_state ? "Clearing"
							     : "Applying");
	std::string guardTypeStr(
	    openpower::guard::guardReasonToStr(target_info->guardType));

	if (!pdbg_target_get_attribute(target, "ATTR_PHYS_BIN_PATH", 1, 21,
				       path))
		// Returning 0 for continue traversal, as the requested target
		// is not found
		return GUARD_CONTINUE_TGT_TRAVERSAL;

	if (memcmp(target_info->path, path, sizeof(path)) != 0)
		return GUARD_CONTINUE_TGT_TRAVERSAL;

	if (!pdbg_target_get_attribute(target, "ATTR_PHYS_DEV_PATH", 1, 64,
				       tgtPhysDevPath)) {
		ipl_log(IPL_ERROR, "Failed to read ATTR_PHYS_DEV_PATH for %s\n",
			pdbg_target_path(target));
		return GUARD_TGT_NOT_FOUND;
	}

	if (!pdbg_target_get_attribute(target, "ATTR_TYPE", 1, 1, &type)) {
		ipl_log(IPL_ERROR, "Failed to read ATTR_TYPE for %s\n",
			pdbg_target_path(target));
		// ATTR_TYPE attribute not found for the target, hence this is
		// an error case, so need to stop
		return GUARD_TGT_NOT_FOUND;
	}

	if (ipl_type() == IPL_TYPE_MPIPL &&
	    (type == FRU_TYPE_CORE || type == FRU_TYPE_FC)) {

		ipl_log(IPL_INFO, "%s guard record for %s. Type: %s\n",
			guard_action.c_str(), tgtPhysDevPath,
			guardTypeStr.c_str());

		if (!set_or_clear_state(target, target_info->set_hwas_state)) {
			ipl_log(
			    IPL_ERROR,
			    "Failed to update functional state of target 0x%x, "
			    "index=0x%x\n",
			    type, pdbg_target_index(target));
			// Unable to update the functional state of HWAS
			// attribute of the target, so we need to stop
			return GUARD_TGT_NOT_FOUND;
		}

		if ((type == FRU_TYPE_FC) && !small_core_enabled()) {
			struct pdbg_target *core;

			ipl_log(IPL_INFO,
				"%s guard record for associated Core since %s "
				"guared\n",
				guard_action.c_str(), tgtPhysDevPath);

			pdbg_for_each_target("core", target, core)
			{
				if (!set_or_clear_state(
					core, target_info->set_hwas_state)) {
					ipl_log(
					    IPL_ERROR,
					    "Failed to update functional state"
					    " of core with index 0x%x\n",
					    pdbg_target_index(core));
					// Unable to update the functional state
					// of HWAS attribute of the target, so
					// we need to stop
					return GUARD_TGT_NOT_FOUND;
				}
			}
		}

	} else if (ipl_type() == IPL_TYPE_NORMAL) {

		if (!openpower::guard::isEphemeralType(
			target_info->guardType)) {
			if (type == TGT_TYPE_PROC) {
				if (ipl_is_master_proc(target)) {
					ipl_log(
					    IPL_INFO,
					    "Primary processor [%s] is guarded "
					    "so, skipping to apply. Type: %s\n",
					    tgtPhysDevPath,
					    guardTypeStr.c_str());
					return GUARD_PRIMARY_PROC_NOT_APPLIED;
				}
			}
		}

		ipl_log(IPL_INFO, "%s guard record for %s. Type: %s\n",
			guard_action.c_str(), tgtPhysDevPath,
			guardTypeStr.c_str());

		if (!set_or_clear_state(target, target_info->set_hwas_state)) {
			ipl_log(IPL_ERROR,
				"Failed to update functional state of fru type "
				"0x%x\n",
				type);
			// Unable to update the functional state of HWAS
			// attribute of the target, so we need to stop
			return GUARD_TGT_NOT_FOUND;
		}

		if ((type == FRU_TYPE_FC) && !small_core_enabled()) {
			struct pdbg_target *core;

			ipl_log(IPL_INFO,
				"%s guard record for associated Core since %s "
				"guared\n",
				guard_action.c_str(), tgtPhysDevPath);

			pdbg_for_each_target("core", target, core)
			{
				if (!set_or_clear_state(
					core, target_info->set_hwas_state)) {
					ipl_log(
					    IPL_ERROR,
					    "Failed to update functional state"
					    " of core with index 0x%x\n",
					    pdbg_target_index(core));
					// Unable to update the functional state
					// of HWAS attribute of the target, so
					// we need to stop
					return GUARD_TGT_NOT_FOUND;
				}
			}
		}

	} else {
		ipl_log(IPL_INFO, "Skip, %s guard record for %s. Type: %s\n",
			guard_action.c_str(), tgtPhysDevPath);
	}

	// Requested target found
	return GUARD_TGT_FOUND;
}

/**
 * @brief Allow guard actions in the below cases to support resource recovery.
 *
 * - If the current boot is MPIPL.
 *
 * - If the BOOTTIME_GUARD_INDICATOR file (that will be created by the BMC
 *   in the PowerOn or TI or Checkstop or Watchdog timeout path) is exist
 *   in the current boot.
 */
static bool guard_action_allowed()
{
	if (ipl_type() == IPL_TYPE_MPIPL) {
		return true;
	}

	namespace fs = std::filesystem;
	fs::path boottime_guard_indicator(BOOTTIME_GUARD_INDICATOR);

	if (fs::exists(boottime_guard_indicator)) {
		// Remove indicator since that will be created by the BMC
		// based on the different boot path.
		fs::remove(boottime_guard_indicator);
		return true;
	} else {
		return false;
	}
}

//@Brief Function will get the guard records and will update the functional
// state of the guarded resources in HWAS state attribute in device tree based
// on the guard actions in the different boots.
static void process_guard_records()
{
	if (!guard_action_allowed()) {
		ipl_log(IPL_INFO, "No guard actions in the current boot");
		return;
	}

	try {
		openpower::guard::libguard_init(false);
		auto records = openpower::guard::getAll();

		if (records.size()) {
			ipl_log(IPL_INFO, "Number of Records = %d\n",
				records.size());

			if (!ipl_guard()) {
				// Don't return, we should handle reconfig type
				// guard records even if guard setting is
				// disabled.
				ipl_log(IPL_INFO,
					"Disabled to apply the guard records");
			}

			for (const auto &elem : records) {

				if (!ipl_guard() &&
				    !openpower::guard::isEphemeralType(
					elem.errType)) {
					// Disabled to apply the guard records
					// so should not allow the records to
					// apply except ephemeral type guard
					// records.
					continue;
				} else if (elem.recordId == GUARD_RESOLVED) {
					// No need to apply the resolved guard
					// records.
					continue;
				}

				guard_target targetinfo;
				targetinfo.guardType = elem.errType;
				int index = 0, i, err;

				targetinfo.path[index] =
				    elem.targetId.type_size;
				index += 1;

				for (i = 0;
				     i < (0x0F & elem.targetId.type_size);
				     i++) {

					targetinfo.path[index] =
					    elem.targetId.pathElements[i]
						.targetType;
					targetinfo.path[index + 1] =
					    elem.targetId.pathElements[i]
						.instance;

					index += sizeof(
					    elem.targetId.pathElements[0]);
				}

				// Clear ephemeral type guard records in the
				// normal ipl.
				if ((ipl_type() == IPL_TYPE_NORMAL) &&
				    openpower::guard::isEphemeralType(
					elem.errType)) {
					openpower::guard::clear(elem.recordId);
					targetinfo.set_hwas_state = true;
				} else {
					targetinfo.set_hwas_state = false;
				}

				err = pdbg_target_traverse(
				    NULL, update_hwas_state_callback,
				    &targetinfo);
				if ((err == GUARD_CONTINUE_TGT_TRAVERSAL) ||
				    (err == GUARD_TGT_NOT_FOUND))
					ipl_log(
					    IPL_ERROR,
					    "Failed to set HWAS state for guard"
					    " record[ID: %d]\n",
					    elem.recordId);
			}
		}
	} catch (const openpower::guard::exception::GuardException &ex) {
		// For any exeption related to guard, add the PEL and continue
		// to boot
		ipl_log(IPL_ERROR,
			"Caught the exception %s and continuing to boot "
			"without processing guard records",
			ex.what());
		ipl_error_callback(IPL_ERR_GUARD_PARTITION_ACCESS);
	}
}

/*
 * @Brief Function will check if the FCO state is set for hardware units
 * consumed by sbe or not. If FCO override bit is set for such hardware
 * units, need to mark present and functional state as set in devicetree
 * during the boot.
 *
 */
static void apply_fco_override(void)
{
	std::array<const char *, 8> mProcChild = {
	    "core", "pauc", "pau", "iohs", "mc", "chiplet", "pec", "fc"};
	struct pdbg_target *proc, *child;
	uint8_t buf[5];

	pdbg_for_each_class_target("proc", proc)
	{
		if (!ipl_is_master_proc(proc))
			continue;

		for (const char *data : mProcChild) {
			pdbg_for_each_target(data, proc, child)
			{

				if (!pdbg_target_get_attribute_packed(
					child, "ATTR_HWAS_STATE", "41", 1,
					buf)) {
					ipl_error_callback(
					    IPL_ERR_ATTR_READ_FAIL);
					continue;
				}

				// 0-1 bits reserved
				// 2nd bit Functional override
				// 3rd bit spec deconfig
				// 4th bit Dump capable
				// 5th bit Functional
				// 6th bit Present
				// 7th bit Deconfig
				// Checking the FCO bit is set or not. If it's
				// set means functional and present state of
				// target should be set.
				if (buf[4] & 0x04) {
					if (!set_or_clear_state(child, true)) {
						ipl_error_callback(
						    IPL_ERR_ATTR_WRITE);
					}
				}
			}
		}
	}
}

//@Brief Function will set the functional and present state of master proc
// and of available procs in the system. For few children of master proc
// functional and present state will be set in device tree during genesis
// boot.
// It will also update functional state oscrefclk target.
static bool update_genesis_hwas_state(void)
{
	std::array<const char *, 8> mProcChild = {
	    "core", "pauc", "pau", "iohs", "mc", "chiplet", "pec", "fc"};
	struct pdbg_target *proc, *child, *clock_target;

	bool target_enabled = false;
	enum pdbg_target_status status;
	uint8_t clk_pos = 0;
	std::vector<std::pair<std::string, std::string>> ffdcs;

	pdbg_for_each_class_target("proc", proc)
	{
		if (pdbg_target_status(proc) != PDBG_TARGET_ENABLED) {
			target_enabled = false;
		} else {
			target_enabled = true;
		}

		if (!set_or_clear_state(proc, target_enabled)) {
			ipl_log(IPL_ERROR,
				"Failed to set HWAS state of proc %d\n",
				pdbg_target_index(proc));
			ipl_error_callback(IPL_ERR_ATTR_WRITE);
			return false;
		}

		if (!ipl_is_master_proc(proc))
			continue;

		for (const char *data : mProcChild) {
			pdbg_for_each_target(data, proc, child)
			{
				if (!set_or_clear_state(child,
							target_enabled)) {
					ipl_log(IPL_ERROR,
						"Failed to set HWAS state of "
						"%s, index %d\n",
						data, pdbg_target_index(child));
					ipl_error_callback(IPL_ERR_ATTR_WRITE);
					return false;
				}
			}
		}
	}

	pdbg_for_each_class_target("oscrefclk", clock_target)
	{
		if (!pdbg_target_get_attribute(clock_target, "ATTR_POSITION", 2,
					       1, &clk_pos)) {

			ipl_log(IPL_ERROR,
				"Attribute ATTR_POSITION read failed"
				" for clock '%s' \n",
				pdbg_target_path(clock_target));
			ipl_plat_procedure_error_handler(
			    IPL_ERR_ATTR_READ_FAIL);
			// IPL need to be failed if this step is failed for any
			// clock, since this clock cannot be marked as
			// functional
			return false;
		}

		status = pdbg_target_probe(clock_target);
		if (status != PDBG_TARGET_ENABLED) {
			ipl_log(
			    IPL_ERROR,
			    "clock '%s' is not operational, pdbg status = %d\n",
			    pdbg_target_path(clock_target), status);

			ffdcs.push_back(std::make_pair("PDBG_STATUS",
						       std::to_string(status)));
			ffdcs.push_back(std::make_pair("FAIL_TYPE",
						       "CHIP_NOT_OPERATIONAL"));
			ipl_plat_clock_error_handler(ffdcs, clk_pos);
			// IPL need to be failed if any clock is not operational
			return false;
		}
		if (!set_or_clear_state(clock_target, true)) {
			ipl_log(IPL_ERROR,
				"Failed to set HWAS state of oscrefclk, %s\n",
				pdbg_target_path(clock_target));
			return false;
		}
	}

	return true;
}

/**
 * @brief Wrapper function to execute continue mpipl on the proc
 *
 * @param[in] proc proc target to operate on
 *
 * return ipl error type enum
 */
static ipl_error_type ipl_sbe_mpipl_continue(struct pdbg_target *proc)
{
	enum sbe_state state;
	char path[16];
	int ret = 0;

	ipl_log(IPL_INFO, "ipl_sbe_mpipl_continue: Enter(%s)",
		pdbg_target_path(proc));

	// get PIB target
	sprintf(path, "/proc%d/pib", pdbg_target_index(proc));
	struct pdbg_target *pib = pdbg_target_from_path(nullptr, path);
	if (pib == nullptr) {
		ipl_log(IPL_ERROR, "Failed to get PIB target for(%s)",
			pdbg_target_path(proc));
		return IPL_ERR_PIB_TGT_NOT_FOUND;
	}

	ret = sbe_get_state(pib, &state);
	if (ret != 0) {
		ipl_log(IPL_ERROR, "Failed to read SBE state information (%s)",
			pdbg_target_path(pib));
		return IPL_ERR_FSI_REG;
	}

	// SBE_STATE_CHECK_CFAM case is already handled by pdbg api
	if (state != SBE_STATE_BOOTED) {
		ipl_log(IPL_ERROR,
			"SBE (%s) is not ready for chip-op: state(0x%08x)",
			pdbg_target_path(pib), state);
		return IPL_ERR_SBE_CHIPOP;
	}

	// call pdbg back-end function
	ret = sbe_mpipl_continue(pib);
	if (ret != 0) {
		ipl_log(IPL_ERROR, "SBE (%s) mpipl continue chip-op failed",
			pdbg_target_path(pib));
		return IPL_ERR_SBE_CHIPOP;
	}

	return IPL_ERR_OK;
}

static int ipl_updatehwmodel(void)
{
	namespace fs = std::filesystem;
	bool boot_file_absent = false;
	constexpr auto GENESIS_BOOT_FILE = "/var/lib/phal/genesisboot";
	fs::path genesis_boot_file = GENESIS_BOOT_FILE;

	ipl_log(IPL_INFO, "Istep: updatehwmodel: started\n");

	if (!fs::exists(genesis_boot_file)) {
		ipl_log(IPL_INFO, "updatehwmodel: Genesis mode boot\n");
		if (!update_genesis_hwas_state()) {
			ipl_log(IPL_ERROR,
				"Failed to set genesis boot state\n");
			return 1;
		}

		// Create new file to skip the genesis setup in next boot.
		if (!fs::exists(genesis_boot_file.parent_path())) {
			if (!fs::create_directories(
				genesis_boot_file.parent_path())) {
				ipl_log(IPL_ERROR,
					"Failed to create genesis boot file\n");
				return 1;
			}
		}
		boot_file_absent = true;
		std::ofstream file(GENESIS_BOOT_FILE);
	}

	if ((ipl_type() == IPL_TYPE_MPIPL) ||
	    (!fs::exists(BOOTTIME_GUARD_INDICATOR)))
		apply_fco_override();

	process_guard_records();

	if (!boot_file_absent && (ipl_type() != IPL_TYPE_MPIPL)) {
		// Update SBE state to Not usable in reboot path(not on MPIPL)
		// Boot error callback is only required for failure
		ipl_set_sbe_state_all(SBE_STATE_NOT_USABLE);

		// update refclock targets  functional sate based on
		// SYS_CLOCK_DECONFIG_STATE values.
		if (!update_clock_func_state()) {
			ipl_log(IPL_ERROR, "Failed to update set-ref clock "
					   "functional state \n");
			return 1;
		}
	}

	if (!ipl_check_functional_master()) {
		ipl_error_callback(IPL_ERR_PRI_PROC_NON_FUNC);
		return 1;
	}

	return 0;
}

static int ipl_alignment_check(void)
{
	return -1;
}

/**
 * @brief Check whether clock reset has to be skipped or not
 *
 * Will look whether file '/tmp/skip_clock_reset' exist or not. If exist
 * this function will return true, indicating clock reset has to be skipped
 *
 *
 * @return true if clock reset has to be skipped, false if clock reset has to be
 * 		performed
 */
static bool skip_clock_reset()
{
	int ret;
	struct stat statbuf;

	// If /tmp/skip_clock_reset file exists, skip clock soft reset
	ret = stat("/tmp/skip_clock_reset", &statbuf);
	if (ret == -1)
		return false;

	if (S_ISREG(statbuf.st_mode)) {
		ipl_log(IPL_INFO, "Skipping clock reset\n");
		return true;
	}

	return false;
}

/**
 * @brief Initialise the clock chip and then check the status of all clocks
 *
 * For all clocks available, do a soft reset, and then check the status register
 * to check whether it encounter a calibration failure or not.
 *
 * @param[out]  clock_select  clock selection type
 *
 * @return 0 on success, 1 on failure
 */
static int initialize_and_check_clock_chip(uint8_t &clock_select)
{
	struct pdbg_target *clock_target;
	enum pdbg_target_status status;
	uint8_t data;
	int rc = 0;
	int i2c_rc = 0;
	std::vector<std::pair<std::string, std::string>> ffdcs;
	uint8_t clk_pos = 0;
	bool skip_reset = false;
	uint8_t clock_count = 0;

	// Clock reset will revert all i2c write done, which may have written
	// to test firmware for different error scenario, in lab environment.
	// So test team can use this method to skip clock reset while testing.
	skip_reset = skip_clock_reset();

	pdbg_for_each_class_target("oscrefclk", clock_target)
	{
		if (!ipl_is_functional(clock_target))
			continue;

		// clock count has to be incremented even if it failed to
		// initiallize
		clock_count++;
		ffdcs.clear();

		if (!pdbg_target_get_attribute(clock_target, "ATTR_POSITION", 2,
					       1, &clk_pos)) {

			ipl_log(IPL_ERROR,
				"Attribute ATTR_POSITION read failed"
				" for clock '%s' \n",
				pdbg_target_path(clock_target));
			ipl_plat_procedure_error_handler(
			    IPL_ERR_ATTR_READ_FAIL);
			rc++;
			continue;
		}

		// Update clock select value based on clock position
		// Note : Unsupported index value defaults to user initliased
		// value
		if (clk_pos == 0) {
			clock_select = ENUM_ATTR_CP_REFCLOCK_SELECT_OSC0;
		} else if (clk_pos == 1) {
			clock_select = ENUM_ATTR_CP_REFCLOCK_SELECT_OSC1;
		}

		status = pdbg_target_probe(clock_target);
		if (status != PDBG_TARGET_ENABLED) {
			ipl_log(
			    IPL_ERROR,
			    "clock '%s' is not operational, pdbg status = %d\n",
			    pdbg_target_path(clock_target), status);

			ffdcs.push_back(std::make_pair("PDBG_STATUS",
						       std::to_string(status)));
			ffdcs.push_back(std::make_pair("FAIL_TYPE",
						       "CHIP_NOT_OPERATIONAL"));
			ipl_plat_clock_error_handler(ffdcs, clk_pos);
			rc++;
			continue;
		}
		ipl_log(
		    IPL_DEBUG,
		    "Istep: soft reset clock, and verify clock status register"
		    " for clock-%d\n",
		    clk_pos);

		if (!skip_reset) {
			// Resetting the clock chip, so that it will recalibrate
			// input oscillator signal and identifies bad
			// oscillators.
			data = OSC_RESET_CMD;
			i2c_rc = i2c_write(clock_target, 0, OSC_CTL_OFFSET,
					   OSC_RESET_CMD_LEN, &data);
			if (i2c_rc) {
				ipl_log(IPL_ERROR,
					"soft reset command is failed for "
					"clock '%s' with rc = %d\n",
					pdbg_target_path(clock_target), i2c_rc);

				ffdcs.push_back(std::make_pair(
				    "I2C_RC", std::to_string(i2c_rc)));
				ffdcs.push_back(
				    std::make_pair("FAIL_TYPE", "SOFT_RESET"));
				ipl_plat_clock_error_handler(ffdcs, clk_pos);
				rc++;
				continue;
			} else {
				ipl_log(IPL_DEBUG,
					"soft reset command is successfull for "
					"clock '%s'\n",
					pdbg_target_path(clock_target));
			}

			// wait for clock to restart
			std::this_thread::sleep_for(std::chrono::milliseconds(
			    CLOCK_RESTART_DELAY_IN_MS));
		}

		// Read clock status register to check whether it reports
		// calibration error. Bit-0 will be set if there is a
		// calibration error.

		i2c_rc = i2c_read(clock_target, 0, OSC_STAT_OFFSET,
				  OSC_STAT_REG_LEN, &data);
		if (i2c_rc) {
			ipl_log(IPL_ERROR,
				"status register read is failed for clock "
				"'%s', with rc = %d\n",
				pdbg_target_path(clock_target), i2c_rc);

			ffdcs.push_back(
			    std::make_pair("I2C_RC", std::to_string(i2c_rc)));
			ffdcs.push_back(
			    std::make_pair("FAIL_TYPE", "STATUS_READ"));
			ipl_plat_clock_error_handler(ffdcs, clk_pos);
			rc++;
			continue;
		} else {
			ipl_log(
			    IPL_DEBUG,
			    "status register value for clock '%s' is 0x%2X\n",
			    pdbg_target_path(clock_target), data);

			if (data & OSC_STAT_ERR_MASK) {
				ipl_log(IPL_ERROR,
					"Calibration is failed for clock '%s', "
					"status=0x%2X",
					pdbg_target_path(clock_target), data);

				ffdcs.push_back(std::make_pair(
				    "CLOCK_STATUS", std::to_string(data)));
				ffdcs.push_back(
				    std::make_pair("FAIL_TYPE", "CALIB_ERR"));
				ipl_plat_clock_error_handler(ffdcs, clk_pos);
				rc++;
				continue;
			}
		}
	}

	// Check clock count value is valid.
	if ((rc == 0) && (clock_count != 1 && clock_count != 2)) {
		ipl_log(IPL_ERROR,
			"Invalid number (%d) of clock target found\n",
			clock_count);

		ipl_plat_procedure_error_handler(IPL_ERR_INVALID_NUM_CLOCK);
		rc++;
	}

	// Override clock slection value incase spare clock support is available
	if ((rc == 0) && (clock_count == NUM_CLOCK_FOR_REDUNDANT_MODE)) {
		clock_select = fapi2::ENUM_ATTR_CP_REFCLOCK_SELECT_BOTH_OSC0;
	}
	return rc;
}

static int ipl_set_ref_clock(void)
{
	struct pdbg_target *proc;
	int rc = 0;
	fapi2::ReturnCode fapirc;
	// Default value of attribute will be for non-redundant mode
	fapi2::ATTR_CP_REFCLOCK_SELECT_Type clock_select =
	    ENUM_ATTR_CP_REFCLOCK_SELECT_OSC0;

	if (ipl_type() == IPL_TYPE_MPIPL)
		return -1;

	ipl_log(IPL_INFO, "Istep: set_ref_clock: started\n");

	proc = ipl_get_functional_primary_proc();
	if (proc == NULL) {
		ipl_error_callback(IPL_ERR_PRI_PROC_NON_FUNC);
		return 1;
	}

	if (initialize_and_check_clock_chip(clock_select)) {
		ipl_log(IPL_ERROR, "Clock initialization failed\n");
		return 1;
	}

	// Update clock select mode value.
	if (!pdbg_target_set_attribute(proc, "ATTR_CP_REFCLOCK_SELECT", 1, 1,
				       &clock_select)) {
		ipl_log(IPL_ERROR,
			"Attribute CP_REFCLOCK_SELECT update failed"
			" for proc %d \n",
			pdbg_target_index(proc));
		ipl_plat_procedure_error_handler(IPL_ERR_ATTR_WRITE);
		rc++;
		return 1;
	}

	ipl_log(IPL_INFO,
		"Running p10_setup_ref_clock HWP on primary processor %d\n",
		pdbg_target_index(proc));
	fapirc = p10_setup_ref_clock(proc);
	if (fapirc != fapi2::FAPI2_RC_SUCCESS) {
		ipl_log(IPL_ERROR,
			"Istep set_ref_clock failed on chip %s, rc=%d \n",
			pdbg_target_path(proc), fapirc);
		rc++;
	}

	ipl_process_fapi_error(fapirc, proc);
	return rc;
}

static int ipl_proc_clock_test(void)
{
	struct pdbg_target *proc;
	int rc = 0;
	fapi2::ReturnCode fapirc;

	if (ipl_type() == IPL_TYPE_MPIPL)
		return -1;

	ipl_log(IPL_INFO, "Istep: proc_clock_test: started\n");

	proc = ipl_get_functional_primary_proc();
	if (proc == NULL) {
		ipl_error_callback(IPL_ERR_PRI_PROC_NON_FUNC);
		return 1;
	}

	ipl_log(IPL_INFO,
		"Running p10_clock_test HWP on primary processor %d\n",
		pdbg_target_index(proc));
	fapirc = p10_clock_test(proc);
	if (fapirc != fapi2::FAPI2_RC_SUCCESS) {
		ipl_log(IPL_ERROR, "HWP clock_test failed on proc %d, rc=%d\n",
			pdbg_target_index(proc), fapirc);
		rc++;
	}

	ipl_process_fapi_error(fapirc, proc);

	return rc;
}

static int ipl_proc_prep_ipl(void)
{
	return -1;
}

static int ipl_edmarepair(void)
{
	return -1;
}

static int ipl_asset_protection(void)
{
	return -1;
}

static int ipl_proc_select_boot_prom(void)
{
	struct pdbg_target *proc;
	int rc = 1;

	ipl_log(IPL_INFO, "Istep: proc_select_boot_prom: started\n");

	// Check the availabilty of primary processor.
	if (!ipl_check_functional_master()) {
		ipl_error_callback(IPL_ERR_PRI_PROC_NON_FUNC);
		return 1;
	}

	pdbg_for_each_class_target("proc", proc)
	{
		fapi2::ReturnCode fapirc;

		if (!ipl_is_master_proc(proc) || !ipl_is_functional(proc))
			continue;

		ipl_log(IPL_INFO,
			"Running p10_select_boot_master HWP on processor %d\n",
			pdbg_target_index(proc));
		fapirc = p10_select_boot_master(proc);
		if (fapirc == fapi2::FAPI2_RC_SUCCESS)
			rc = 0;

		ipl_process_fapi_error(fapirc, proc);
		break;
	}

	return rc;
}

static int ipl_hb_config_update(void)
{
	return -1;
}

static int ipl_sbe_config_update(void)
{
	struct pdbg_target *root, *proc;
	int rc = 1;
	uint8_t istep_mode, core_mode, disable_security, attr_override;
	uint8_t scom_allowed;

	fapi2::buffer<uint32_t> boot_flags;

	ipl_log(IPL_INFO, "Istep: sbe_config_update: started\n");

	// Check the availabilty of primary processor.
	if (!ipl_check_functional_master()) {
		ipl_error_callback(IPL_ERR_PRI_PROC_NON_FUNC);
		return 1;
	}

	root = pdbg_target_root();
	if (!pdbg_target_get_attribute(root, "ATTR_ISTEP_MODE", 1, 1,
				       &istep_mode)) {
		ipl_log(IPL_ERROR,
			"Attribute [ATTR_ISTEP_MODE] read failed \n");
		return 1;
	}

	// Bit 0 indicates istep IPL (0b1) (Used by SBE, HB – ATTR_ISTEP_MODE)
	if (istep_mode)
		boot_flags.setBit(0);
	else
		boot_flags.clearBit(0);

	// Set the Security Disable bit based on the ATTR_DISABLE_SECURITY
	// attribute. Its "0" by default, i.e. security is always enabled by
	// default, unless user overrides the value.
	if (!pdbg_target_get_attribute(root, "ATTR_DISABLE_SECURITY", 1, 1,
				       &disable_security)) {
		ipl_log(IPL_ERROR,
			"Attribute [ATTR_DISABLE_SECURITY] read failed \n");
		return 1;
	}

	// Bit 4 and 5 Enable SBE FFDC collection.
	boot_flags.setBit(4);
	boot_flags.setBit(5);

	// Bit 6 – disable security. 0b1 indicates disable the security
	if (disable_security)
		boot_flags.setBit(6);
	else
		boot_flags.clearBit(6);

	// bit 7 - Allow hostboot attribute overrides. 0b1 indicates enable
	if (!pdbg_target_get_attribute(root, "ATTR_ALLOW_ATTR_OVERRIDES", 1, 1,
				       &attr_override)) {
		ipl_log(IPL_ERROR,
			"Attribute [ATTR_ALLOW_ATTR_OVERRIDES] read failed \n");
		return 1;
	}
	if (attr_override)
		boot_flags.setBit(7);
	else
		boot_flags.clearBit(7);

	// bit 11 - Disable denial list based SCOM access. 0b1 indicates disable
	if (!pdbg_target_get_attribute(root, "ATTR_NO_XSCOM_ENFORCEMENT", 1, 1,
				       &scom_allowed)) {
		ipl_log(IPL_ERROR,
			"Attribute [ATTR_NO_XSCOM_ENFORCEMENT] read failed \n");
		return 1;
	}
	if (scom_allowed)
		boot_flags.setBit(11);
	else
		boot_flags.clearBit(11);

	if (!pdbg_target_set_attribute(root, "ATTR_BOOT_FLAGS", 4, 1,
				       &boot_flags)) {
		ipl_log(IPL_ERROR,
			"Attribute [ATTR_BOOT_FLAGS] update failed \n");
		return 1;
	}

	if (small_core_enabled())
		core_mode = 0x00; /* CORE_UNFUSED mode */
	else
		core_mode = 0x01; /* CORE_FUSED mode */

	if (!pdbg_target_set_attribute(root, "ATTR_FUSED_CORE_MODE", 1, 1,
				       &core_mode)) {
		ipl_log(IPL_ERROR,
			"Attribute [ATTR_FUSED_CORE_MODE] update failed \n");
		return 1;
	}

	pdbg_for_each_class_target("proc", proc)
	{
		fapi2::ReturnCode fapirc;

		// Run HWP only on functional master processor
		if (!ipl_is_master_proc(proc) || !ipl_is_functional(proc))
			continue;

		ipl_log(IPL_INFO,
			"Running p10_setup_sbe_config HWP on processor %d\n",
			pdbg_target_index(proc));
		fapirc = p10_setup_sbe_config(proc);
		if (fapirc == fapi2::FAPI2_RC_SUCCESS)
			rc = 0;

		ipl_process_fapi_error(fapirc, proc);
		break;
	}

	return rc;
}

static int ipl_sbe_start(void)
{
	struct pdbg_target *proc;
	int rc = 1, ret = 0;

	ipl_log(IPL_INFO, "Istep: sbe_start: started\n");

	pdbg_for_each_class_target("proc", proc)
	{
		fapi2::ReturnCode fapirc;

		if (!ipl_is_functional(proc))
			continue;

		if (ipl_mode() == IPL_CRONUS) {
			ipl_log(IPL_INFO,
				"Running p10_start_cbs HWP on processor %d\n",
				pdbg_target_index(proc));
			fapirc = p10_start_cbs(proc, true);
			if (fapirc != fapi2::FAPI2_RC_SUCCESS)
				ret++;

			ipl_process_fapi_error(fapirc, proc);
			rc = ret;
			continue;
		}

		// Run HWP or MPIPL chip-op only on master processor in
		// non cronus mode
		if (ipl_is_master_proc(proc)) {
			if (ipl_type() == IPL_TYPE_MPIPL) {
				ipl_error_type err =
				    ipl_sbe_mpipl_continue(proc);
				ipl_error_callback(err);
				rc = err;
			} else {
				ipl_error_type err_type = IPL_ERR_OK;

				fapirc = p10_start_cbs(proc, true);
				if (fapirc == fapi2::FAPI2_RC_SUCCESS) {
					// Update Primary processor SBE state to
					// check cfam. Boot error callback is
					// only required for failure.
					ipl_set_sbe_state(proc,
							  SBE_STATE_CHECK_CFAM);

					if (!ipl_sbe_booted(proc, 25)) {
						ipl_log(IPL_ERROR,
							"SBE did not boot\n");
						err_type = IPL_ERR_SBE_BOOT;
					} else {
						// Update Primary processor SBE
						// state to booted Boot error
						// callback is only required for
						// failure.
						ipl_set_sbe_state(
						    proc, SBE_STATE_BOOTED);
						rc = 0;
					}
				} else {
					err_type = IPL_ERR_HWP;
				}
				ipl_error_callback(err_type);
				break;
			}
		}
	}

	if (!rc) {
		// Update Secondary processors SBE state to check cfam
		// Boot error callback is required for failue
		ipl_set_sbe_state_all_sec(SBE_STATE_CHECK_CFAM);

		if (!ipl_check_functional_master()) {
			ipl_error_callback(IPL_ERR_PRI_PROC_NON_FUNC);
			return 1;
		}
	}

	return rc;
}

static int ipl_startPRD(void)
{
	return -1;
}

static int ipl_proc_attn_listen(void)
{
	struct pdbg_target *fsi, *proc = NULL;
	uint32_t regval; // for register read/write
	char path[16];
	int rc;

	pdbg_for_each_class_target("proc", proc)
	{
		if (!ipl_is_functional(proc))
			continue;

		if (ipl_is_master_proc(proc))
			break;
	}

	if (!proc) {
		ipl_error_callback(IPL_ERR_PRI_PROC_NON_FUNC);
		return 1;
	}

	sprintf(path, "/proc%d/fsi", pdbg_target_index(proc));
	fsi = pdbg_target_from_path(NULL, path);
	if (!fsi) {
		ipl_error_callback(IPL_ERR_FSI_TGT_NOT_FOUND);
		return 1;
	}

	ipl_log(IPL_INFO, "enable attention listen on processor %d\n",
		pdbg_target_index(proc));

	rc = fsi_read(fsi, 0x100d, &regval); // FSI2_PIB_TRUE_MASK
	if (rc != 0) {
		ipl_log(IPL_ERROR, "read TRUEMASK register failed, rc=%d\n",
			rc);
	} else {
		// ANY_ERROR, RECOVERABLE_ERROR
		regval &= ~0x90000000; // mask

		// SYSTEM_CHECKSTOP, SPECIAL_ATTENTION,
		// SELFBOOT_ENGINE_ATTENTION
		regval |= 0x60000002; // un-mask

		rc = fsi_write(fsi, 0x100d, regval); // FSI2_PIB_TRUE_MASK
		if (rc != 0) {
			ipl_log(IPL_ERROR,
				"write TRUEMASK register failed, rc=%d\n", rc);
		}
	}

	ipl_error_callback((rc == 0) ? IPL_ERR_OK : IPL_ERR_FSI_REG);
	return rc;
}

static struct ipl_step ipl0[] = {
    {IPL_DEF(poweron), 0, 1, true, true},
    {IPL_DEF(startipl), 0, 2, true, true},
    {IPL_DEF(DisableAttns), 0, 3, true, true},
    {IPL_DEF(updatehwmodel), 0, 4, true, true},
    {IPL_DEF(alignment_check), 0, 5, true, true},
    {IPL_DEF(set_ref_clock), 0, 6, true, true},
    {IPL_DEF(proc_clock_test), 0, 7, true, true},
    {IPL_DEF(proc_prep_ipl), 0, 8, true, true},
    {IPL_DEF(edmarepair), 0, 9, true, true},
    {IPL_DEF(asset_protection), 0, 10, true, true},
    {IPL_DEF(proc_select_boot_prom), 0, 11, true, true},
    {IPL_DEF(hb_config_update), 0, 12, true, true},
    {IPL_DEF(sbe_config_update), 0, 13, true, true},
    {IPL_DEF(sbe_start), 0, 14, true, true},
    {IPL_DEF(startPRD), 0, 15, true, true},
    {IPL_DEF(proc_attn_listen), 0, 16, true, true},
    {NULL, NULL, -1, -1, false, false},
};

__attribute__((constructor)) static void ipl_register_ipl0(void)
{
	ipl_register(0, ipl0, ipl_pre0);
}
