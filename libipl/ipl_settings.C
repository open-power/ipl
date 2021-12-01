#include <stdio.h>

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
