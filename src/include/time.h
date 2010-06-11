/* time.h
 * 	miaofng@2009 initial version
 */
#ifndef __TIME_H_
#define __TIME_H_

typedef int time_t;

void time_Init(void);
void time_Update(void);
void time_isr(void);
time_t time_get(int delay); //unit: ms
int time_left(time_t deadline); //unit: ms
void udelay(int us);
void mdelay(int ms);
void sdelay(int ss);

#endif /*__TIME_H_*/
