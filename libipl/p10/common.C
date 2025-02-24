extern "C" {
#include <stdio.h>
#include <unistd.h>
#include <libpdbg.h>
}

#include "libipl.H"
#include "libipl_internal.H"
#include "common.H"
#include "plat_utils.H"
#include <attributes_info.H>
#include <libekb.H>
#include <error_info_defs.H>

#include <ekb/hwpf/fapi2/include/return_code_defs.H>
#include <ekb/chips/p10/procedures/hwp/istep/p10_do_fw_hb_istep.H>
#include <ekb/chips/p10/procedures/hwp/sbe/p10_get_sbe_msg_register.H>

bool ipl_is_master_proc(struct pdbg_target *proc)
{
	uint8_t type;

	if (!pdbg_target_get_attribute(proc, "ATTR_PROC_MASTER_TYPE", 1, 1,
				       &type)) {
		ipl_log(IPL_ERROR,
			"Attribute [ATTR_PROC_MASTER_TYPE] read failed \n");

		if (pdbg_target_index(proc) == 0)
			return true;

		return false;
	}

	/* Attribute value 0 corresponds to master processor */
	if (type == 0)
		return true;

	return false;
}

int ipl_istep_via_sbe(int major, int minor)
{
	struct pdbg_target *pib, *proc;
	int rc = 1;

	ipl_log(IPL_INFO, "Istep: SBE %d.%d : started\n", major, minor);

	pdbg_for_each_class_target("pib", pib)
	{
		int ret;

		if (pdbg_target_status(pib) != PDBG_TARGET_ENABLED)
			continue;

		// Run SBE isteps only on functional master processor
		proc = pdbg_target_require_parent("proc", pib);
		if (!ipl_is_master_proc(proc))
			continue;

		if (!ipl_is_functional(proc)) {
			ipl_log(IPL_ERROR,
				"Master processor (%d) is not functional\n",
				pdbg_target_index(proc));
			ipl_error_callback(IPL_ERR_PRI_PROC_NON_FUNC);
			return 1;
		}

		ipl_log(IPL_INFO, "Running sbe_istep on processor %d\n",
			pdbg_target_index(proc));

		ret = sbe_istep(pib, major, minor);
		if (ret) {
			ipl_log(IPL_ERROR,
				"Istep %d.%d failed on proc-%d, rc=%d\n", major,
				minor, pdbg_target_index(proc), ret);
			ipl_log_sbe_ffdc(pib);
		} else {
			rc = 0;
		}

		ipl_error_callback((rc == 0) ? IPL_ERR_OK : IPL_ERR_HWP);
		break;
	}

	return rc;
}

int ipl_istep_via_hostboot(int major, int minor)
{
	struct pdbg_target *proc;
	uint64_t retry_limit_ms = 30 * 60 * 1000;
	uint64_t delay_ms = 100;
	int rc = 1;

	ipl_log(IPL_INFO, "Istep: Hostboot %d.%d : started\n", major, minor);

	pdbg_for_each_class_target("proc", proc)
	{
		fapi2::ReturnCode fapi_rc;

		// Run HWP only on functional master processor
		if (!ipl_is_master_proc(proc))
			continue;

		if (!ipl_is_functional(proc)) {
			ipl_log(IPL_ERROR,
				"Master processor(%d) is not functional\n",
				pdbg_target_index(proc));
			ipl_error_callback(IPL_ERR_PRI_PROC_NON_FUNC);
			return 1;
		}

		ipl_log(IPL_INFO,
			"Running p10_do_fw_hb_istep HWP on processor %d\n",
			pdbg_target_index(proc));

		fapi_rc = p10_do_fw_hb_istep(proc, major, minor, retry_limit_ms,
					     delay_ms);
		if (fapi_rc != fapi2::FAPI2_RC_SUCCESS)
			ipl_log(IPL_ERROR,
				"Istep %d.%d failed on chip %d, rc=%d\n", major,
				minor, pdbg_target_index(proc), fapi_rc);
		else
			rc = 0;

		ipl_error_callback((rc == 0) ? IPL_ERR_OK : IPL_ERR_HWP);
		break;
	}

	return rc;
}

bool ipl_sbe_booted(struct pdbg_target *proc, uint32_t wait_time_seconds)
{
	sbeMsgReg_t sbeReg;
	fapi2::ReturnCode fapi_rc;
	uint32_t loopcount;

	loopcount = wait_time_seconds > 0 ? wait_time_seconds : 25;

	while (loopcount > 0) {
		struct pdbg_target *fsi;
	uint32_t data;
	int rc;

	if (!(fsi = fapi2::get_fsi_target(proc)))
	{	rc=fapi2::FAPI2_RC_PLAT_ERR_SEE_DATA;
	}
    else
	{
     	rc = fsi_read(fsi, 0x2809, &data);
		sbeReg.reg = data;
	}
		if (rc == fapi2::FAPI2_RC_SUCCESS) {
			if (sbeReg.sbeBooted) {
				ipl_log(IPL_INFO,
					"SBE booted. sbeReg[0x%08x] Wait time: "
					"[%d]\n",
					uint32_t(sbeReg.reg), loopcount);
				return true;
			} else {
				ipl_log(
				    IPL_DEBUG,
				    "SBE boot is in progress. sbeReg[0x%08x]\n",
				    uint32_t(sbeReg.reg));
			}
		} else {
			ipl_log(IPL_ERROR,
				"p10_get_sbe_msg_register failed for proc %d, "
				"rc=%d\n",
				pdbg_target_index(proc), fapi_rc);
		}

		loopcount--;
		sleep(1);
	}
	// Get SBE debug data.
	uint32_t val = 0xFFFFFFFF;
	char path[16];
	sprintf(path, "/proc%d/fsi", pdbg_target_index(proc));
	struct pdbg_target *fsi = pdbg_target_from_path(NULL, path);
	if (fsi) {
		if (fsi_read(fsi, 0x1007, &val)) {
			ipl_log(IPL_ERROR, "CFAM(0x1007) on %s failed",
				pdbg_target_path(fsi));
		}
	}
	ipl_log(IPL_ERROR, "SBE Debug Data: 0x2809[0x%08x]  0x1007[0x%08x] \n",
		uint32_t(sbeReg.reg), val);
	return false;
}

bool ipl_is_present(struct pdbg_target *target)
{
	uint8_t buf[5];

	if (!pdbg_target_get_attribute_packed(target, "ATTR_HWAS_STATE", "41",
					      1, buf)) {
		ipl_log(IPL_ERROR, "Attribute [ATTR_HWAS_STATE] read failed\n");

		if (pdbg_target_status(target) == PDBG_TARGET_ENABLED)
			return true;

		return false;
	}

	// Present bit is stored in 4th byte and bit 2 position in HWAS_STATE
	return (buf[4] & 0x40);
}

bool ipl_is_functional(struct pdbg_target *target)
{
	uint8_t buf[5];

	if (!pdbg_target_get_attribute_packed(target, "ATTR_HWAS_STATE", "41",
					      1, buf)) {
		ipl_log(IPL_INFO, "Attribute [ATTR_HWAS_STATE] read failed\n");

		// Checking pdbg functional state
		if (pdbg_target_status(target) == PDBG_TARGET_ENABLED)
			return true;

		return false;
	}

	// isFuntional bit is stored in 4th byte and bit 3 position in
	// HWAS_STATE
	return (buf[4] & 0x20);
}

bool ipl_check_functional_master(void)
{
	struct pdbg_target *proc;

	pdbg_for_each_class_target("proc", proc)
	{
		if (!ipl_is_master_proc(proc))
			continue;

		if (!ipl_is_functional(proc)) {
			ipl_log(IPL_ERROR,
				"Master processor(%d) is not functional\n",
				pdbg_target_index(proc));
			return false;
		}
	}

	return true;
}

struct pdbg_target *ipl_get_functional_primary_proc(void)
{
	struct pdbg_target *proc = NULL;

	pdbg_for_each_class_target("proc", proc)
	{
		if (!ipl_is_master_proc(proc))
			continue;

		if (!ipl_is_functional(proc)) {
			ipl_log(IPL_ERROR,
				"Primary processor(%d) is not functional\n",
				pdbg_target_index(proc));
			return NULL;
		}
		return proc;
	}
	return NULL;
}

void ipl_log_sbe_ffdc(struct pdbg_target *pib)
{
	uint8_t *ffdc = NULL;
	uint32_t status = 0;
	uint32_t ffdc_len = 0;
	int ret;

	ret = sbe_ffdc_get(pib, &status, &ffdc, &ffdc_len);
	if (ret != 0) {
		ipl_log(IPL_ERROR, "%s: sbe_ffdc_get function failed (%d)",
			pdbg_target_path(pib), ret);
	} else {
		uint32_t i;

		ipl_log(IPL_INFO, "%s status:0x%x ffdc_len:0x%x\n",
			pdbg_target_path(pib), status, ffdc_len);
		ipl_log(IPL_DEBUG, "SBE FFDC data");
		for (i = 0; i < ffdc_len; i++) {
			if (i % 32 == 0)
				ipl_log(IPL_DEBUG, "\n");
			ipl_log(IPL_DEBUG, "%02x", ffdc[i]);
		}
		ipl_log(IPL_DEBUG, "\nSBE FFDC data\n");
	}

	// free ffdc data
	if (ffdc)
		free(ffdc);
}

int ipl_set_sbe_state(struct pdbg_target *proc, enum sbe_state state)
{
	ipl_log(IPL_INFO, "(%s)SBE state update (%d) \n",
		pdbg_target_path(proc), state);

	// get PIB target
	char path[16];
	sprintf(path, "/proc%d/pib", pdbg_target_index(proc));
	struct pdbg_target *pib = pdbg_target_from_path(nullptr, path);
	if (pib == nullptr) {
		ipl_log(IPL_ERROR, "Failed to get PIB target for(%s)",
			pdbg_target_path(proc));
		ipl_error_callback(IPL_ERR_PIB_TGT_NOT_FOUND);
		return 1;
	}
	// PIB already probed as part of ipl init function.
	// update SBE state
	if (sbe_set_state(pib, state)) {
		ipl_log(IPL_ERROR,
			"Failed to update SBE state information (%s)",
			pdbg_target_path(proc));
		ipl_error_callback(IPL_ERR_FSI_REG);
		return 1;
	}
	return 0;
}

int ipl_set_sbe_state_all(enum sbe_state state)
{
	struct pdbg_target *proc;
	int ret = 0;
	pdbg_for_each_class_target("proc", proc)
	{
		if (ipl_is_present(proc)) {
			if (ipl_set_sbe_state(proc, state)) {
				ret = 1;
			}
		}
	}
	return ret;
}

int ipl_set_sbe_state_all_sec(enum sbe_state state)
{
	struct pdbg_target *proc;
	int ret = 0;
	pdbg_for_each_class_target("proc", proc)
	{
		if (ipl_is_master_proc(proc))
			continue;
		if (ipl_is_present(proc)) {
			if (ipl_set_sbe_state(proc, state)) {
				ret = 1;
			}
		}
	}
	return ret;
}

void ipl_process_fapi_error(const fapi2::ReturnCode &fapirc,
			    struct pdbg_target *target, bool deconfig)
{
	if (fapirc == fapi2::FAPI2_RC_SUCCESS) {
		ipl_error_callback(IPL_ERR_OK);
	} else if (fapirc.getCreator() == fapi2::ReturnCode::CREATOR_PLAT) {
		CDG_Target cdgTarget;
		cdgTarget.callout_priority =
		    fapi2::plat_CalloutPriority_tostring(
			fapi2::CalloutPriorities::MEDIUM);

		ATTR_PHYS_BIN_PATH_Type physBinPath;
		uint32_t binPathElemCount =
		    dtAttr::fapi2::ATTR_PHYS_BIN_PATH_ElementCount;
		if (!pdbg_target_get_attribute(
			target, "ATTR_PHYS_BIN_PATH",
			std::stoi(dtAttr::fapi2::ATTR_PHYS_BIN_PATH_Spec),
			binPathElemCount, physBinPath)) {
			ipl_log(
			    IPL_ERROR,
			    "Failed to read ATTR_PHYS_BIN_PATH for target %s\n",
			    pdbg_target_path(target));
			ipl_error_callback(IPL_ERR_ATTR_READ_FAIL);
		} else {
			std::copy(
			    physBinPath, physBinPath + binPathElemCount,
			    std::back_inserter(cdgTarget.target_entity_path));
			cdgTarget.deconfigure = deconfig;

			FFDC ffdc;
			ffdc.hwp_errorinfo.cdg_targets.push_back(cdgTarget);
			ffdc.ffdc_type = FFDC_TYPE_HWP;
			uint32_t rc = fapirc;
			ffdc.hwp_errorinfo.rc = std::to_string(rc);
			ffdc.hwp_errorinfo.rc_desc =
			    "Error in executing platform function";

			ProcedureCallout proc_callout;
			proc_callout.proc_callout =
			    fapi2::plat_ProcedureCallout_tostring(
				fapi2::ProcedureCallouts::ProcedureCallout::
				    BUS_CALLOUT);
			proc_callout.callout_priority =
			    fapi2::plat_CalloutPriority_tostring(
				fapi2::CalloutPriorities::CalloutPriority::
				    MEDIUM);
			ffdc.hwp_errorinfo.procedures_callout.push_back(
			    proc_callout);
			ipl_error_callback({IPL_ERR_PLAT, &ffdc});
		}
	} else if (fapirc.getCreator() == fapi2::ReturnCode::CREATOR_HWP) {
		ipl_error_callback(IPL_ERR_HWP);
	} else {
		ipl_log(IPL_ERROR, "Unknown fapi error 0x%08X, ignoring\n",
			fapirc);
	}
}

/**
 * @brief get the error message corresponding to the ipl_error_type given
 *
 * Return the error message, if it existing for given error_type. If its not
 * available try with backup error_type. If that is also not available, return
 * default error message.
 *
 * @param[in] err_type error type
 * @param[in] bkp_err_type backup error type
 *
 * @return Error message corresponding to err_type
 */
static std::string ipl_get_err_msg(const ipl_error_type &err_type,
				   const ipl_error_type &bkp_err_type)
{
	std::string ret_msg = "Undefined error";

	auto err_msg_it = err_msg_map.find(err_type);
	if (err_msg_it != err_msg_map.end()) {
		ret_msg = err_msg_it->second;
	} else {
		auto err_msg_it = err_msg_map.find(bkp_err_type);
		if (err_msg_it != err_msg_map.end()) {
			ret_msg = err_msg_it->second;
		}
	}
	return ret_msg;
}

/**
 * @brief Function to check clock redundant mode is enabled in the system
 *
 * if the functional clock targets is greater than one redundant mode
 * is enabled. Default behaviour is redundant mode is disabled.
 * Any internal failures during this function execution results  redundant
 * mode as disabled.
 *
 * return true if clock redundant mode is enabled, false otherwise
 */

bool ipl_clock_is_redundancy_enabled()
{
	fprintf(stderr, "ipl_clock_is_redundancy_enabled \n");

	struct pdbg_target *clock_target;
	uint8_t clock_count = 0;
	pdbg_for_each_class_target("oscrefclk", clock_target)
	{
		if (!ipl_is_functional(clock_target))
			continue;
		clock_count++;
	}

	if (clock_count != 1 && clock_count != 2) {
		ipl_log(IPL_ERROR,
			"Invalid number (%d) of clock target found\n",
			clock_count);
		return false;
	}

	if (clock_count == NUM_CLOCK_FOR_REDUNDANT_MODE) {
		ipl_log(IPL_INFO, "Clock redundant mode(%d) enabled",
			clock_count);
		return true;
	}

	ipl_log(IPL_INFO, "Clock redundant mode(%d) disabled", clock_count);
	return false;
}
void ipl_plat_clock_error_handler(
    const std::vector<std::pair<std::string, std::string>> &ffdcs_data,
    uint8_t clk_pos)
{
	FFDC ffdc;
	HWCallout hwcallout_data;

	if (ipl_clock_is_redundancy_enabled()) {
		// Redudancy enaabled case
		ffdc.ffdc_type = FFDC_TYPE_SPARE_CLOCK_INFO;

		// Get clock target
		struct pdbg_target *clock_target;
		uint8_t attr_clk_pos = 0;
		pdbg_for_each_class_target("oscrefclk", clock_target)
		{
			if (!pdbg_target_get_attribute(clock_target,
						       "ATTR_POSITION", 2, 1,
						       &attr_clk_pos)) {
				ipl_log(IPL_ERROR,
					"Attribute ATTR_POSITION read failed"
					" for clock '%s' \n",
					pdbg_target_path(clock_target));
				ipl_plat_procedure_error_handler(
				    IPL_ERR_ATTR_READ_FAIL);
				return;
			}
			if (attr_clk_pos == clk_pos)
				break;
		}

		// clock_target returns null incase pdbg_for_each_class_target()
		// fail to match target
		if (clock_target == nullptr) {
			ipl_log(IPL_ERROR,
				"oscrefclk target not found"
				" for clock position(%d) \n",
				clk_pos);
			ipl_plat_procedure_error_handler(
			    IPL_ERR_PDBG_TARGET_NOT_FOUND);
			return;
		}

		ffdc.hwp_errorinfo.rc = std::to_string(IPL_ERR_SPARE_CLK);
		ffdc.hwp_errorinfo.rc_desc =
		    ipl_get_err_msg(IPL_ERR_SPARE_CLK, IPL_ERR_PLAT);
		ffdc.hwp_errorinfo.ffdcs_data.insert(
		    ffdc.hwp_errorinfo.ffdcs_data.end(), ffdcs_data.begin(),
		    ffdcs_data.end());

		CDG_Target cdgTarget;
		ATTR_PHYS_BIN_PATH_Type physBinPath;
		uint32_t binPathElemCount =
		    dtAttr::fapi2::ATTR_PHYS_BIN_PATH_ElementCount;
		if (!pdbg_target_get_attribute(
			clock_target, "ATTR_PHYS_BIN_PATH",
			std::stoi(dtAttr::fapi2::ATTR_PHYS_BIN_PATH_Spec),
			binPathElemCount, physBinPath)) {
			ipl_log(
			    IPL_ERROR,
			    "Failed to read ATTR_PHYS_BIN_PATH for target %s\n",
			    pdbg_target_path(clock_target));
			ipl_error_callback(IPL_ERR_ATTR_READ_FAIL);
			return;
		}
		std::copy(physBinPath, physBinPath + binPathElemCount,
			  std::back_inserter(cdgTarget.target_entity_path));
		cdgTarget.deconfigure = true;
		cdgTarget.guard = false;
		cdgTarget.callout = false;
		ffdc.hwp_errorinfo.cdg_targets.push_back(cdgTarget);

		// No planar callout.
		hwcallout_data.isPlanarCallout = false;

	} else {
		ffdc.ffdc_type = FFDC_TYPE_HWP;
		ffdc.hwp_errorinfo.rc = std::to_string(IPL_ERR_CLK);
		ffdc.hwp_errorinfo.rc_desc =
		    ipl_get_err_msg(IPL_ERR_CLK, IPL_ERR_PLAT);

		ffdc.hwp_errorinfo.ffdcs_data.insert(
		    ffdc.hwp_errorinfo.ffdcs_data.end(), ffdcs_data.begin(),
		    ffdcs_data.end());

		hwcallout_data.isPlanarCallout = true;
	}
	hwcallout_data.hwid = fapi2::plat_HwCalloutEnum_tostring(
	    fapi2::HwCallouts::PROC_REF_CLOCK);
	hwcallout_data.callout_priority = fapi2::plat_CalloutPriority_tostring(
	    fapi2::CalloutPriorities::HIGH);
	hwcallout_data.clkPos = clk_pos;
	// No need for entity path, not used by pel code.
	ffdc.hwp_errorinfo.hwcallouts.push_back(hwcallout_data);

	// Use "ipl_error_type" as "IPL_ERR_PLAT" since it is plat
	// specific error and the exact error code and description are
	// included in the FFDC.RC and FFDC.RC_DESC members.
	ipl_error_callback(ipl_error_info{IPL_ERR_PLAT, &ffdc});
}

void ipl_plat_procedure_error_handler(
    const ipl_error_type &err_type,
    const std::vector<std::pair<std::string, std::string>> &ffdcs_data)
{
	FFDC ffdc;
	ffdc.ffdc_type = FFDC_TYPE_HWP;
	ffdc.hwp_errorinfo.rc = std::to_string(err_type);
	ffdc.hwp_errorinfo.rc_desc = ipl_get_err_msg(err_type, IPL_ERR_PLAT);

	ffdc.hwp_errorinfo.ffdcs_data.insert(
	    ffdc.hwp_errorinfo.ffdcs_data.end(), ffdcs_data.begin(),
	    ffdcs_data.end());

	ProcedureCallout procedurecallout_data;
	procedurecallout_data.proc_callout =
	    fapi2::plat_ProcedureCallout_tostring(
		fapi2::ProcedureCallouts::ProcedureCallout::CODE);
	procedurecallout_data.callout_priority =
	    fapi2::plat_CalloutPriority_tostring(
		fapi2::CalloutPriorities::HIGH);
	ffdc.hwp_errorinfo.procedures_callout.push_back(procedurecallout_data);

	// Use "ipl_error_type" as "IPL_ERR_PLAT" since it is plat specific
	// error and the exact error code and description are included in the
	// FFDC.RC and FFDC.RC_DESC members.
	ipl_error_callback(ipl_error_info{IPL_ERR_PLAT, &ffdc});
}
