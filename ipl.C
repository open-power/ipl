extern "C" {

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>

#include <libpdbg.h>
}

#include <libipl/libipl.H>

static bool isstring(const char *arg)
{
	size_t i;

	if (!arg || arg[0] == '\0' || !isalpha(arg[0]))
		return false;

	for (i=1; i<strlen(arg); i++) {
		if (!isalnum(arg[i]) && arg[i] != '_')
			return false;
	}

	return true;
}

static bool run_istep(const char *arg, int *rc)
{
	char *ptr1 = NULL, *ptr2 = NULL;
	long int major, minor;

	assert(arg);

	if (isstring(arg)) {
		printf("Running istep procedure %s\n", arg);
		*rc = ipl_run_step(arg);
		return true;
	}

	errno = 0;
	major = strtol(arg, &ptr1, 10);
	if (errno) {
		fprintf(stderr, "Invalid istep number %s\n", arg);
		return false;
	}

	if (!ptr1 || ptr1[0] == '\0') {
		printf("Running major istep %d\n", (int)major);
		*rc = ipl_run_major((int)major);
		return true;
	}

	if (*ptr1 != '.') {
		fprintf(stderr, "Invalid istep number %s\n", arg);
		return false;
	}

	errno = 0;
	minor = strtol(ptr1+1, &ptr2, 10);
	if (errno) {
		fprintf(stderr, "Invalid istep number %s\n", arg);
		return false;
	}

	if (!ptr2 || ptr2[0] == '\0') {
		printf("Running istep %d.%d\n", (int)major, (int)minor);
		*rc = ipl_run_major_minor((int)major, (int)minor);
		return true;
	}

	fprintf(stderr, "Invalid istep number %s\n", arg);
	return false;
}

int main(int argc, const char **argv)
{
	int rc, i;

	if (argc < 2) {
		fprintf(stderr, "Usage: ipl <istep> [<istep>...]\n");
		exit(1);
	}

	if (ipl_init(IPL_HOSTBOOT))
		exit(1);

	for (i=1; i<argc; i++) {
		rc = 0;
		if (!run_istep(argv[i], &rc))
			return -1;
	}

	return 0;
}
