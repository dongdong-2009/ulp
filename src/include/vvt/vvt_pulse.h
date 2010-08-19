/*
 * 	miaofng@2010 initial version
 */
#ifndef __VVT_H_
#define __VVT_H_

#define GPIO_KNOCK_PATTERN	(GPIO_Pin_All & 0x003f)
#define CH_NE58X			ADC_Channel_6
#define CH_VSS				ADC_Channel_7
#define CH_WSS				ADC_Channel_8
#define CH_KNOCK_FRQ		ADC_Channel_9
#define CH_MISFIRE_STREN	ADC_Channel_10
#define ADC3_DR_Address    ((uint32_t)0x40013C4C)

#endif /*__VVT_H_*/

