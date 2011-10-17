/*
 *	miaofng@2011 initial version
 *
 */
#include "config.h"
#include "sys/task.h"
#include "ulp_time.h"

time_t pdi_loop_timer;

static void pdi_init(void)
{
}

static void pdi_update(void)
{
}

int main(void)
{
	ulp_init();
	pdi_init();
	while(1) {
#if 1
		printf("loop = %dmS\n", - time_left(pdi_loop_timer));
		pdi_loop_timer = time_get(0);
#endif
		ulp_update();
		pdi_update();
	}
}
