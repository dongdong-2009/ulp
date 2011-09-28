/*
 * 	miaofng@2010 initial version
 */
#ifndef __MISFIRE_H_
#define __MISFIRE_H_

/*shared with command shell*/

void misfire_Init(void);
void misfire_SetSpeed(short hz);
void misfire_ConfigStrength(short strength);
void misfire_ConfigPattern(short pattern);
short misfire_GetSpeed(short gear);
#endif /*__VVT_H_*/

