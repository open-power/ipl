#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "libipl.h"
#include "libipl_internal.h"

static struct ipl_step_data ipl_steps[MAX_ISTEP+1];

void ipl_register(int major, struct ipl_step *steps, void (*pre_func)(void))
{
	assert(major >= 0 && major <= MAX_ISTEP);

	ipl_steps[major].steps = steps;
	ipl_steps[major].pre_func = pre_func;
}

int ipl_run_major_minor(int major, int minor)
{
	struct ipl_step_data *idata;
	int i, rc;

	if (major < 0 || major > MAX_ISTEP || minor < 0)
		return EINVAL;

	idata = &ipl_steps[major];
	assert(idata->steps);

	idata->pre_func();

	for (i=0; idata->steps[i].major != -1; i++) {
		if (idata->steps[i].minor == minor) {
			rc = idata->steps[i].func();
			if (rc == -1)
				return ENOSYS;
			else
				return rc;
		}
	}

	return EINVAL;
}

int ipl_run_major(int major)
{
	struct ipl_step_data *idata;
	int i, rc;

	if (major < 0 || major > 21)
		return EINVAL;

	idata = &ipl_steps[major];
	assert(idata->steps);

	idata->pre_func();

	for (i=0; idata->steps[i].major != -1; i++) {
		rc = idata->steps[i].func();
		if (rc != 0 && rc != -1)
			break;
	}

	return rc;
}

static struct ipl_step *ipl_find_step(struct ipl_step *steps, const char *name)
{
	int i;

	for (i=0; steps[i].major != -1; i++) {
		if (!strcmp(steps[i].name, name))
			return &steps[i];
	}

	return NULL;
}

int ipl_run_step(const char *name)
{
	struct ipl_step_data *idata;
	struct ipl_step *step = NULL;
	int i, rc;

	for (i=0; i<=MAX_ISTEP; i++) {
		idata = &ipl_steps[i];
		step = ipl_find_step(idata->steps, name);
		if (step)
			break;
	}

	if (!step)
		return EINVAL;

	idata->pre_func();

	rc = step->func();
	if (rc == -1)
		return ENOSYS;

	return rc;
}

void ipl_list(int major)
{
	struct ipl_step_data *idata;
	int i;

	if (major < 0 || major > MAX_ISTEP) {
		fprintf(stderr, "Invalid istep number %d\n", major);
		return;
	}

	idata = &ipl_steps[major];
	assert(idata->steps);

	for (i=0; idata->steps[i].major != -1; i++)
		printf("\t%d.%d\t%s\n", major, idata->steps[i].minor, idata->steps[i].name);
}
