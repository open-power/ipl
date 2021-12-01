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
