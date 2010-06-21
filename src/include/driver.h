/*
 * 	miaofng@2010 initial version
 */
#ifndef __DRIVER_H_
#define __DRIVER_H_

#pragma section=".driver" 4
#define driver_init(init) \
	void (*##init##_entry)(void)@".driver" = &##init

void drv_Init(void);

#endif /*__DRIVER_H_*/
