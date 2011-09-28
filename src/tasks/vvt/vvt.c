/*
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "sys/task.h"
#include "vvt.h"
#include "misfire.h"
#include "driver/vvt_pulse.h"
#include <stdio.h>
#include <stdlib.h>
#include "sys/task.h"
#include "debug.h"
#include "time.h"

//private
short vvt_gear_advance;
int vvt_knock_pos;
short vvt_knock_width;
short vvt_knock_strength; //unit: mV
int vvt_knock_pattern; //...D C B A
short vvt_counter; //0-719

//val ? [min, max]
#define IS_IN_RANGE(val, min, max) \
	(((val) >= (min)) && ((val) <= (max)))

void vvt_Init(void)
{
	vvt_pulse_Init();
	misfire_Init();
	misfire_SetSpeed(500);
	vvt_Start();
	vvt_counter = 0;
	vvt_gear_advance = 0;
	vvt_knock_pos = 0;
	vvt_knock_width = 50;
	vvt_knock_pattern = 0;
}

void vvt_Update(void)
{
	vvt_pulse_Update();

	//gear related varible calculation
	vvt_gear_advance = md204l_read_buf[0] ;
	vvt_knock_pos = md204l_read_buf[1] ;
	vvt_knock_width = md204l_read_buf[2] ;
	vvt_knock_pattern = knock_GetPattern();
}

DECLARE_TASK(vvt_Init, vvt_Update)

void vvt_isr(void)
{
	int mv;
	int x, y, z, tmp;
	int ch, tdc;

	vvt_counter ++;
	if (vvt_counter >= 720)
		vvt_counter = 0;
	if (vvt_counter % 3 == 0) {
		x = vvt_counter/3;	// 0 - 239
		y = (x >> 1) + 1;	// 1 - 120
		z = x & 0x01;

		//output NE58X, pos [58~59] + [58~59]
		mv = IS_IN_RANGE(y, 58, 59);
		mv |= IS_IN_RANGE(y, 118, 119);	//(58+60, 59+60);
		mv = !mv && z;
		pss_SetVolt(NE58X, mv);

		//output CAM1X, pos [56~14]
		mv = IS_IN_RANGE(y, 56, 74);	//14+60;
		pss_SetVolt(CAM1X, !mv);
	}

 	//output CAM4X_IN, pos [22~44] + [52~14] + [22~28] + [52~58]
	tmp = vvt_counter + vvt_gear_advance;
	mv = IS_IN_RANGE(tmp, 132, 264);
	mv |= IS_IN_RANGE(tmp, 312, 444);
	mv |= IS_IN_RANGE(tmp, 492, 528);
	mv |= IS_IN_RANGE(tmp, 672, 708);
	pss_SetVolt(CAM4X_IN, !mv);

	//output CAM4X_EXT, pos [12~34] + [42~4] + [12~18] + [42~48]
	tmp = vvt_counter - vvt_gear_advance;
	mv = IS_IN_RANGE(tmp, 72, 204);
	mv |= IS_IN_RANGE(tmp, 252, 384);
	mv |= IS_IN_RANGE(tmp, 432, 468);
	mv |= IS_IN_RANGE(tmp, 612, 648);
	pss_SetVolt(CAM4X_EXT, !mv);

	//knock
	for (ch = 0; ch < 4; ch++) {
		mv = (vvt_knock_pattern >> ch) & 0x01;
		if(mv != 0) {
			//enable knock signal output
			tdc = 120 + ch * 180;
			tdc += vvt_knock_pos;
			mv = IS_IN_RANGE(vvt_counter, tdc, tdc + vvt_knock_width) ? 1 : 0;
			knock_Enable(mv);
			if(mv)
				break;
		}
	}

	//misfire
	tmp = misfire_GetSpeed(x);	//x(0,239)
	pss_SetSpeed(tmp);
}

void EXTI9_5_IRQHandler(void)
{
	EXTI->PR = EXTI_Line6;
	vvt_isr();
}
