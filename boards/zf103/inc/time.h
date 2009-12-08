/* time.h
 * 	miaofng@2009 initial version
 */
#ifndef __TIME_H_
#define __TIME_H_

void time_init(void);
#define time_update()
unsigned int time_get(unsigned int delay); //unit: ms
unsigned int time_left(unsigned int deadline); //unit: ms
void udelay(unsigned int us);
void mdelay(unsigned int ms);
void sdelay(unsigned int ss);

#endif /*__TIME_H_*/
