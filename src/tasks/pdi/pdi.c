/*
 *	miaofng@2011 initial version
 *	David peng.guo@2011 add content for PDI_SDM10
 */
#include "config.h"
#include "sys/task.h"
#include "ulp_time.h"
#include "drv.h"
#include "mbi5025.h"
#include "ls1203.h"
#include "cfg.h"
#include "can.h"

static time_t pdi_loop_timer;
static char pdi_barcode[19];
static char pdi_cfg_name[9];
const struct pdi_rule_s* pdi_cfg;
const can_bus_t* pdi_can_bus = &can1;

static int cfg_filename(char *pdi_code , char *pdi_cfg_name)
{
	for(int i = 0;i <= 8;i++)
		pdi_cfg_name[i] = pdi_code[i];
	return 0;
}

static int pdi_check(const struct pdi_rule_s *sr)
{

}

static void pdi_init(void)
{
	pin_init();
	static mbi5025_t pdi_mbi5025 = {
		.bus = &spi1,
		.idx = SPI_CS_DUMMY,
		.load_pin = SPI_CS_PC3,
		.oe_pin = SPI_CS_PC4,
	};
	ls1203_t pdi_ls1203 = {
		.bus = &uart2,
		.data_len = 19,
		.dead_time = 20,
	};
	static can_cfg_t pdi_can = {
		.baud = 33330,
	};
	mbi5025_Init(&pdi_mbi5025);
	mbi5025_EnableOE(&pdi_mbi5025);
	ls1203_Init(&pdi_ls1203);
	pdi_swcan_mode();
	pdi_can_bus->init(&pdi_can);
}

/*static void pdi_update(void)
{
	while(1)
	{
		if(check_start() == 1) {
			while(1) {
				if(ls1203_Read(&pdi_ls1203,pdi_barcode) == 0)
					cfg_filename(barcode,pdi_cfg_name);
				pdi_cfg_file = pdi_cfg_get(pdi_cfg_name);
				pdi_check(pdi_cfg_file);
			}
		}
	}
}*/

/*int main(void)
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
}*/
