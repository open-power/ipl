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

#include <ekb/chips/p10/procedures/hwp/perv/p10_start_cbs.H>
#include <ekb/chips/p10/procedures/hwp/perv/p10_setup_ref_clock.H>
#include <ekb/chips/p10/procedures/hwp/perv/p10_clock_test.H>
#include <ekb/chips/p10/procedures/hwp/perv/p10_setup_sbe_config.H>
#include <ekb/chips/p10/procedures/hwp/perv/p10_select_boot_master.H>

#include <libguard/guard_interface.hpp>
#include <libguard/guard_entity.hpp>
#include <libguard/include/guard_record.hpp>
#include <filesystem>
#include <fstream>
#include <array>

#define FRU_TYPE_CORE   0x07
#define FRU_TYPE_MC     0x44
#define FRU_TYPE_FC     0x53
#define GUARD_ERROR_TYPE_RECONFIG  0xEB

struct guard_target {
  	uint8_t path[21];
	bool set_hwas_state;

	guard_target() {
	  	memset(&path, 0, sizeof(path));
		set_hwas_state = false;
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

	if (!pdbg_target_get_attribute_packed(target,
					      "ATTR_HWAS_STATE",
					      "41", 1, buf)) {
		ipl_log(IPL_ERROR, "Attribute [ATTR_HWAS_STATE] read failed\n");
		return false;
	}

	if(do_set)
		buf[4] |= (flag_present | flag_functional);
	else
		buf[4] &= (uint8_t)(~flag_functional);

	if (!pdbg_target_set_attribute_packed(target,
					      "ATTR_HWAS_STATE",
					      "41", 1, buf)) {
		ipl_log(IPL_ERROR, "Attribute [ATTR_HWAS_STATE] write failed\n");
		return false;
	}
	return true;
}

static int update_hwas_state_callback(struct pdbg_target* target, void *priv)
{
  	guard_target* target_info = static_cast<guard_target*>(priv);
	uint8_t path[21] = { 0 };
	uint8_t type;

	if (!pdbg_target_get_attribute(target, "ATTR_PHYS_BIN_PATH", 1, 21, path))
		//Returning 0 for continue traversal, as the requested target is not
		//found
		return 0;

	if (memcmp(target_info->path, path, sizeof(path)) != 0)
		return 0;

	if(!pdbg_target_get_attribute(target, "ATTR_TYPE", 1, 1, &type)) {
		ipl_log(IPL_ERROR, "Failed to read ATTR_TYPE for %s\n",
			pdbg_target_path(target));
		//ATTR_TYPE attribute not found for the target, hence this is an
		//error case, so need to stop
		return 2;
	}

	if (ipl_type() == IPL_TYPE_MPIPL && type == FRU_TYPE_CORE) {

		if (!set_or_clear_state(target, false)) {
			ipl_log(IPL_ERROR,
				"Failed to clear functional state of core, index=0x%x\n",
				pdbg_target_index(target));
			//Unable to clear the functional state of HWAS attribute of the
			//target, so we need to stop
			return 2;
		}

	} else if (ipl_type() == IPL_TYPE_NORMAL) {

		if (!set_or_clear_state(target, target_info->set_hwas_state)) {
			ipl_log(IPL_ERROR,
				"Failed to clear functional state of fru type 0x%x\n",
				type);
			//Unable to clear the functional state of HWAS attribute of the
			//target, so we need to stop
			return 2;
		}

		if((type == FRU_TYPE_FC) && !small_core_enabled()) {
			struct pdbg_target *core;

			pdbg_for_each_target("core", target, core) {
				if (!set_or_clear_state(core, false)) {
					ipl_log(IPL_ERROR,
						"Failed to clear functional state"
						" of core with index 0x%x\n",
						pdbg_target_index(core));
					// Unable to clear the functional state of HWAS
					// attribute of the target, so we need to stop
					return 2;
				}
			}
		}

	} else {
		ipl_log(IPL_DEBUG,
			"Skipping to clear functional state for"
			" fru type 0x%x in ipl mode %d\n",
			type, ipl_type());
	}

	//Requested target found
	return 1;
}

//@Brief Function will get the guard records and will unset the functional
//state of the guarded resources in HWAS state attribute in device tree.
static void update_hwas_state(bool is_coldboot)
{
	openpower::guard::libguard_init(false);

	auto records = openpower::guard::getAll();
	if (records.size()) {
		ipl_log(IPL_INFO, "Number of Records = %d\n",records.size());

		for (const auto& elem : records) {

			//Not to apply resolved guard records
		  	if(elem.recordId == GUARD_RESOLVED) {
				continue;
			}

		  	guard_target targetinfo;
			int index = 0, i, err;

			targetinfo.path[index] = elem.targetId.type_size;
			index += 1;

			for (i = 0; i < (0x0F & elem.targetId.type_size); i++) {

				targetinfo.path[index] = elem.targetId.pathElements[i].targetType;
				targetinfo.path[index+1] = elem.targetId.pathElements[i].instance;

				index += sizeof(elem.targetId.pathElements[0]);
			}


			//Check is to remove guard record with type reconfig during
			//normal/cold boot. During this the GENESIS_BOOT_FILE will
			//get deleted as a part of DEVTREE initilization.
			//During warm reboot GENESIS_BOOT_FILE will be present. Hence
			//all the guard records will be applied.
			if(is_coldboot && (elem.errType == GUARD_ERROR_TYPE_RECONFIG) &&
			  (ipl_type() == IPL_TYPE_NORMAL)) {
			  	openpower::guard::clear(elem.targetId);
			  	targetinfo.set_hwas_state = true;
			}
			else {
			  	targetinfo.set_hwas_state = false;
			}

			err = pdbg_target_traverse(NULL, update_hwas_state_callback, &targetinfo);
			if ((err == 0) || (err == 2))
				ipl_log(IPL_ERROR,
					"Failed to set HWAS state for guard"
					" record[ID: %d]\n",
					elem.recordId);
		}
	}
}

//@Brief Function will set the functional and present state of master proc
// and of available procs in the system. For few children of master proc
// functional and present state will be set in device tree during genesis
// boot.
static bool update_genesis_hwas_state(void)
{
	std::array<const char*, 8> mProcChild =
		{"core", "pauc", "pau", "iohs", "mc", "chiplet", "pec", "fc"};
	struct pdbg_target *proc, *child;

	bool target_enabled = false;

	pdbg_for_each_class_target("proc", proc) {
		if (pdbg_target_status(proc) != PDBG_TARGET_ENABLED) {
			target_enabled = false;
		}
		else {
			target_enabled = true;
		}

		if (!set_or_clear_state(proc, target_enabled)) {
			ipl_log(IPL_ERROR,
				"Failed to set HWAS state of proc %d\n",
				pdbg_target_index(proc));
			return false;
		}

		if(!ipl_is_master_proc(proc))
			continue;

		for(const char* data : mProcChild) {
			pdbg_for_each_target(data, proc, child) {
				if (!set_or_clear_state(child, target_enabled)) {
					ipl_log(IPL_ERROR,
						"Failed to set HWAS state of %s, index %d\n",
						data, pdbg_target_index(child));
					return false;
				}
			}
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
static ipl_error_type  ipl_sbe_mpipl_continue(struct pdbg_target *proc)
{
	enum sbe_state state;
	char path[16];
	int ret = 0;

	ipl_log(IPL_INFO, "ipl_sbe_mpipl_continue: Enter(%s)", pdbg_target_path(proc));

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
		ipl_log(IPL_ERROR, "SBE (%s) is not ready for chip-op: state(0x%08x)",
			pdbg_target_path(pib), state);
		return IPL_ERR_SBE_CHIPOP;
	}

	// call pdbg back-end function
	ret = sbe_mpipl_continue(pib);
        if(ret != 0) {
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
			ipl_log(IPL_ERROR,"Failed to set genesis boot state\n");
			return 1;
		}

		//Create new file to skip the genesis setup in next boot.
		if (!fs::exists(genesis_boot_file.parent_path())) {
			if (!fs::create_directories(genesis_boot_file.parent_path())){
				ipl_log(IPL_ERROR,"Failed to create genesis boot file\n");
				return 1;
			}
		}
		boot_file_absent = true;
	  	std::ofstream file(GENESIS_BOOT_FILE);
	}

	update_hwas_state(boot_file_absent);

	if(!boot_file_absent && (ipl_type() != IPL_TYPE_MPIPL)) {
		// Update SBE state to Not usable in reboot path(not on MPIPL)
		// Boot error callback is only required for failure
		ipl_set_sbe_state_all(SBE_STATE_NOT_USABLE);
	}

	if (!ipl_check_functional_master()){
		ipl_error_callback(IPL_ERR_PRI_PROC_NON_FUNC);
		return 1;
	}

	return 0;
}

static int ipl_alignment_check(void)
{
	return -1;
}

static int ipl_set_ref_clock(void)
{
	struct pdbg_target *proc;
	int rc = 0;

	if (ipl_type() == IPL_TYPE_MPIPL)
		return -1;

	ipl_log(IPL_INFO, "Istep: set_ref_clock: started\n");

	pdbg_for_each_class_target("proc", proc) {
		fapi2::ReturnCode fapirc;

		if (!ipl_is_functional(proc))
			continue;

		ipl_log(IPL_INFO, "Running p10_setup_ref_clock HWP on processor %d\n",
			pdbg_target_index(proc));
		fapirc = p10_setup_ref_clock(proc);
		if (fapirc != fapi2::FAPI2_RC_SUCCESS) {
			ipl_log(IPL_ERROR, "Istep set_ref_clock failed on chip %s, rc=%d \n",
				pdbg_target_path(proc), fapirc);
			rc++;
		}

		ipl_process_fapi_error(fapirc, proc);
	}

	if (!ipl_check_functional_master()){
		ipl_error_callback(IPL_ERR_PRI_PROC_NON_FUNC);
		return 1;
	}

	return rc;
}

static int ipl_proc_clock_test(void)
{
	struct pdbg_target *proc;
	int rc = 0;

	if (ipl_type() == IPL_TYPE_MPIPL)
		return -1;

	ipl_log(IPL_INFO, "Istep: proc_clock_test: started\n");

	pdbg_for_each_class_target("proc", proc) {
		fapi2::ReturnCode fapirc;

		if (!ipl_is_functional(proc))
			continue;

		ipl_log(IPL_INFO,"Running p10_clock_test HWP on processor %d\n",
			pdbg_target_index(proc));
		fapirc = p10_clock_test(proc);
		if (fapirc != fapi2::FAPI2_RC_SUCCESS) {
			ipl_log(IPL_ERROR, "HWP clock_test failed on proc %d, rc=%d\n",
				pdbg_target_index(proc), fapirc);
			rc++;
		}

		ipl_process_fapi_error(fapirc, proc);
	}

	if (!ipl_check_functional_master()){
		ipl_error_callback(IPL_ERR_PRI_PROC_NON_FUNC);
		return 1;
    }

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

	pdbg_for_each_class_target("proc", proc) {
		fapi2::ReturnCode fapirc;

		if (!ipl_is_master_proc(proc) || !ipl_is_functional(proc))
			continue;

		ipl_log(IPL_INFO,"Running p10_select_boot_master HWP on processor %d\n",
			pdbg_target_index(proc));
		fapirc = p10_select_boot_master(proc);
		if (fapirc == fapi2::FAPI2_RC_SUCCESS)
			rc = 0;

		ipl_process_fapi_error(fapirc, proc);
		break;
    }

	if (!ipl_check_functional_master()) {
		ipl_error_callback(IPL_ERR_PRI_PROC_NON_FUNC);
		return 1;
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

	root = pdbg_target_root();
	if (!pdbg_target_get_attribute(root, "ATTR_ISTEP_MODE", 1, 1, &istep_mode)) {
		ipl_log(IPL_ERROR, "Attribute [ATTR_ISTEP_MODE] read failed \n");
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
	if (!pdbg_target_get_attribute(root, "ATTR_DISABLE_SECURITY", 1, 1, &disable_security)) {
		ipl_log(IPL_ERROR, "Attribute [ATTR_DISABLE_SECURITY] read failed \n");
		return 1;
	}

	// Bit 6 – disable security. 0b1 indicates disable the security
	if (disable_security)
		boot_flags.setBit(6);
	else
		boot_flags.clearBit(6);

	// bit 7 - Allow hostboot attribute overrides. 0b1 indicates enable
	if (!pdbg_target_get_attribute(root, "ATTR_ALLOW_ATTR_OVERRIDES", 1, 1, &attr_override)) {
		ipl_log(IPL_ERROR, "Attribute [ATTR_ALLOW_ATTR_OVERRIDES] read failed \n");
		return 1;
	}
	if (attr_override)
		boot_flags.setBit(7);
	else
		boot_flags.clearBit(7);

	// bit 11 - Disable denial list based SCOM access. 0b1 indicates disable
	if (!pdbg_target_get_attribute(root, "ATTR_NO_XSCOM_ENFORCEMENT", 1, 1, &scom_allowed)) {
		ipl_log(IPL_ERROR, "Attribute [ATTR_NO_XSCOM_ENFORCEMENT] read failed \n");
		return 1;
	}
	if (scom_allowed)
		boot_flags.setBit(11);
	else
		boot_flags.clearBit(11);

	if (!pdbg_target_set_attribute(root, "ATTR_BOOT_FLAGS", 4, 1, &boot_flags)) {
		ipl_log(IPL_ERROR, "Attribute [ATTR_BOOT_FLAGS] update failed \n");
		return 1;
	}

	if (small_core_enabled())
		core_mode = 0x00;  /* CORE_UNFUSED mode */
	else
		core_mode = 0x01;  /* CORE_FUSED mode */

	if (!pdbg_target_set_attribute(root, "ATTR_FUSED_CORE_MODE", 1, 1, &core_mode)) {
		ipl_log(IPL_ERROR, "Attribute [ATTR_FUSED_CORE_MODE] update failed \n");
		return 1;
	}

	pdbg_for_each_class_target("proc", proc) {
		fapi2::ReturnCode fapirc;

		// Run HWP only on functional master processor
		if (!ipl_is_master_proc(proc) || !ipl_is_functional(proc))
			continue;

		ipl_log(IPL_INFO, "Running p10_setup_sbe_config HWP on processor %d\n",
			pdbg_target_index(proc));
		fapirc = p10_setup_sbe_config(proc);
		if (fapirc == fapi2::FAPI2_RC_SUCCESS)
			rc = 0;

		ipl_process_fapi_error(fapirc, proc);
		break;
	}

	if (!ipl_check_functional_master()) {
		ipl_error_callback(IPL_ERR_PRI_PROC_NON_FUNC);
		return 1;
	}

	return rc;
}

static int ipl_sbe_start(void)
{
	struct pdbg_target *proc;
	int rc = 1, ret = 0;

	ipl_log(IPL_INFO, "Istep: sbe_start: started\n");

	pdbg_for_each_class_target("proc", proc) {
		fapi2::ReturnCode fapirc;

		if (!ipl_is_functional(proc))
			continue;

		if (ipl_mode() == IPL_CRONUS) {
			ipl_log(IPL_INFO, "Running p10_start_cbs HWP on processor %d\n",
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
				ipl_error_type err = ipl_sbe_mpipl_continue(proc);
				ipl_error_callback(err);
				rc = err;
			} else {
				ipl_error_type err_type = IPL_ERR_OK;

				fapirc = p10_start_cbs(proc, true);
				if (fapirc == fapi2::FAPI2_RC_SUCCESS) {
					// Update Primary processor SBE state to check cfam.
					// Boot error callback is only required for failure.
					ipl_set_sbe_state(proc, SBE_STATE_CHECK_CFAM);

					if (!ipl_sbe_booted(proc, 25)) {
						ipl_log(IPL_ERROR, "SBE did not boot\n");
						err_type = IPL_ERR_SBE_BOOT;
					}
					else {
						// Update Primary processor SBE state to booted
						// Boot error callback is only required for failure.
						ipl_set_sbe_state(proc, SBE_STATE_BOOTED);
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

	if(!rc) {
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

	pdbg_for_each_class_target("proc", proc) {
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
		ipl_log(IPL_ERROR, "read TRUEMASK register failed, rc=%d\n", rc);
	} else {
		// ANY_ERROR, RECOVERABLE_ERROR
		regval &= ~0x90000000;	// mask

		// SYSTEM_CHECKSTOP, SPECIAL_ATTENTION, SELFBOOT_ENGINE_ATTENTION
		regval |= 0x60000002;	// un-mask

		rc = fsi_write(fsi, 0x100d, regval); // FSI2_PIB_TRUE_MASK
		if (rc != 0) {
			ipl_log(IPL_ERROR, "write TRUEMASK register failed, rc=%d\n", rc);
		}
	}

	ipl_error_callback((rc == 0) ? IPL_ERR_OK : IPL_ERR_FSI_REG);
	return rc;
}

static struct ipl_step ipl0[] = {
	{ IPL_DEF(poweron),                   0,  1,  true,  true  },
	{ IPL_DEF(startipl),                  0,  2,  true,  true  },
	{ IPL_DEF(DisableAttns),              0,  3,  true,  true  },
	{ IPL_DEF(updatehwmodel),             0,  4,  true,  true  },
	{ IPL_DEF(alignment_check),           0,  5,  true,  true  },
	{ IPL_DEF(set_ref_clock),             0,  6,  true,  true  },
	{ IPL_DEF(proc_clock_test),           0,  7,  true,  true  },
	{ IPL_DEF(proc_prep_ipl),             0,  8,  true,  true  },
	{ IPL_DEF(edmarepair),                0,  9,  true,  true  },
	{ IPL_DEF(asset_protection),          0, 10,  true,  true  },
	{ IPL_DEF(proc_select_boot_prom),     0, 11,  true,  true  },
	{ IPL_DEF(hb_config_update),          0, 12,  true,  true  },
	{ IPL_DEF(sbe_config_update),         0, 13,  true,  true  },
	{ IPL_DEF(sbe_start),                 0, 14,  true,  true  },
	{ IPL_DEF(startPRD),                  0, 15,  true,  true  },
	{ IPL_DEF(proc_attn_listen),          0, 16,  true,  true  },
	{ NULL, NULL, -1, -1, false, false },
};

__attribute__((constructor))
static void ipl_register_ipl0(void)
{
	ipl_register(0, ipl0, ipl_pre0);
}
