/*
 * David@2011,9 initial version
 */
#ifndef __C131_MISC_H_
#define __C131_MISC_H_

//for failed led
#define FailLed_On()		GPIO_SetBits(GPIOC, GPIO_Pin_2)
#define FailLed_Off()		GPIO_ResetBits(GPIOC, GPIO_Pin_2)

//for buzzer
#define Buzzer_On()			GPIO_SetBits(GPIOC, GPIO_Pin_0)
#define Buzzer_Off()		GPIO_ResetBits(GPIOC, GPIO_Pin_0)

//for external power
#define Enable_EXTPWR()		GPIO_SetBits(GPIOC, GPIO_Pin_6)
#define Disable_EXTPWR()	GPIO_ResetBits(GPIOC, GPIO_Pin_6)

//for external led power
#define Enable_LEDPWR()		GPIO_SetBits(GPIOC, GPIO_Pin_5)
#define Disable_LEDPWR()	GPIO_ResetBits(GPIOC, GPIO_Pin_5)

//for sdm power
#define Enable_SDMPWR()		GPIO_SetBits(GPIOD, GPIO_Pin_13)
#define Disable_SDMPWR()	GPIO_ResetBits(GPIOD, GPIO_Pin_13)

//for sdm ready detect
#define Get_SDMStatus()		GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_8)

void c131_misc_Init(void);

#endif /*__C131_MISC_H_*/
