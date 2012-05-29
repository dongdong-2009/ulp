/*
 *	peng.guo@2012 initial version
 */

#include <string.h>
#include "config.h"
#include "sys/task.h"
#include "ulp_time.h"
#include "can.h"
#include "drv.h"
#include "Mbi5025.h"
#include "cfg.h"
#include "priv/usdt.h"
#include "ulp/debug.h"
#include "shell/cmd.h"
#include "led.h"
#include "pdi.h"

static const mbi5025_t pdi_mbi5025 = {
		.bus = &spi1,
		.idx = SPI_CS_DUMMY,
		.load_pin = SPI_CS_PC3,
		.oe_pin = SPI_CS_PC4,
};
void pdi_relay_init()
{
	mbi5025_Init(&pdi_mbi5025);		//SPI bus,shift register
	mbi5025_EnableOE(&pdi_mbi5025);
}

void pdi_set_relays(const struct pdi_cfg_s *sr)
{
	char *o = (char *)&(sr -> relay_ex);
	mbi5025_WriteByte(&pdi_mbi5025, *(o+3));
	mbi5025_WriteByte(&pdi_mbi5025, *(o+2));
	mbi5025_WriteByte(&pdi_mbi5025, *(o+1));
	mbi5025_WriteByte(&pdi_mbi5025, *(o+0));
	char *p = (char *)&(sr->relay);
	mbi5025_WriteByte(&pdi_mbi5025, *(p+3));
	mbi5025_WriteByte(&pdi_mbi5025, *(p+2));
	mbi5025_WriteByte(&pdi_mbi5025, *(p+1));
	mbi5025_WriteByte(&pdi_mbi5025, *(p+0));
	spi_cs_set(pdi_mbi5025.load_pin, 1);
	spi_cs_set(pdi_mbi5025.load_pin, 0);
}

void pdi_fail_action()
{
	pdi_IGN_off();
	led_off(LED_GREEN);
	led_off(LED_RED);
	led_on(LED_RED);
	counter_fail_add();
	beep_on();
	pdi_mdelay(3000);
	beep_off();
}

void pdi_pass_action()
{
	printf("##START##STATUS-100##END##\n");
	pdi_IGN_off();
	led_off(LED_GREEN);
	led_off(LED_RED);
	led_on(LED_GREEN);
	beep_on();
	pdi_mdelay(20);
	printf("##START##EC-Test Result : No Error...##END##\n");
	counter_pass_add();
	pdi_mdelay(750);
	beep_off();
	pdi_mdelay(140);
	beep_on();
	pdi_mdelay(750);
	beep_off();
}

void pdi_noton_action()
{
	led_off(LED_GREEN);
	led_off(LED_RED);
	for(int i = 0; i < 4; i++) {
		beep_on();
		led_on(LED_GREEN);
		led_on(LED_RED);
		pdi_mdelay(200);
		beep_off();
		led_off(LED_GREEN);
		led_off(LED_RED);
		pdi_mdelay(100);
	}
	for(int i = 0; i < 4; i++) {
		led_on(LED_GREEN);
		led_on(LED_RED);
		pdi_mdelay(200);
		led_off(LED_GREEN);
		led_off(LED_RED);
		pdi_mdelay(100);
	}
}

void counter_fail_add()
{
	counter_fail_rise();
	pdi_mdelay(40);
	counter_fail_down();
}

void counter_pass_add()
{
	counter_pass_rise();
	pdi_mdelay(40);
	counter_pass_down();
}

void pre_check_action()
{
	led_on(LED_RED);
	led_on(LED_GREEN);
	beep_on();
	pdi_mdelay(200);
	led_off(LED_RED);
	led_off(LED_GREEN);
	beep_off();
	pdi_mdelay(100);
	for(int i = 0; i < 5; i++){
		led_on(LED_RED);
		led_on(LED_GREEN);
		pdi_mdelay(200);
		led_off(LED_RED);
		led_off(LED_GREEN);
		pdi_mdelay(100);
	}
}

void start_action()
{
	start_botton_off();
	led_off(LED_GREEN);
	led_off(LED_RED);
	led_Update_Immediate();
	led_flash(LED_GREEN);
	led_flash(LED_RED);
}

static int cmd_pdi_func(int argc, char *argv[])
{
	const char *usage = {
		"pdi , usage:\n"
		"pdi battery on/off		power on/off\n"
		"pdi clear clear 		internal errorcode\n"
	};

	if (argc < 2) {
		printf("%s", usage);
		return 0;
	}
	//clear error code
	if(argc == 2) {
		/*clear internal errorcode*/
		if(argv[1][0] == 'c') {
			pdi_IGN_on();
			if (pdi_clear_dtc())
				printf("##ERROR##\n");
			else
				printf("##OK##\n");
			pdi_IGN_off();
		}
		/*start the diagnostic session*/
		if(argv[1][0] == 's')
			pdi_startsession();
	}

	/*power on/off*/
	if(argc == 3) {
		if(argv[1][0] == 'b') {
			if(argv[2][1] == 'n')
				pdi_IGN_on();
			if(argv[2][1] == 'f')
				pdi_IGN_off();
		}
	}

	return 0;
}
const cmd_t cmd_pdi = {"pdi", cmd_pdi_func, "pdi cmd i/f"};
DECLARE_SHELL_CMD(cmd_pdi)