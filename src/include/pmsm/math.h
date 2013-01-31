/*
 * 	king@2013 initial version
 */
#ifndef __MATH_H_
#define __MATH_H_

void sin_cos_q15(int theta, int *pSinVal, int *pCosVal);
void clarke_q15(int Ia, int Ib, int *pIalpha, int *pIbeta);
void park_q15(int Ialpha, int Ibeta, int *pId, int *pIq, int SinVal, int CosVal);
void inv_park_q15(int Id, int Iq, int *pIalpha, int *pIbeta, int SinVal, int CosVal);

#endif /*__MATH_H_*/
