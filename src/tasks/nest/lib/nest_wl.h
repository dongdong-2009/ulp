/*
 * 	miaofng@2011 initial version
 */


#ifndef __NEST_WL_H_
#define __NEST_WL_H_

#define NEST_WL_ADDR 0x12345678
#define NEST_WL_FREQ 2430
#define NEST_WL_MS (1000*10)
#define NEST_WL_TXMS (1000)

int nest_wl_init(void);
int nest_wl_update(void);

#endif
