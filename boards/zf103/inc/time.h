/* time.h
 * 	miaofng@2009 initial version
 */
#ifndef __TIME_H_
#define __TIME_H_

void time_init(void);
void time_update(void);
int time_get(int delay); //unit: ms
int time_left(int deadline); //unit: ms
void udelay(int us);
void mdelay(int ms);
void sdelay(int ss);

#endif /*__TIME_H_*/
