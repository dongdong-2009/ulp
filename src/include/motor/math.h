/*
 * 	miaofng@2009 initial version
 */
#ifndef __MATH_H_
#define __MATH_H_

#include "sys/system.h"

/*loopup table to get sin(theta), cos(theta) at the same time in order to increase speed*/
/*theta = phi(unit: rad) / (2*pi) * (2^16)*/
void mtri(short theta, short *sin, short *cos);
short msigmoid(short x);

/*arctan(y/x), return (short) (phi/(2*pi) * (2^16)), range: -pi ~ pi */
short matan(short sin, short cos);
#endif /*__CONSTANT_H_*/
