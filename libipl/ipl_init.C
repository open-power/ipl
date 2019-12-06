extern "C" {
#include <stdio.h>
#include <stdbool.h>

#include "mmap_file.h"
#include "libipl.h"
}

#include "config.h"
#include "libekb.H"

int ipl_init(void)
{
	return libekb_init();
}
