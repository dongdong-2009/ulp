/*
*  miaofng@2015-8-25   initial version
*
*/

#ifndef __UMX_BSP_H__
#define __UMX_BSP_H__

#include "config.h"

void oe_set(int high);
void le_set(int high);
void miso_sel(int board);

void trig_set(int high);
int trig_get(void); //vmcomp pulsed

void board_init(void);
void board_reset(void);

/*mxc i/f api*/
void mxc_init(void);
int mxc_switch(int line, int bus, int opcode);
void mxc_image_store(void); /*store image changes*/
void mxc_image_restore(void); /*ignore image changes*/
void mxc_execute(void); //relay acts here!!

#endif
