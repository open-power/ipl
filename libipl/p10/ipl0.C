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
#include<iostream>
#include"iplfacade.H"

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

inline struct pdbg_target* get_pib_target(struct pdbg_target* target)
{
    struct pdbg_target* pib = NULL;

    /* We assume each processor chip contains one and only one pib */
    pdbg_for_each_target("pib", target, pib)
    {
        break;
    }

    if (!pib)
    {
        std::cout << "No pib target associated with target %s"
                  << pdbg_target_path(target) << std::endl;
        return NULL;
    }

    return pib;
}

static void ipl_pre0(void)
{
	ipl_pre();
}

static int ipl_pre_cfam_reset(void)
{
	return -1;
}

static int ipl_cfam_reset(void)
{
	return -1;
}

static int ipl_poweron(void)
{
	return -1;
}

static int ipl_startipl(void)
{
	return -1;
}

static int ipl_disable_attns(void)
{
	return -1;
}

static int ipl_updatehwmodel(void)
{
	return -1;
}

static int ipl_smp_topology(void)
{
	return -1;
}

static int ipl_boot_optimization_check(void)
{
	return -1;
}

static int ipl_set_ref_clock(void)
{
	 struct pdbg_target* hubchip;
	 int ret=-1;
    pdbg_for_each_target("hubchip", NULL, hubchip)
    {
        std::cout << "hubchip found " << std::endl;
        pdbg_target_probe(hubchip);
        if (pdbg_target_status(hubchip) != PDBG_TARGET_ENABLED)
        {
            std::cout << "hubchip chip not enabled " << std::endl;
            return -1;
        }
        pdbg_target* pib = get_pib_target(hubchip);
        pdbg_target_probe(pib);
        if (pdbg_target_status(pib) != PDBG_TARGET_ENABLED)
        {
            std::cout << "pib target for hub chip not enabled " << std::endl;
            return -1;
        }
        std::cout << "invoking ps_set_ref_clock HWP" << std::endl;
        ret = ipl::ps_setup_ref_clock(hubchip);
        if (ret == 0)
        {
            std::cout << "succes in call to HWP ps_set_ref_clock "
                      << std::endl;
        }
		break;
	}
	return ret;
}

static int ipl_clock_test(void)
{
	return -1;
}

static int ipl_hb_config_update(void)
{
	return -1;
}

static int ipl_ph_sppe_config_update(void)
{
	struct pdbg_target* hubchip;
	int ret=-1;
    pdbg_for_each_target("hubchip", NULL, hubchip)
    {
        std::cout << "hubchip found " << std::endl;
        pdbg_target_probe(hubchip);
        if (pdbg_target_status(hubchip) != PDBG_TARGET_ENABLED)
        {
            std::cout << "hubchip chip not enabled " << std::endl;
            return -1;
        }
        pdbg_target* pib = get_pib_target(hubchip);
        pdbg_target_probe(pib);
        if (pdbg_target_status(pib) != PDBG_TARGET_ENABLED)
        {
            std::cout << "pib target for hub chip not enabled " << std::endl;
            return -1;
        }
        std::cout << "invoking ps_sppe_config_update HWP" << std::endl;
        ret = ipl::ps_sppe_config_update(hubchip);
        if (ret == 0)
        {
            std::cout << "succes in call to HWP ps_sppe_config_update "
                      << std::endl;
        }
		break;
    }
	return ret;
}

static int ipl_cbs_start(void)
{
	struct pdbg_target* hubchip;
	int ret=-1;
    pdbg_for_each_target("hubchip", NULL, hubchip)
    {
        std::cout << "hubchip found " << std::endl;
        pdbg_target_probe(hubchip);
        if (pdbg_target_status(hubchip) != PDBG_TARGET_ENABLED)
        {
            std::cout << "hubchip chip not enabled " << std::endl;
            return -1;
        }
        pdbg_target* pib = get_pib_target(hubchip);
        pdbg_target_probe(pib);
        if (pdbg_target_status(pib) != PDBG_TARGET_ENABLED)
        {
            std::cout << "pib target for hub chip not enabled " << std::endl;
            return -1;
        }
        std::cout << "invoking ps_cbs_start HWP" << std::endl;
        ret = ipl::ps_cbs_start(hubchip);
        if (ret == 0)
        {
            std::cout << "succes in call to HWP ps_cbs_start "
                      << std::endl;
        }
        break;
    }
	return ret;
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
	{IPL_DEF(pre_cfam_reset), 0, 1, true, true},
    {IPL_DEF(cfam_reset), 0, 2, true, true},
    {IPL_DEF(poweron), 0, 3, true, true},
    {IPL_DEF(startipl), 0, 4, true, true},
    {IPL_DEF(disable_attns), 0, 5, true, true},
    {IPL_DEF(updatehwmodel), 0, 6, true, true},
    {IPL_DEF(smp_topology), 0, 7, true, true},
    {IPL_DEF(boot_optimization_check), 0, 8, true, true},
    {IPL_DEF(set_ref_clock), 0, 9, true, true},
    {IPL_DEF(clock_test), 0, 10, true, true},
    {IPL_DEF(hb_config_update), 0, 11, true, true},
    {IPL_DEF(ph_sppe_config_update), 0, 12, true, true},
    {IPL_DEF(cbs_start), 0, 13, true, true},
    {IPL_DEF(startPRD), 0, 14, true, true},
    {IPL_DEF(proc_attn_listen), 0, 15, true, true},
    {NULL, NULL, -1, -1, false, false},
};

__attribute__((constructor)) static void ipl_register_ipl0(void)
{
	ipl_register(0, ipl0, ipl_pre0);
}