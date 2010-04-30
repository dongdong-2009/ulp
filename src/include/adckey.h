/* adckey.h
 * 	dusk@2010 initial version
 */
#ifndef __ADCKEY_H_
#define __ADCKEY_H_

#include "key.h"

void adckey_Init(void);
void adckey_Update(void);
unsigned int adckey_GetKey(void);
void adckey_SetKeyMap(void *);
#endif /*__ADCKEY_H_*/
