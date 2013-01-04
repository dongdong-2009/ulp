/* time.h
 * 	miaofng@2009 initial version
 */
#ifndef __ULP_TIME_H_
#define __ULP_TIME_H_

#include <time.h>

void time_Init(void);
void time_Update(void);
void time_isr(void);
time_t time_get(int delay); //unit: ms
int time_left(time_t deadline); //unit: ms
time_t time_shift(time_t time, int ms);
int time_diff(time_t t0, time_t t1);
void udelay(int us);
void mdelay(int ms);
void sdelay(int ss);

/*rtc interface*/
void rtc_init(unsigned now);
unsigned rtc_get(void);
void rtc_alarm(unsigned t);

#endif /*__ULP_TIME_H_*/
