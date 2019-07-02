extern "C" {
#include <stdio.h>
#include <stdlib.h>

#include <libpdbg.h>
#include <libipl.h>
};

int main(void)
{
	if (ipl_init())
		exit(1);

	return ipl_run_major(0);
}
