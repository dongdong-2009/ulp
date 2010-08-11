/*
 * 	miaofng@2010 initial version
 */
#ifndef __HVP_H_
#define __HVP_H_

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
void dlg_set_prog_addr(int addr);
#define dlg_set_prog_err dlg_set_prog_step
#endif
