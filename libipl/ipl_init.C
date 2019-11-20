extern "C" {
#include <stdio.h>
#include <stdbool.h>

#include "mmap_file.h"
#include "libipl.h"
}

#include "libekb.H"

static const char *dtree_path[] = {
	"/etc/pdata/p9.dtb",
	"p9.dtb",
	NULL,
};

static struct mmap_file_context *mfile;

int ipl_init(void)
{
	int i;

	for (i=0; dtree_path[i]; i++) {
		mfile = mmap_file_open(dtree_path[i], true);
		if (mfile) {
			printf("Using %s\n", dtree_path[i]);
			break;
		}
	}

	if (!mfile)
		return -1;

	return libekb_init(mmap_file_ptr(mfile));
}

void ipl_close(void)
{
	mmap_file_close(mfile);
}
