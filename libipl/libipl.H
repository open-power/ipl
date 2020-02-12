#ifndef __LIBIPL_H__
#define __LIBIPL_H__

#define MAX_ISTEP	21

enum ipl_mode {
	IPL_DEFAULT,
	IPL_INTERACTIVE,
};

extern "C" {

void ipl_set_mode(enum ipl_mode mode);
int ipl_init(void);
int ipl_run_major_minor(int major, int minor);
int ipl_run_major(int major);
int ipl_run_step(const char *name);
void ipl_list(int major);

void ipl_log(const char *fmt, ...);

}

#endif /* __LIBIPL_H__ */
