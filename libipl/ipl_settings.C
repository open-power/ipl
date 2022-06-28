#include <stdio.h>
#include <assert.h>

#include "libipl.H"

struct ipl_settings {
	enum ipl_mode mode;
	enum ipl_type type;

	int log_level;
	ipl_log_func_t log_func;
	void *log_func_priv_data;

	ipl_error_callback_func_t error_callback_fn;

	bool apply_guard;
};

static void ipl_log_default(void *priv, const char *fmt, va_list ap)
{
	vfprintf(stdout, fmt, ap);
}

static ipl_settings g_ipl_settings = {
    .mode = IPL_HOSTBOOT,
    .type = IPL_TYPE_NORMAL,
    .log_level = IPL_ERROR,
    .log_func = ipl_log_default,
    .apply_guard = true,
};

void ipl_set_mode(enum ipl_mode mode)
{
	switch (mode) {
	case IPL_AUTOBOOT:
		ipl_log(IPL_INFO, "IPL mode set to AUTOBOOT\n");
		break;

	case IPL_HOSTBOOT:
		ipl_log(IPL_INFO, "IPL mode set to HOSTBOOT\n");
		break;

	case IPL_CRONUS:
		ipl_log(IPL_INFO, "IPL mode set to CRONUS\n");
		break;

	default:
		ipl_log(IPL_ERROR, "Invalid IPL mode\n");
		assert(0);
	}

	g_ipl_settings.mode = mode;
}

enum ipl_mode ipl_mode(void)
{
	return g_ipl_settings.mode;
}

void ipl_set_type(enum ipl_type type)
{
	switch (type) {
	case IPL_TYPE_NORMAL:
		ipl_log(IPL_INFO, "IPL type NORMAL\n");
		break;

	case IPL_TYPE_MPIPL:
		if (ipl_mode() != IPL_AUTOBOOT) {
			ipl_log(IPL_ERROR, "MPIPL can only be set in AUTOBOOOT "
					   "mode, ignoring\n");
			return;
		}

		ipl_log(IPL_INFO, "IPL type MPIPL\n");
		break;

	default:
		ipl_log(IPL_ERROR, "Invalid IPL type\n");
		assert(0);
	}

	g_ipl_settings.type = type;
}

enum ipl_type ipl_type(void)
{
	return g_ipl_settings.type;
}

void ipl_set_logfunc(ipl_log_func_t fn, void *private_data)
{
	g_ipl_settings.log_func = fn;
	g_ipl_settings.log_func_priv_data = private_data;
}

ipl_log_func_t ipl_log_func(void)
{
	return g_ipl_settings.log_func;
}

void *ipl_log_func_priv_data(void)
{
	return g_ipl_settings.log_func_priv_data;
}

void ipl_set_loglevel(int loglevel)
{
	if (loglevel < IPL_ERROR)
		loglevel = IPL_ERROR;

	if (loglevel > IPL_DEBUG)
		loglevel = IPL_DEBUG;

	g_ipl_settings.log_level = loglevel;
}

int ipl_log_level(void)
{
	return g_ipl_settings.log_level;
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

void ipl_set_error_callback_func(ipl_error_callback_func_t fn)
{
	g_ipl_settings.error_callback_fn = fn;
}

ipl_error_callback_func_t ipl_error_callback_fn(void)
{
	return g_ipl_settings.error_callback_fn;
}

void ipl_error_callback(const ipl_error_info &error)
{
	if (!g_ipl_settings.error_callback_fn)
		return;

	g_ipl_settings.error_callback_fn(error);
}

void ipl_disable_guard(void)
{
	g_ipl_settings.apply_guard = false;
}

bool ipl_guard(void)
{
	return g_ipl_settings.apply_guard;
}
