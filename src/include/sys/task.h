/*
 * 	miaofng@2010 initial version
 */
#ifndef __TASK_H_
#define __TASK_H_

#pragma section=".sys.task" 4
#define DECLARE_TASK(init, update) \
	void (*##init##_entry)(void)@".sys.task" = &##init; \
	void (*##update##_entry)(void)@".sys.task" = &##update;

#pragma section=".sys.lib" 4
#define DECLARE_LIB(init, update) \
	void (*##init##_entry)(void)@".sys.lib" = &##init; \
	void (*##update##_entry)(void)@".sys.lib" = &##update;

void task_Init(void);
void task_Update(void);
void task_Isr(void);

//obsoleted
void task_SetForeground(void (*task)(void));

//private
void task_init(void);
void task_update(void);
void lib_init(void);
void lib_update(void);

#endif /*__TASK_H_*/
