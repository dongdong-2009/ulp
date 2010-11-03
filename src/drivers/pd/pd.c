/*
 * 	miaofng@2010 initial version
 *		-virtual clear_all/clear_rect/scroll support
 */

#include "config.h"
#include "lcd.h"
#include <stdlib.h>
#include <string.h>
#include "FreeRTOS.h"
#include "common/glib.h"
#include "pd.h"
#include "tsc2046.h"

int pd_Init(void)
{
	int ret;

#if CONFIG_PD_BUS_SPI1 == 1
	const spi_bus_t *spi = &spi1;
	int idx = SPI_1_NSS;
#elif CONFIG_PD_BUS_SPI2 == 1
	const spi_bus_t *spi = &spi2;
	int idx = SPI_2_NSS;
#elif CONFIG_PD_BUS_SPI3 == 1
	const spi_bus_t *spi = &spi3;
	int idx = SPI_3_NSS;
#endif

#if CONFIG_PD_TSC2046 == 1
	tsc2046_t tsc = {
		.bus = spi,
		.idx = idx,
	};

	ret = tsc2046_init(&tsc);
#endif

	return ret;
}

int pd_SetMargin(const rect_t *r)
{
	return 0;
}

int pd_GetEvent(dot_t *p)
{
	return PDE_NONE;
}



