extern "C" {
#include <stdio.h>

#include <libpdbg.h>

#include "libipl.h"
#include "libipl_internal.h"
}

#include "common.H"

int ipl_istep_via_sbe(int major, int minor)
{
	struct pdbg_target *pib;
	int rc = 0;

	pdbg_for_each_class_target("pib", pib) {
		if (ipl_mode() == IPL_DEFAULT && pdbg_target_index(pib) != 0)
			continue;

		rc |= sbe_istep(pib, major, minor);
		if (rc)
			ipl_log("Istep %d.%d failed on chip %d, rc=%d\n",
				major, minor, pdbg_target_index(pib), rc);
	}

	return rc;
}
