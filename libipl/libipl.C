extern "C" {

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <stdarg.h>

#include <libpdbg.h>
#include <config.h>
}

#include "libipl.H"
#include "libipl_internal.H"

static struct ipl_step_data ipl_steps[MAX_ISTEP+1];

static bool g_ipl_test_mode = false;

void ipl_pre(void)
{
	struct pdbg_target *proc;

	pdbg_for_each_class_target("proc", proc) {
		struct pdbg_target *fsi, *pib;
		char path[16];

		sprintf(path, "/proc%d/fsi", pdbg_target_index(proc));
		fsi = pdbg_target_from_path(NULL, path);
		assert(fsi);

		sprintf(path, "/proc%d/pib", pdbg_target_index(proc));
		pib = pdbg_target_from_path(NULL, path);
		assert(pib);

		if (pdbg_target_probe(fsi) != PDBG_TARGET_ENABLED ||
		    pdbg_target_probe(pib) != PDBG_TARGET_ENABLED) {
			pdbg_target_status_set(proc, PDBG_TARGET_DISABLED);
		}
	}
}

void ipl_register(int major, struct ipl_step *steps, void (*pre_func)(void))
{
	assert(major >= 0 && major <= MAX_ISTEP);

	ipl_steps[major].steps = steps;
	ipl_steps[major].pre_func = pre_func;
}

#ifdef IPL_P10
static int ipl_init_p10(void)
{
	struct pdbg_target *root;
	uint8_t istep_mode = 1;

	if (ipl_mode() == IPL_AUTOBOOT)
		istep_mode = 0;

	root = pdbg_target_root();
	if (!pdbg_target_set_attribute(root, "ATTR_ISTEP_MODE", 1, 1, &istep_mode)) {
		ipl_log(IPL_ERROR, "Attribute [ATTR_ISTEP_MODE] update failed\n");
		return 1;
	}

	return 0;
}
#endif /* IPL_P10 */

int ipl_init(enum ipl_mode mode)
{
	char *tmp;

	ipl_set_mode(mode);

	if (!pdbg_target_root()) {
		ipl_log(IPL_ERROR, "libpdbg not initialized\n");
		return -1;
	}

	tmp = getenv("IPL_TEST_MODE");
	if (tmp) {
		g_ipl_test_mode = true;
		return 0;
	}

#ifdef IPL_P10
	if (ipl_init_p10())
		return -1;
#endif /* IPL_P10 */

	return 0;
}

static void ipl_execute_pre(struct ipl_step_data *idata)
{
	if (g_ipl_test_mode)
		fprintf(stderr, "  Executing pre\n");
	else
		idata->pre_func();
}

static int ipl_execute_istep(struct ipl_step *step)
{
	int rc = 0;

	if (g_ipl_test_mode)
		fprintf(stderr, "  Executing %s\n", step->name);
	else
		rc = step->func();

	return rc;
}

static struct ipl_step *ipl_find_minor(struct ipl_step *steps, int minor)
{
	int i;

	for (i=0; steps[i].minor != -1; i++) {
		if (steps[i].minor == minor)
			return &steps[i];
	}

	return NULL;
}

int ipl_run_major_minor(int major, int minor)
{
	struct ipl_step_data *idata;
	struct ipl_step *step = NULL;
	int rc = 0;

	if (major < 0 || major > MAX_ISTEP || minor < 0)
		return EINVAL;

	if (ipl_mode() == IPL_AUTOBOOT && major != 0)
		return EINVAL;

	idata = &ipl_steps[major];
	assert(idata->steps);

	step = ipl_find_minor(idata->steps, minor);
	if (!step)
		return EINVAL;

	ipl_execute_pre(idata);

	rc = ipl_execute_istep(step);
	if (rc == -1)
		rc = 0;

	return rc;
}

int ipl_run_major(int major)
{
	struct ipl_step_data *idata;
	int i;
	int rc = 0;

	if (major < 0 || major > MAX_ISTEP)
		return EINVAL;

	if (ipl_mode() == IPL_AUTOBOOT && major != 0)
		return EINVAL;

	idata = &ipl_steps[major];
	assert(idata->steps);

	ipl_execute_pre(idata);

	for (i=0; idata->steps[i].major != -1; i++) {
		rc = ipl_execute_istep(&idata->steps[i]);
		if (rc != 0 && rc != -1)
			break;
	}

	if (rc == -1)
		rc = 0;

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
	int i;
	int rc = 0;

	for (i=0; i<=MAX_ISTEP; i++) {
		idata = &ipl_steps[i];
		step = ipl_find_step(idata->steps, name);
		if (step)
			break;
	}

	if (!step)
		return EINVAL;

	if (ipl_mode() == IPL_AUTOBOOT && step->major != 0)
		return EINVAL;

	ipl_execute_pre(idata);

	rc = ipl_execute_istep(step);
	if (rc == -1)
		rc = 0;

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

void ipl_log(int loglevel, const char *fmt, ...)
{
	va_list ap;

	if (!ipl_log_func())
		return;

	if (loglevel > ipl_log_level())
		return;

	va_start(ap, fmt);
	(*ipl_log_func())(ipl_log_func_priv_data(), fmt, ap);
	va_end(ap);
}

void ipl_error_callback(const ipl_error_info& error)
{
	if (!ipl_error_callback_fn())
		return;
	(*ipl_error_callback_fn())(error);
}
