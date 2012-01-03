#include "config.h"
#include "ulp/debug.h"
#include "ulp_time.h"
#include "wl.h"
#include "spi.h"
#include "sys/task.h"

static const struct nrf_cfg_s nrf_cfg = {
	.spi = &spi1,
	.gpio_cs = SPI_1_NSS,
	.gpio_ce = SPI_CS_PC5,
};

static int upa_Init(void)
{
	dev_register("nrf", &nrf_cfg);
	return 0;
}

static int upa_Update(void)
{
	return 0;
}

void main(void)
{
	task_Init();
	upa_Init();
	printf("\nPower Conditioning - UPA\n");
	printf("IAR C Version v%x.%x, Compile Date: %s,%s\n", (__VER__ >> 24),((__VER__ >> 12) & 0xfff),  __TIME__, __DATE__);

	while(1){
		task_Update();
		upa_Update();
	}
} // main
