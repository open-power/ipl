#ifndef __LIBIPL_H__
#define __LIBIPL_H__

#include <stdlib.h>
#include <string>

#define MAX_ISTEP	21

enum ipl_mode {
	IPL_AUTOBOOT = 0,
	IPL_HOSTBOOT,
	IPL_CRONUS,
};

enum ipl_type {
	IPL_TYPE_NORMAL = 0,
	IPL_TYPE_MPIPL,
};

#define IPL_ERROR   0
#define IPL_INFO    1
#define IPL_DEBUG   2

extern "C" {
#include <stdarg.h>

//IPL Error types
enum ipl_error_type {
	IPL_NO_ERROR = 0,
	IPL_SBE_CHIPOP_FAIL = 1,
};

// Error info structure.
struct ipl_error_info {
	enum ipl_error_type type;
	void *private_data;  //additional data
	const std::string msg; //Error messaage
	~ipl_error_info(){
		// freeup the private_data allocated memory
		if (private_data){
			free(private_data);
		}
	}
};


typedef void (*ipl_log_func_t)(void *private_data, const char *fmt, va_list ap);
typedef void (*ipl_error_callback_func_t)(bool status, ipl_error_info error);

int ipl_init(enum ipl_mode mode);
int ipl_run_major_minor(int major, int minor);
int ipl_run_major(int major);
int ipl_run_step(const char *name);
void ipl_list(int major);

void ipl_set_type(enum ipl_type type);
enum ipl_type ipl_type(void);

void ipl_set_logfunc(ipl_log_func_t fn, void *private_data);
void ipl_set_loglevel(int loglevel);
void ipl_log(int loglevel, const char *fmt, ...);

void ipl_set_error_callback_func(ipl_error_callback_func_t fn);

/*
 * @Brief This function will call pre_poweroff hardware procedure
 * during poweroff of host, on all the available procs.
 *
 * return 0 on success and non zero in case of failure
 */
int ipl_pre_poweroff(void);
}

#endif /* __LIBIPL_H__ */
