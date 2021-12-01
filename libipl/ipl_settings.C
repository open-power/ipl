#include <cstdio>
#include <cassert>

#include "libipl.H"

static void ipl_log_default(void *priv, const char *fmt, va_list ap);

// Structure for IPL settings
struct ipl_settings {

    enum ipl_mode mode;
    enum ipl_type type;
    int log_level;
    ipl_log_func_t log_func;
    void* log_func_priv_data;
    ipl_error_callback_func_t error_callback_fn;

    // Default IPL settings
    ipl_settings() :
        mode(IPL_HOSTBOOT),
        type(IPL_TYPE_NORMAL),
        log_level(IPL_ERROR),
        log_func(ipl_log_default),
        log_func_priv_data(NULL),
        error_callback_fn(NULL)
    {}

    // Below constructors are not allowed
    ipl_settings(const ipl_settings& ipl_settings) = delete;
    ipl_settings& operator=(const ipl_settings& ipl_settings) = delete;
    ipl_settings(ipl_settings&&) = delete;
    ipl_settings& operator=(ipl_settings&&) = delete;

    ~ipl_settings() = default;
};

static ipl_settings g_ipl_settings;

static void ipl_log_default(void *priv, const char *fmt, va_list ap)
{
    vfprintf(stdout, fmt, ap);
}
