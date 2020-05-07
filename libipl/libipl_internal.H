#ifndef __IPL_H__
#define __IPL_H__

extern "C" {
#include <stdbool.h>
}

#define IPL_DEF(a)	#a, ipl_ ## a

struct ipl_step {
	const char *name;
	int (*func)(void);
	int major;
	int minor;
	bool interactive;
	bool hostboot;
};

struct ipl_step_data {
	struct ipl_step *steps;
	void (*pre_func)(void);
};

void ipl_pre(void);
void ipl_register(int major, struct ipl_step *steps, void (*pre_func)(void));
void ipl_set_mode(enum ipl_mode mode);
enum ipl_mode ipl_mode(void);

void ipl_error_callback(bool status);
#endif /* __IPL_H__ */
