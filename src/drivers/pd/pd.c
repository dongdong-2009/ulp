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
#include "pd_linear.h"
#include "nvm.h"

pdl_t pd_pdl __nvm = PDL_DEF;

//vars to support software scrolling
static int pd_ofs_x = 0;
static int pd_ofs_y = 0;

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
	int x, y, w, h;
	rect_get(r, &x, &y, &w, &h);
	pd_ofs_x = x;
	pd_ofs_y = y;
	return 0;
}

int pd_GetEvent(dot_t *p)
{
	int ret;
	struct pd_sample sa;

	ret = pdd_get( &sa );
	if( !ret ) {
		ret = pdl_process( &pd_pdl, &sa);
		if( !ret ) {
			p -> x = sa.x + pd_ofs_x;
			p -> y = sa.y + pd_ofs_y;
			return PDE_CLICK;
		}
	}

	return PDE_NONE;
}



