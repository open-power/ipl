extern "C" {

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <libipl.h>
#include <libpdbg.h>
}

int main(int argc, const char **argv)
{
	int i;

	if (argc != 2) {
		fprintf(stderr, "Usage: ipl <istep>\n");
		exit(1);
	}

	i = atoi(argv[1]);

	printf("booting\n");

	if (ipl_init())
		exit(1);

	ipl_run_major(i);

	return 0;
}
