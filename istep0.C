extern "C" {
#include <stdio.h>
#include <stdlib.h>

#include <libpdbg.h>
}

#include <libipl/libipl.H>

int main(void)
{
	if (ipl_init(IPL_DEFAULT))
		exit(1);

	return ipl_run_major(0);
}
