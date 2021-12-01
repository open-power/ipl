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

void ipl_set_type(enum ipl_type type)
{
	switch (type) {
	case IPL_TYPE_NORMAL:
		ipl_log(IPL_INFO, "IPL type NORMAL\n");
		break;

	case IPL_TYPE_MPIPL:
		if (ipl_mode() != IPL_AUTOBOOT) {
			ipl_log(IPL_ERROR,
				"MPIPL can only be set in AUTOBOOOT mode, ignoring\n");
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
