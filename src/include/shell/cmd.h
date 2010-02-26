/*
 * 	miaofng@2009 initial version
 */
#ifndef __CMD_H_
#define __CMD_H_

typedef struct {
	char *name;
	int (*func)(int argc, char *argv[]);
	char *help;
} cmd_t;

typedef struct {
	cmd_t *cmd;
	int flag;
	void *next;
} cmd_list_t;

#define CMD_FLAG_REPEAT 1

/*commands declaration*/
extern cmd_t cmd_help;
extern cmd_t cmd_pause;
extern cmd_t cmd_kill;
extern cmd_t cmd_speed;
extern cmd_t cmd_rpm;
extern cmd_t cmd_motor;
extern cmd_t cmd_pid;
extern cmd_t cmd_debug;
extern cmd_t cmd_dalgo;
extern cmd_t cmd_svpwm;

void cmd_Init(void);
void cmd_Update(void);
void cmd_Add(cmd_t *cmd);
void cmd_Exec(int argc, char *argv[]);

#endif /*__SHELL_H_*/
