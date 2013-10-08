/*
*
* 1, during power-up, if calibration data is incorrect, err mode is entered
* 2, ybs do continously adc
* 3, according to 485 command, do corresponding response
*
*/

#include "ulp/sys.h"
#include "ybs.h"
#include "ybsd.h"
#include "nvm.h"
#include "led.h"

static struct ybs_cfg_s ybs_cfg __nvm;
static int ybs_mgf;

int main(void)
{
	sys_init();
	//mb_init(MB_RTU, 0x00, 9600);
	ybsd_vi_init();
	led_flash(LED_RED);
	IRQEN |= IRQ_ADC;
	//IRQEN |= IRQ_EINT2;
	while(1) {
		sys_update();
		//mb_update();
	}
}

void ADC_IRQHandler(void)
{
	int v = ybsd_vi_read();
}

int mb_hreg_cb(int addr, char *buf, int nregs, int flag_read)
{
	return 0;
}

