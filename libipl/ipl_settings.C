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
                ipl_log(IPL_ERROR, "IPL type MPIPL can only be set in AUTOBOOOT mode, ignoring\n");
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
