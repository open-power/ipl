extern "C" {
#include <stdio.h>
#include <stdbool.h>

#include <libpdbg.h>
#include <atdb/atdb_blob.h>

#include "libipl.h"
}

#include "plat_trace.H"
#include "plat_utils.H"

int ipl_init(void)
{
	struct atdb_blob_info *binfo;

	binfo = atdb_blob_open("/etc/pdata/attributes.atdb", true);
	if (!binfo)
		binfo = atdb_blob_open("attributes.atdb", true);

	if (!binfo)
		return -1;

	plat_set_atdb_context(atdb_blob_atdb(binfo));

	return 0;
}
