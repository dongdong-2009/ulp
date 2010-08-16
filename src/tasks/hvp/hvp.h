/*
 * 	miaofng@2010 initial version
 */
#ifndef __HVP_H_
#define __HVP_H_

#include <stdarg.h>

enum {
	HVP_CMD_PROGRAM,
};

typedef struct {
	char cmd;
	void *para1;
	void *para2;
} hvp_msg_t;

int hvp_prog(char *model, char *sub);
void dlg_init(void);
int dlg_set_prog_step(const char *fmt, ...);
void dlg_prog_finish(int status);
#define dlg_set_prog_err(...) do { \
	dlg_set_prog_step(__VA_ARGS__); \
	dlg_prog_finish(-1); \
} while (0)

#endif
