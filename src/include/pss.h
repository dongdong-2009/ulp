/*
 * 	miaofng@2010 initial version
 *		vvt position and speed sensor signal generator
 *		provide vvt signal: ne58x cam1x cam4x_in cam4x_ext vss
 */
#ifndef __PSS_H_
#define __PSS_H_

typedef enum {
	NE58X = 0,
	CAM1X,
	CAM4X_IN,
	CAM4X_EXT,
	PSS_CH_NR
} pss_ch_t;

void pss_Init(void);
void pss_Update(void);
void pss_Enable(int on);
void pss_SetSpeed(short hz); /*1hz speed -> 2hz dsso -> 1rpm*/
void pss_SetVolt(pss_ch_t ch, short mv); /*on/off?*/
void pss_SetVssSpeed(short hz);
void pss_SetVssVolt(short mv);

#endif /*__PSS_H_*/
