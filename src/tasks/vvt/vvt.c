/*
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "vvt/vvt.h"
#include "vvt/misfire.h"
#include <stdio.h>
#include <stdlib.h>
#include "pss.h"
#include "knock.h"
#include "sys/task.h"
#include "osd/osd.h"
#include "key.h"

//global
short vvt_gear_advance; //0~10
short vvt_knock_pos;
short vvt_knock_width;
short vvt_knock_strength; //unit: mV
short vvt_knock_pattern; //...D C B A

//private
static short vvt_gear; //unit: tooth*0.5, range:(1~60*2)*2

//val ? [min, max]
#define IS_IN_RANGE(val, min, max) \
	(((val) >= (min)) && ((val) <= (max)))


static const int keymap[] = {
	KEY_UP,
	KEY_DOWN,
	KEY_ENTER,
	KEY_RIGHT,
	KEY_LEFT,
	KEY_RESET,
	KEY_NONE
};

void vvt_Init(void)
{
#ifdef CONFIG_TASK_OSD
	//handle osd display
	int hdlg = osd_ConstructDialog(&vvt_dlg);
	osd_SetActiveDialog(hdlg);

	//set key map
	key_SetLocalKeymap(keymap);
#endif

	pss_Init();
	knock_Init();
	misfire_Init();
	vvt_Stop();

	vvt_gear = 2; //tooth 1.0 1.1 2.0 2.1 ...
	vvt_knock_pos = 0;
	vvt_knock_width = 10;
	vvt_knock_pattern = 0;
}

void vvt_Update(void)
{
	vvt_knock_pattern = knock_GetPattern();

}

DECLARE_TASK(vvt_Init, vvt_Update)

void vvt_isr(void)
{
	int mv;
	int x, y, tmp;
	int ch, tdc;

	vvt_gear ++;
	if(vvt_gear > 241)
		vvt_gear = 2;

	x = vvt_gear >> 1;
	y = vvt_gear & 0x01;

	//output NE58X, pos [58~59] + [58~59]
	mv = IS_IN_RANGE(x, 58, 59);
	mv |= IS_IN_RANGE(x, 58+60, 59+60);
	mv = !mv && y;
	pss_SetVolt(NE58X, mv);

	//output CAM1X, pos [56~14]
	mv = IS_IN_RANGE(x, 56, 14+60);
	pss_SetVolt(CAM1X, !mv);

	//output CAM4X_IN, pos [22~44] + [52~14] + [22~28] + [52~58]
	tmp = x + vvt_gear_advance;
	mv = IS_IN_RANGE(tmp, 22, 44);
	mv |= IS_IN_RANGE(tmp, 52, 14+60);
	mv |= IS_IN_RANGE(tmp, 22+60, 28+60);
	mv |= IS_IN_RANGE(tmp, 52+60, 58+60);
	pss_SetVolt(CAM4X_IN, !mv);

	//output CAM4X_EXT, pos [12~34] + [42~4] + [12~18] + [42~48]
	tmp = x - vvt_gear_advance;
	mv = IS_IN_RANGE(tmp, 12, 34);
	mv |= IS_IN_RANGE(tmp, 42, 4+60);
	mv |= IS_IN_RANGE(tmp, 12+60, 18+60);
	mv |= IS_IN_RANGE(tmp, 42+60, 48+60);
	pss_SetVolt(CAM4X_EXT, !mv);

	//knock
	for (ch = 0; ch < 4; ch++) {
		mv = (vvt_knock_pattern >> ch) & 0x01;
		if(mv != 0) {
			//enable knock signal output
			tdc = 20 + ch * 30;
			tdc += vvt_knock_pos;
			mv = IS_IN_RANGE(x, tdc, tdc + vvt_knock_width) ? 1 : 0;
			knock_Enable(mv);
			if(mv)
				break;
		}
	}

	//misfire
	tmp = misfire_GetSpeed(vvt_gear);
	pss_SetSpeed(tmp);
}

