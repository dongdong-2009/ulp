/*
 * 	miaofng@2009 initial version
 */
#ifndef __MATH_H_
#define __MATH_H_

#define maxShort (1<<16)
#define midShort (1<<15)
#define divSQRT_3 (0.57735026918963*midShort) /* 1/sqrt(3) */

/*loopup table to get sin(theta), cos(theta) at the same time in order to increase speed*/
/*theta: unit: 0001 = pi/2/(2^13) = 0.01degree*/
void mtri(short theta, short *sin, short *cos);
#endif /*__CONSTANT_H_*/
