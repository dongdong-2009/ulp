/* time.c
 * 	miaofng@2009 initial version, be carefull of the overflow bug!!!
 */

#include "config.h"
#include "ulp_time.h"

void time_hwInit(void);

static time_t jiffies;

/*use RTC as timebase, tick period = 1ms*/
void time_Init(void)
{
	jiffies = 0;
#ifndef __SELF_DEBUG
#ifndef CONFIG_LIB_FREERTOS
	time_hwInit();
#endif
#endif
}

void time_Update(void)
{
}

void time_isr(void)
{
	jiffies ++;
}

/*unit: ms*/
time_t time_get(int delay)
{
	return (time_t)(jiffies + delay);
}

int time_left(time_t deadline)
{
	return time_diff(deadline, jiffies);
}

time_t time_shift(time_t time, int ms)
{
	int result = time + ms;
	return result;
}

int time_diff(time_t t0, time_t t1)
{
	int left = t0 - t1;
	return left;
}

void mdelay(int ms)
{
	ms *= CONFIG_TICK_HZ;
	ms /= 1000;
	time_t deadline = time_get(ms);
	while(time_left(deadline) > 0);
}

void sdelay(int ss)
{
	mdelay(ss * 1000);
}

#ifndef __SELF_DEBUG
#if CONFIG_CPU_STM32 == 1
#include "stm32f10x.h"
void udelay(int us)
{
	us *= (SystemFrequency / 9000000);
	while(us > 0) us--;
}
#elif CONFIG_CPU_LM3S == 1
#include "lm3s.h"
#elif CONFIG_CPU_SAM3U == 1
#include "sam3u.h"
#elif CONFIG_CPU_LPC178X == 1
#include "LPC177x_8x.h"
#include "system_LPC177x_8x.h"
#elif defined CONFIG_CPU_ADUC706X
#include "aduc706x.h"
#endif

void time_hwInit(void)
{
#if CONFIG_CPU_SAM3U == 1
	SysTick_Config(CONFIG_MCK_VALUE / CONFIG_TICK_HZ);
#elif CONFIG_CPU_LPC178X == 1
	SysTick_Config(SystemCoreClock / CONFIG_TICK_HZ);
#else
	SysTick_Config(SystemFrequency / CONFIG_TICK_HZ);
#endif
}
#endif

#ifdef __SELF_DEBUG
#include <stdio.h>
#include <string.h>
int main(int argc, char *argv[])
{
	time_t t0, t1;
	int ms = 0, delta = 0, mode = 't', steps = 17;
	const char *usage = {
		"time t ms		return t1 = time_shift(t0, ms) and time_diff(t1, t0)\n"
		"time t ms 100		t scan mode, delta t = 100\n"
		"time t ms 100ms   	ms scan mode, delta ms = 100mS\n"
		"time t ms mode steps	scan mode, if step is not given, default to 17\n"
	};

	switch(argc) {
	case 5:
		if(argv[4][1] == 'x') sscanf(argv[4], "0x%x", &steps);
		else sscanf(argv[4], "%d", &steps);
	case 4:
		if(argv[3][1] == 'x') sscanf(argv[3], "0x%x", &delta);
		else sscanf(argv[3], "%d", &delta);
		mode = (argv[3][strlen(argv[3])-1] == 's') ? 's' : 't';
	case 3:
		if(argv[2][1] == 'x') sscanf(argv[2], "0x%x", &ms);
		else sscanf(argv[2], "%d", &ms);
	case 2:
		if(argv[1][1] == 'x') sscanf(argv[1], "0x%x", &t0);
		else sscanf(argv[1], "%d", &t0);
		break;
	default:
		printf("%s", usage);
		return 0;
	}

	printf("t0 = 0x%08x(%d)\n", t0, t0);
	printf("ms = 0x%08x(%d)\n", ms, ms);
	printf("delta = 0x%08x(%d)\n", delta, delta);
	printf("steps = 0x%08x(%d)\n", steps, steps);

	for(int i = 0; i < steps; i ++) {
		t1 = time_shift(t0, ms);
		int ms_new = time_diff(t1, t0);
		printf("t0: %d t1: %d ms: %d -> %d\n", t0, t1, ms, ms_new);
		if(delta == 0) break;
		if(mode == 't') t0 += delta;
		else ms += delta;
	}
}
#endif
