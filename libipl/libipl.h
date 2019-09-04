#ifndef __LIBIPL_H__
#define __LIBIPL_H__

#define MAX_ISTEP	21

int ipl_init(void);
int ipl_run_major_minor(int major, int minor);
int ipl_run_major(int major);
int ipl_run_step(const char *name);
void ipl_list(int major);

#endif /* __LIBIPL_H__ */
