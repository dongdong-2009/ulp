/*
 * 	miaofng@2010 initial version
 */
#ifndef __VVT_PULSE_H_
#define __VVT_PUSLE_H_

#define CH_NE58X			ADC_Channel_6
#define CH_VSS				ADC_Channel_7
#define CH_WSS				ADC_Channel_8
#define CH_KNOCK_FRQ		ADC_Channel_9
#define CH_MISFIRE_STREN	ADC_Channel_10

#define GPIO_KNOCK_PATTERN		(GPIO_Pin_All & 0x003f)
#define GPIO_MISFIRE_PATTERN	(GPIO_Pin_All & 0x003f)

typedef enum {
KS1 = 0,
KS2,
KS3,
KS4,
NR_OF_KS
} knock_ch_t;

typedef enum {
NE58X = 0,
CAM1X,
CAM4X_IN,
CAM4X_EXT,
PSS_CH_NR
} pss_ch_t;

void vvt_pulse_Init(void);

void pss_Update(void);
void pss_Enable(int on);
void pss_SetSpeed(short hz); /*1hz speed -> 2hz dsso -> 1rpm*/
void pss_SetVolt(pss_ch_t ch, short mv); /*on/off?*/
void pss_SetVssSpeed(short hz);
void pss_SetVssVolt(short mv);

void knock_Update(void);
void knock_SetFreq(short hz);
void knock_SetVolt(knock_ch_t ch, short mv);
int knock_GetPattern(void);
void knock_Enable(int en);

#endif /*__VVT_PULSE_H_*/

