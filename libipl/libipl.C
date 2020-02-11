extern "C" {

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <stdarg.h>
}

#include "libekb.H"
#include "libipl.H"
#include "libipl_internal.H"

static struct ipl_step_data ipl_steps[MAX_ISTEP+1];
static enum ipl_mode g_ipl_mode = IPL_DEFAULT;

static ipl_log_func_t g_ipl_log_fn;
static void * g_ipl_log_priv;
static int g_ipl_log_level = IPL_DEBUG;

struct {
	int ipl_loglevel;
	int libekb_loglevel;
} ipl_libekb_log_map[] = {
	{ IPL_ERROR,  LIBEKB_LOG_ERR },
	{ IPL_INFO,   LIBEKB_LOG_INF },
	{ IPL_DEBUG,  LIBEKB_LOG_DBG },
	{ -1,  -1 },
};

static int ipl_to_libekb_loglevel(int ipl_loglevel)
{
	int i;

	for (i=0; ipl_libekb_log_map[i].ipl_loglevel != -1; i++) {
		if (ipl_libekb_log_map[i].ipl_loglevel == ipl_loglevel)
			return ipl_libekb_log_map[i].libekb_loglevel;
	}

	return LIBEKB_LOG_ERR;
}

static void ipl_log_default(void *priv, const char *fmt, va_list ap)
{
	vfprintf(stdout, fmt, ap);
}

static void ipl_libekb_log(void *priv, const char *fmt, va_list ap)
{
	if (!g_ipl_log_fn)
		return;

	g_ipl_log_fn(g_ipl_log_priv, fmt, ap);
}

void ipl_register(int major, struct ipl_step *steps, void (*pre_func)(void))
{
	assert(major >= 0 && major <= MAX_ISTEP);

	ipl_steps[major].steps = steps;
	ipl_steps[major].pre_func = pre_func;
}

int ipl_init(void)
{
	int ret;

	if (!g_ipl_log_fn)
		ipl_set_logfunc(ipl_log_default, NULL);

	libekb_set_logfunc(ipl_libekb_log, NULL);

	ipl_set_loglevel(g_ipl_log_level);

	ret = libekb_init();
	if (ret != 0)
		return ret;

	return 0;
}

int ipl_run_major_minor(int major, int minor)
{
	struct ipl_step_data *idata;
	int i;
	int rc = 0;

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
	int i;
	int rc = 0;

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

void ipl_set_mode(enum ipl_mode mode)
{
	g_ipl_mode = mode;
}

enum ipl_mode ipl_mode(void)
{
	return g_ipl_mode;
}

void ipl_set_logfunc(ipl_log_func_t fn, void *private_data)
{
	g_ipl_log_fn = fn;
	g_ipl_log_priv = private_data;
}

void ipl_set_loglevel(int loglevel)
{
	int libekb_loglevel;

	if (loglevel < IPL_ERROR)
		loglevel = IPL_ERROR;

	if (loglevel > IPL_DEBUG)
		loglevel = IPL_DEBUG;

	g_ipl_log_level = loglevel;

	libekb_loglevel = ipl_to_libekb_loglevel(loglevel);
	libekb_set_loglevel(libekb_loglevel);
}

void ipl_log(int loglevel, const char *fmt, ...)
{
	va_list ap;

	if (!g_ipl_log_fn)
		return;

	if (loglevel > g_ipl_log_level)
		return;

	va_start(ap, fmt);
	g_ipl_log_fn(g_ipl_log_priv, fmt, ap);
	va_end(ap);
}