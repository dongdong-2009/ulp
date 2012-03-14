/*
 * David@2012 initial version
 */
#ifndef __WEIFU_H_
#define __WEIFU_H_

#define simulator_Start() do{clock_Enable(1);} while(0)
#define simulator_Stop() do{clock_Enable(0);} while(0)

//val ? [min, max]
#define IS_IN_RANGE(val, min, max) \
	(((val) >= (min)) && ((val) <= (max)))

//clock reference functions
void clock_Init(void);
void clock_SetFreq(short hz);

//for sine wave output functions, 58x, oil pump...
void axle_Init(int option);  //such as 58x, 36x
void axle_SetFreq(short hz);
void axle_SetAmp(short amp);
void op_Init(int option);    //such as 58x, 36x
void op_SetAmp(short amp);

void driver_Init(void);
void clock_Enable(int on);

#endif /*__WEIFU_H_*/