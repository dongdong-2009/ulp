/* L6208.h
 * 	dusk@2010 initial version
 */
#ifndef __L6208_H_
#define __L6208_H_

#include "stm32f10x.h"

#define L6208_RST   	GPIO_Pin_11
#define L6208_ENA   	GPIO_Pin_12
#define L6208_DECAY 	GPIO_Pin_13
#define L6208_HoF   	GPIO_Pin_14
#define L6208_DIR   	GPIO_Pin_15

/*define take pwm as l6208's reference voltage*/
#define L6208_VREF_USE_PWM

//l6208 config or store to flash
typedef struct{
	char dir : 1;
	char stepmode : 1;
	char decaymode : 1;
}l6208_config_t;


/* sm direction */
enum {
	SM_DIR_Clockwise,
	SM_DIR_CounterClockwise
};

/*sm step mode*/
enum {
	SM_STEPMODE_FULL,
	SM_STEPMODE_HALF
};

/*sm magnetic decay mode*/
enum {
	SM_DECAYMODE_FAST,
	SM_DECAYMODE_SLOW
};


/* set the stepper Rotation Direction motor direction */
#define L6208_SET_RD_CW()   	GPIO_SetBits(GPIOB, L6208_DIR)
#define L6208_SET_RD_CCW()  	GPIO_ResetBits(GPIOB, L6208_DIR)

/* set the stepper motor step mode,half or full */
#define L6208_SET_MODE_HALF()   GPIO_SetBits(GPIOB, L6208_HoF)
#define L6208_SET_MODE_FULL()   GPIO_ResetBits(GPIOB, L6208_HoF)

/* set the stepper motor Decay Mode of coil,fast or slow */
#define L6208_SET_DM_FAST() 	GPIO_ResetBits(GPIOB, L6208_DECAY)
#define L6208_SET_DM_SLOW() 	GPIO_SetBits(GPIOB, L6208_DECAY)

/* enable or disable the stepper motor */
#define L6208_ENABLE()  		GPIO_SetBits(GPIOB,L6208_ENA)
#define L6208_DISABLE() 		GPIO_ResetBits(GPIOB,L6208_ENA)

void l6208_Init(const l6208_config_t * config);
void l6208_SetHomeState(void);
void l6208_SetClockHZ (int hz);
void l6208_Start(void);
void l6208_Stop(void);

#endif /*__L6208_H_*/
