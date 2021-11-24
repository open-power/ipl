#ifndef __LIBIPL_H__
#define __LIBIPL_H__

#include <stdlib.h>
#include <map>
#include <vector>
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

//IPL Error types
enum ipl_error_type {
	IPL_ERR_OK = 0,
	IPL_ERR_HWP,
	IPL_ERR_SBE_CHIPOP,
	IPL_ERR_SBE_BOOT,
	IPL_ERR_PRI_PROC_NON_FUNC,
	IPL_ERR_FSI_TGT_NOT_FOUND,
	IPL_ERR_FSI_REG,
	IPL_ERR_PIB_TGT_NOT_FOUND,
	IPL_ERR_PLAT,
};

typedef std::map<ipl_error_type, const char*> err_msg_map_type;
const err_msg_map_type err_msg_map = {
	{ IPL_ERR_OK, "" },
	{ IPL_ERR_HWP, "HWP execution reported error" },
	{ IPL_ERR_SBE_CHIPOP, "SBE chip-op reported error" },
	{ IPL_ERR_SBE_BOOT, "SBE Boot failure" },
	{ IPL_ERR_PRI_PROC_NON_FUNC, "Primary processor is non functional" },
	{ IPL_ERR_FSI_TGT_NOT_FOUND, "FSI target is not available" },
	{ IPL_ERR_FSI_REG, "FSI register read/write failure" },
	{ IPL_ERR_PIB_TGT_NOT_FOUND, "PIB target is not available" },
	{ IPL_ERR_PLAT, "PLAT execution reported error" },
};

// Error info structure.
struct ipl_error_info {
	enum ipl_error_type type;
	//additional debug/callout data need to be included for cfam failures
	void *private_data;
	//specilized constructors
	ipl_error_info() : type(IPL_ERR_OK), private_data(NULL)  {}
	ipl_error_info(ipl_error_type type) : type(type), private_data(NULL) {}
	ipl_error_info(ipl_error_type type, void* private_data) : type(type),
		private_data(private_data) {}

	~ipl_error_info(){
		// freeup the private_data allocated memory
		if (private_data)
			free(private_data);
	}
};

extern "C" {
#include <stdarg.h>

typedef void (*ipl_log_func_t)(void *private_data, const char *fmt, va_list ap);
typedef void (*ipl_error_callback_func_t)(const ipl_error_info& error);

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
 * @Brief Process PLAT error to add callout/deconfig and callback
 * details
 * @param ffdcs_data list of internal ffdc values
 * @param clk_pos    position of failed clock
 */
void ipl_plat_clock_error_handler(
	const std::vector<std::pair<std::string, std::string>>& ffdcs_data,
	uint8_t clk_pos);

/*
 * @Brief This function will call pre_poweroff hardware procedure
 * during poweroff of host, on all the available procs.
 *
 * return 0 on success and non zero in case of failure
 */
int ipl_pre_poweroff(void);
}

#endif /* __LIBIPL_H__ */
