extern "C" {
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>

#include <libpdbg.h>
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
#include <filesystem>
#include <fstream>
#include <array>

#define FRU_TYPE_CORE   0x07
#define FRU_TYPE_MC     0x44


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
	uint8_t *match_path = (uint8_t *)priv;
	uint8_t path[21] = { 0 };
	uint8_t type;

	if (!pdbg_target_get_attribute(target, "ATTR_PHYS_BIN_PATH", 1, 21, path))
	  	//Returning 0 for continue traversal, as the requested target is not
		//found
		return 0;

	if (memcmp(match_path, path, sizeof(path)) != 0)
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
			ipl_log(IPL_ERROR, "Failed to clear functional state of core, \
			                    index=0x%x\n", pdbg_target_index(target));
			//Unable to clear the functional state of HWAS attribute of the
			//target, so we need to stop
			return 2;
		}

	} else if (ipl_type() == IPL_TYPE_NORMAL && type != FRU_TYPE_MC) {

		if (!set_or_clear_state(target, false)) {
			ipl_log(IPL_ERROR, "Failed to clear functional state of fru \
			                    type 0x%x\n", type);
			//Unable to clear the functional state of HWAS attribute of the
			//target, so we need to stop
			return 2;
		}

	} else {
		ipl_log(IPL_DEBUG, "Skipping to clear functional state for \
		                   fru type 0x%x in ipl mode %d\n", type, ipl_type());
	}

	//Requested target found
	return 1;
}

//@Brief Function will get the guard records and will unset the functional
//state of the guarded resources in HWAS state attribute in device tree.
static void update_hwas_state(void)
{
	openpower::guard::libguard_init(false);

	auto records = openpower::guard::getAll();
	if (records.size()) {
		ipl_log(IPL_INFO, "Number of Records = %d\n",records.size());

		for (const auto& elem : records) {
			uint8_t path[21];
			int index = 0, i, err;

			memset(path, 0, sizeof(path));

			path[index] = elem.targetId.type_size;
			index += 1;

			for (i = 0; i < (0x0F & elem.targetId.type_size); i++) {

				path[index] = elem.targetId.pathElements[i].targetType;
				path[index+1] = elem.targetId.pathElements[i].instance;

				index += sizeof(elem.targetId.pathElements[0]);
			}

			err = pdbg_target_traverse(NULL, update_hwas_state_callback, path);
			if ((err == 0) || (err == 2))
				ipl_log(IPL_ERROR, "Failed to set HWAS state for guard \
				          record[ID: %d]\n", elem.recordId);
		}
	}
}

//@Brief Function will set the functional and present state of master proc
// and of available procs in the system. For few children of master proc
// functional and present state will be set in device tree during genesis
// boot.
static bool update_genesis_hwas_state(void)
{
	std::array<const char*, 6> mProcChild =
		{"core", "pauc", "pau", "iohs", "mc", "chiplet"};
	struct pdbg_target *proc, *child;

	pdbg_for_each_class_target("proc", proc) {
		if (pdbg_target_status(proc) != PDBG_TARGET_ENABLED)
			continue;

		if (!set_or_clear_state(proc, true)) {
			ipl_log(IPL_ERROR,
				"Failed to set HWAS state of proc %d\n",
				pdbg_target_index(proc));
			return false;
		}

		if(!ipl_is_master_proc(proc))
			continue;

		for(const char* data : mProcChild) {
			pdbg_for_each_target(data, proc, child) {
				if (!set_or_clear_state(child, true)) {
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

static int ipl_updatehwmodel(void)
{
	namespace fs = std::filesystem;
	constexpr auto GENESIS_BOOT_FILE = "/var/lib/phal/genesisboot";
	fs::path genesis_boot_file = GENESIS_BOOT_FILE;

	ipl_log(IPL_INFO, "Istep: updatehwmodel: started\n");

	if (!fs::exists(genesis_boot_file)) {
		ipl_log(IPL_INFO, "updatehwmodel: Genesis mode boot\n");
		if(!update_genesis_hwas_state()) {
			ipl_log(IPL_ERROR,"Failed to set genesis boot state\n");
			return 1;
		}

		//Create new file to skip the genesis setup in next boot.
		if (!fs::create_directories(genesis_boot_file.parent_path())){
			ipl_log(IPL_ERROR,"Failed to create genesis boot file\n");
			return 1;
		}
		std::ofstream file(GENESIS_BOOT_FILE);
	}

	update_hwas_state();
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

		if (pdbg_target_status(proc) != PDBG_TARGET_ENABLED)
			continue;

		ipl_log(IPL_INFO, "Running p10_setup_ref_clock HWP on processor %d\n",
			pdbg_target_index(proc));
		fapirc = p10_setup_ref_clock(proc);
		if (fapirc != fapi2::FAPI2_RC_SUCCESS) {
			ipl_log(IPL_ERROR, "Istep set_ref_clock failed on chip %d, rc=%d\n",
                                pdbg_target_index(proc), fapirc);
			rc++;
		}

		ipl_error_callback(fapirc == fapi2::FAPI2_RC_SUCCESS);
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

		if (pdbg_target_status(proc) != PDBG_TARGET_ENABLED)
			continue;

		ipl_log(IPL_INFO,"Running p10_clock_test HWP on processor %d\n",
			pdbg_target_index(proc));
		fapirc = p10_clock_test(proc);
		if (fapirc != fapi2::FAPI2_RC_SUCCESS) {
			ipl_log(IPL_ERROR, "HWP clock_test failed on proc %d, rc=%d\n",
				pdbg_target_index(proc), fapirc);
			rc++;
		}

		ipl_error_callback(fapirc == fapi2::FAPI2_RC_SUCCESS);
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

		if (pdbg_target_status(proc) != PDBG_TARGET_ENABLED)
			continue;

		if (!ipl_is_master_proc(proc))
			continue;

		ipl_log(IPL_INFO,"Running p10_select_boot_master HWP on processor %d\n",
			pdbg_target_index(proc));
		fapirc = p10_select_boot_master(proc);
		if (fapirc == fapi2::FAPI2_RC_SUCCESS)
			rc = 0;

		ipl_error_callback(fapirc == fapi2::FAPI2_RC_SUCCESS);
		break;
        }

	return rc;
}

static int ipl_hb_config_update(void)
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

static int ipl_sbe_config_update(void)
{
	struct pdbg_target *root, *proc;
	uint32_t boot_flags = 0;
	int rc = 1;
	uint8_t istep_mode, core_mode;

	ipl_log(IPL_INFO, "Istep: sbe_config_update: started\n");

	root = pdbg_target_root();
	if (!pdbg_target_get_attribute(root, "ATTR_ISTEP_MODE", 1, 1, &istep_mode)) {
		ipl_log(IPL_ERROR, "Attribute [ATTR_ISTEP_MODE] read failed \n");
		return 1;
	}

	// Bit 0 indicates istep IPL (0b1) (Used by SBE, HB – ATTR_ISTEP_MODE)
	if (istep_mode)
		boot_flags = 0x80000000;

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

		if (pdbg_target_status(proc) != PDBG_TARGET_ENABLED)
			continue;

		// Run HWP only on master processor
		if (!ipl_is_master_proc(proc))
			continue;

		ipl_log(IPL_INFO, "Running p10_setup_sbe_config HWP on processor %d\n",
			pdbg_target_index(proc));
		fapirc = p10_setup_sbe_config(proc);
		if (fapirc == fapi2::FAPI2_RC_SUCCESS)
			rc = 0;

		ipl_error_callback(fapirc == fapi2::FAPI2_RC_SUCCESS);
		break;
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

		if (pdbg_target_status(proc) != PDBG_TARGET_ENABLED)
			continue;

		if (ipl_mode() == IPL_CRONUS) {
			ipl_log(IPL_INFO, "Running p10_start_cbs HWP on processor %d\n",
				pdbg_target_index(proc));
			fapirc = p10_start_cbs(proc, true);
			if (fapirc != fapi2::FAPI2_RC_SUCCESS)
				ret++;

			ipl_error_callback(fapirc == fapi2::FAPI2_RC_SUCCESS);
			rc = ret;
			continue;
		}

		// Run HWP or MPIPL chip-op only on master processor in
		// non cronus mode
		if (ipl_is_master_proc(proc)) {
			if (ipl_type() == IPL_TYPE_MPIPL) {
				struct pdbg_target *pib;

				pdbg_for_each_target("pib", proc, pib) {
					ipl_log(IPL_INFO, "Running sbe_mpipl_continue on processor %d\n",
						pdbg_target_index(proc));
					ret = sbe_mpipl_continue(pib);
					if (ret != 0) {
						ipl_log(IPL_ERROR,
							"Continue MPIPL failed, ret=%d\n",
							ret);
					}

					ipl_error_callback(ret == 0);
					return ret;
				}
			} else {
				fapirc = p10_start_cbs(proc, true);
				if (fapirc == fapi2::FAPI2_RC_SUCCESS) {
					if (ipl_sbe_booted(proc, 5)) {
						rc = 0;
					} else {
						ipl_log(IPL_ERROR, "SBE did not boot\n");
						fapirc = ~(fapi2::FAPI2_RC_SUCCESS);
					}
				}

				ipl_error_callback(fapirc == fapi2::FAPI2_RC_SUCCESS);
				break;
			}
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
	return -1;
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
