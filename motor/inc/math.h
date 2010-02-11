/*
 * 	miaofng@2009 initial version
 */
#ifndef __MATH_H_
#define __MATH_H_

#include "board.h"

/*loopup table to get sin(theta), cos(theta) at the same time in order to increase speed*/
/*theta: unit: 0001 = pi/2/(2^13) = 0.01degree*/
void mtri(short theta, short *sin, short *cos);
short msigmoid(short x);
short matan(short x);
#endif /*__CONSTANT_H_*/
