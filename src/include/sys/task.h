/*
 * 	miaofng@2010 initial version
 */
#ifndef __TASK_H_
#define __TASK_H_

#pragma section=".sys.task" 4
#define DECLARE_TASK(init, update) \
	void (*##init##_entry)(void)@".sys.task" = &##init; \
	void (*##update##_entry)(void)@".sys.task" = &##update;

void task_Init(void);
void task_Update(void);

#endif /*__TASK_H_*/
