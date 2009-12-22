/*
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "math.h"

/*
* d=alpha*cos(theta)+beta*sin(theta)
* q=-alpha*sin(theta)+beta*cos(theta)        
*/
void park(const vector_t *pvi, vector_t *pvo, short theta)
{
	short sin, cos;
	int tmp1, tmp2;
	
	mtri(theta, &sin, &cos);
	tmp1 = pvi->alpha * cos;
	tmp2 = pvi->beta * sin;
	tmp1 = tmp1 >> 15;
	tmp2 = tmp2 >> 15;
	pvo->d = (short)(tmp1 + tmp2);
	
	tmp1 = pvi->alpha * sin;
	tmp2 = pvi->beta * cos;
	tmp1 = tmp1 >> 15;
	tmp2 = tmp2 >> 15;
	pvo->q = (short)(-tmp1 + tmp2);
}

/*
*                  alpha=d*cos(theta)-q*sin(theta)
*                  beta=d*sin(theta)+q*cos(theta)            
*/
void ipark(const vector_t *pvi, vector_t *pvo, short theta)
{
	short sin, cos;
	int tmp1, tmp2;
	
	mtri(theta, &sin, &cos);
	tmp1 = pvi->d * cos;
	tmp2 = pvi->q * sin;
	tmp1 = tmp1 >> 15;
	tmp2 = tmp2 >> 15;
	pvo->alpha = (short)(tmp1 - tmp2);
	
	tmp1 = pvi->d * sin;
	tmp2 = pvi->q * cos;
	tmp1 = tmp1 >> 15;
	tmp2 = tmp2 >> 15;
	pvo->beta = (short)(tmp2 + tmp1);
}

/*
* Clarke Transformation
*	alpha = a
*	beta = -(2*b+a)/sqrt(3)
*/
void clarke(const vector_t *pvi, vector_t *pvo)
{
	int a,b;
	
	/*calc alpha*/
	a = pvi->a;
	pvo->alpha = (short)a;
	
	/*calc beta*/
	a = a * divSQRT_3;
	a = a >> 15;
	b = b * divSQRT_3;
	b = b >> (15 - 1); /*b*2*/
	a = a + b;
	pvo->beta = (short)(-a);
}

/*iclarke is excuted by svpwm modulator*/
void iclarke(const vector_t *pvi, vector_t *pvo)
{
}
