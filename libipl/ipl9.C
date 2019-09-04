extern "C" {
#include <stdio.h>
#include <assert.h>

#include <libpdbg.h>

#include "libipl.h"
#include "libipl_internal.h"
}

#include <ekb/chips/p9/procedures/hwp/io/p9_io_xbus_dccal.H>
#include <ekb/chips/p9/procedures/hwp/io/p9_io_xbus_linktrain.H>
#include <ekb/chips/p9/procedures/hwp/nest/p9_smp_link_layer.H>
#include <ekb/chips/p9/procedures/hwp/nest/p9_fab_iovalid.H>
#include <ekb/chips/p9/procedures/hwp/nest/p9_fbc_eff_config_aggregate.H>

static void ipl_pre9(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib)
		pdbg_target_probe(pib);
}

static int ipl_fabric_erepair(void)
{
	return -1;
}

static int ipl_fabric_io_dccal(void)
{
	struct pdbg_target *pib, *master_xbus = NULL, *slave_xbus = NULL;

	pdbg_for_each_class_target("pib", pib) {
		struct pdbg_target *target;

		pdbg_for_each_target("xbus", pib, target) {
			uint8_t master_mode = 0;

			/* Find a master/slave bus */
			FAPI_ATTR_GET(fapi2::ATTR_IO_XBUS_MASTER_MODE, target, master_mode);
			if (master_mode)
				master_xbus = target;
			else
				slave_xbus = target;
		}
	}

	assert(slave_xbus && master_xbus);

	/* We only run zcal once per PHY */
	p9_io_xbus_dccal(XbusDccalMode::TxZcalRunBus, master_xbus, 0);
	p9_io_xbus_dccal(XbusDccalMode::TxZcalSetGrp, master_xbus, 0);
	p9_io_xbus_dccal(XbusDccalMode::TxZcalSetGrp, master_xbus, 1);

	p9_io_xbus_dccal(XbusDccalMode::TxZcalRunBus, slave_xbus, 0);
	p9_io_xbus_dccal(XbusDccalMode::TxZcalSetGrp, slave_xbus, 0);
	p9_io_xbus_dccal(XbusDccalMode::TxZcalSetGrp, slave_xbus, 1);

	p9_io_xbus_dccal(XbusDccalMode::RxDccalStartGrp, master_xbus, 0);
	p9_io_xbus_dccal(XbusDccalMode::RxDccalStartGrp, slave_xbus, 0);
	p9_io_xbus_dccal(XbusDccalMode::RxDccalStartGrp, master_xbus, 1);
	p9_io_xbus_dccal(XbusDccalMode::RxDccalStartGrp, slave_xbus, 1);

	p9_io_xbus_dccal(XbusDccalMode::RxDccalCheckGrp, master_xbus, 0);
	p9_io_xbus_dccal(XbusDccalMode::RxDccalCheckGrp, slave_xbus, 0);
	p9_io_xbus_dccal(XbusDccalMode::RxDccalCheckGrp, master_xbus, 1);
	p9_io_xbus_dccal(XbusDccalMode::RxDccalCheckGrp, slave_xbus, 1);

	return 0;
}

static int ipl_fabric_pre_trainadv(void)
{
	return -1;
}

static int ipl_fabric_io_run_training(void)
{
	struct pdbg_target *pib, *master_xbus = NULL, *slave_xbus = NULL;

	pdbg_for_each_class_target("pib", pib) {
		struct pdbg_target *target;

		pdbg_for_each_target("xbus", pib, target) {
			uint8_t master_mode = 0;

			/* Find a master/slave bus */
			FAPI_ATTR_GET(fapi2::ATTR_IO_XBUS_MASTER_MODE, target, master_mode);
			if (master_mode)
				master_xbus = target;
			else
				slave_xbus = target;
		}
	}

	assert(slave_xbus && master_xbus);

	p9_io_xbus_linktrain(master_xbus, slave_xbus, 0);
	p9_io_xbus_linktrain(master_xbus, slave_xbus, 1);

	return 0;
}

static int ipl_fabric_post_trainadv(void)
{
	return -1;
}

static int ipl_proc_smp_link_layer(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib)
		p9_smp_link_layer(pib, 1, 0);

	return 0;
}

static int ipl_proc_fab_iovalid(void)
{
	struct pdbg_target *pib;
	std::vector<fapi2::ReturnCode> err;

	pdbg_for_each_class_target("pib", pib)
		p9_fab_iovalid(pib, true, true, false, err);

	return 0;
}

static int ipl_host_fbc_eff_config_aggregate(void)
{
	struct pdbg_target *pib;

	pdbg_for_each_class_target("pib", pib)
		p9_fbc_eff_config_aggregate(pib);

	return 0;
}

static struct ipl_step ipl9[] = {
	{ IPL_DEF(fabric_erepair),                9,  1,  false, true  },
	{ IPL_DEF(fabric_io_dccal),               9,  2,  true,  true  },
	{ IPL_DEF(fabric_pre_trainadv),           9,  3,  true,  true  },
	{ IPL_DEF(fabric_io_run_training),        9,  4,  true,  true  },
	{ IPL_DEF(fabric_post_trainadv),          9,  5,  true,  true  },
	{ IPL_DEF(proc_smp_link_layer),           9,  6,  true,  true  },
	{ IPL_DEF(proc_fab_iovalid),              9,  7,  true,  true  },
	{ IPL_DEF(host_fbc_eff_config_aggregate), 9,  8,  true,  true  },
	{ NULL, NULL, -1, -1, false, false },
};

__attribute__((constructor))
static void ipl_register_ipl9(void)
{
	ipl_register(9, ipl9, ipl_pre9);
}
