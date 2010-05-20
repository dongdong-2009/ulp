/*
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "sys/system.h"
#include <stdio.h>
#include "motor/math.h"

void motor_Debug(void)
{
	sys_Init();

	/*clear screen*/
	/*ref: http://www.cnblogs.com/mugua/archive/2009/11/25/1610118.html*/
	putchar(0x1b);
	putchar('c');
	printf("This is motor selftest program\n");
	led_flash(LED_GREEN);
	
	/*test s16*s16*/
	short x1, x2, z;
	int tmp1,tmp2, y;
	
	while(1) {
		led_Update();
		
		scanf("%d %d ", &tmp1, &tmp2);
		printf("\n");
		
		x1 = (short)tmp1;
		x2 = (short)tmp2;
		y = x1 * x2;
		z = (short)y;
		printf("\n%04x*%04x=%08x(%d)\n", x1, x2, y, z);
	}
}
