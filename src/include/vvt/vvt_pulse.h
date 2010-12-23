/*
 * 	miaofng@2010 initial version
 */
#ifndef __VVT_PULSE_H_
#define __VVT_PUSLE_H_

#define CH_NE58X			ADC_Channel_10
#define CH_KNOCK_FRQ		ADC_Channel_11
#define CH_MISFIRE_STREN	ADC_Channel_12
#define CH_VSS				ADC_Channel_13
#define CH_WSS				ADC_Channel_14

#define GPIO_MISFIRE_PATTERN	(GPIO_Pin_All & 0x003f)

//md204l related define
#define MD204L_READ_ADDR		0x20
#define MD204L_READ_LEN		0x04
#define MD204L_WRITE_ADDR		0x00
#define MD204L_WRITE_LEN		0x01

extern short vvt_adc[5];
extern short md204l_read_buf[MD204L_READ_LEN];
extern short md204l_write_buf[MD204L_WRITE_LEN];

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
void vvt_pulse_Update(void);

void pss_Update(void);
void pss_Enable(int on);
void pss_SetSpeed(short hz); /*1hz speed -> 2hz dsso -> 1rpm*/
void pss_SetVolt(pss_ch_t ch, short mv); /*on/off?*/
void pss_SetVssSpeed(short hz);
void pss_SetVssVolt(short mv);

void knock_SetFreq(short hz);
int knock_GetPattern(void);
void knock_Enable(int en);

int misfire_GetPattern(void);
int misfire_GetStrength(void);

void vss_SetFreq(short hz);
void wss_SetFreq(short hz);

void vvt_adc_Update(void);

#endif /*__VVT_PULSE_H_*/

