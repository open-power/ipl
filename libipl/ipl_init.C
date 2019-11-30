extern "C" {
#include <stdio.h>
#include <stdbool.h>

#include "mmap_file.h"
#include "libipl.h"
}

#include "config.h"
#include "libekb.H"

static struct mmap_file_context *mfile;

int ipl_init(void)
{
    mfile = mmap_file_open(DTB_FILE, true);

	if (!mfile)
		return -1;

	return libekb_init(mmap_file_ptr(mfile));
}

void ipl_close(void)
{
	mmap_file_close(mfile);
}
