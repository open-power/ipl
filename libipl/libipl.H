#ifndef __LIBIPL_H__
#define __LIBIPL_H__

#define MAX_ISTEP	21

enum ipl_mode {
	IPL_DEFAULT,
	IPL_INTERACTIVE,
};

#define IPL_ERROR   0
#define IPL_INFO    1
#define IPL_DEBUG   2

extern "C" {
#include <stdarg.h>

typedef void (*ipl_log_func_t)(void *private_data, const char *fmt, va_list ap);

void ipl_set_mode(enum ipl_mode mode);
int ipl_init(void);
int ipl_run_major_minor(int major, int minor);
int ipl_run_major(int major);
int ipl_run_step(const char *name);
void ipl_list(int major);

void ipl_set_logfunc(ipl_log_func_t fn, void *private_data);
void ipl_set_loglevel(int loglevel);
void ipl_log(int loglevel, const char *fmt, ...);

}

#endif /* __LIBIPL_H__ */