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
#include "common/debounce.h"
#include <string.h>

static struct debounce_s rb_pressed;
static struct ybs_cfg_s ybs_cfg __nvm;
static float ybs_gf;
static int ybs_ms;
static struct {
	int analog_vout : 1; /*continue analog voltage out*/
	int digital_out	: 1; /*continue digital gf out*/
	int digital_bin : 1; /*binary mode or text mode*/
} ybs_mode;

/*cache for fast algorithm*/
static float ybs_gi, ybs_di, ybs_go, ybs_do;

static void algo_cache_update(void)
{
	float swgi, swgo, swdi, swdo;

	/*HW: Vybs = (gf_rough * swgo + swdo) * gdac * gamp + damp, note:
	1, amp = amplifier = ybs_a
	2, gdac = vout/din = vout_1v/din_1v = 1v/v2d(1v)
	*/
	swdo = (ybs_cfg.DY - ybs_cfg.DA) / ybs_cfg.GA * YBSD_DAC_V2D(1.00);
	swgo = ybs_cfg.GY / ybs_cfg.GA * YBSD_DAC_V2D(1.00);

	/*HW: gf_rough = ((gf * gh + vh) * gadc + swdi) * swgi
	= (gf * gh * gadc + (vh * gadc + swdi)) * swgi =>
	1) vh * gadc + swdi = 0
	2) gf * gh * gadc * swgi = gf => swgi = 1 / gh / gadc
	3) adc_gain = d_1 / vof(1) = 1 / YBSD_ADC_D2V(1)
	*/
	swdi = ybs_cfg.swdi;
	swgi = YBSD_ADC_D2V(1) / ybs_cfg.hwgi;

	/*CAL: gf_rough = (adc_value + swdi) * swgi;
	ybs_gf = gf_rough * Gi + Di = adc_value * (swgi * Gi) + (swdi * swgi * Gi + Di);
	*/
	ybs_gi = swgi * ybs_cfg.Gi;
	ybs_di = swgi * ybs_cfg.Gi * swdi + ybs_cfg.Di;

	/*CAL: dac_value = (gf * Go + Do) * swgo + swdo
	= gf * (Go * swgo) + (Do * swgo + swdo)
	*/
	ybs_go = ybs_cfg.Go * swgo;
	ybs_do = ybs_cfg.Do * swgo + swdo;
}

static char cksum(const void *data, int n)
{
	const char *p = (const char *) data;
	char sum = 0;

	for(int i = 0; i < n; i ++) sum += p[i];
	return sum;
}

static void config_save(void)
{
	ybs_cfg.cksum = 0;
	ybs_cfg.cksum = cksum(&ybs_cfg, sizeof(ybs_cfg));
	nvm_save();
}

static void input_offset_update(void)
{
	float v, avg = 0;
	int status = IRQEN;

	IRQCLR |= IRQ_ADC;
	for(int i = 0; i < 1000; i ++) {
		v = ybsd_vi_read();
		avg = (avg * i + v) / (i + 1);
		sys_mdelay(1);
	}
        ybs_cfg.swdi = -avg;
	IRQEN |= (status & IRQ_ADC);
}

static void ybs_init(void)
{
	led_flash(LED_RED);
	if((ybs_cfg.sn[0] == 0) || (!strcmp(ybs_cfg.sn, "UNKNOWN")) || cksum(&ybs_cfg, sizeof(ybs_cfg))) {
		led_flash(LED_RED);

		//try to use default parameters
		sprintf(ybs_cfg.sn, "UNKNOWN");
		ybs_cfg.Gi = ybs_cfg.Go = 1;
		ybs_cfg.Di = ybs_cfg.Do = 0;

		ybs_cfg.GA = AGAIN;
		ybs_cfg.DA = AVOFS;
		ybs_cfg.GY = YGAIN;
		ybs_cfg.DY = YVOFS;

		ybs_cfg.hwgi = -0.4/100/512; //400mv/100g/512;
		input_offset_update();
	}

	debounce_init(&rb_pressed, 300, ybsd_rb_get());
	algo_cache_update();
}

static void rb_update(void)
{
	if(debounce(&rb_pressed, ybsd_rb_get())) {
		if(rb_pressed.on) {
			input_offset_update();
			algo_cache_update();
			config_save();
		}
	}
}

static void ybs_d_update(void)
{
	static time_t ybs_timer = 0;
	if(ybs_mode.digital_out) {
		if((ybs_ms == 0) || (time_left(ybs_timer) <= 0)) {
			ybs_timer = time_get(ybs_ms);
			if(ybs_mode.digital_bin) {
			}
			else {
				printf("%.3f gf\n", ybs_gf);
			}
		}
	}
}

int main(void)
{
	IRQCLR |= IRQ_ADC;
	sys_init();
	ybsd_rb_init();
	ybsd_vi_init();
	ybsd_vo_init();
	ybs_init();
	//mb_init(MB_RTU, 0x00, 9600);
	IRQEN |= IRQ_ADC;
	while(1) {
		sys_update();
		//mb_update();
		rb_update();
		ybs_d_update();
	}
}

static void ybs_a_update(void)
{
	float v = ybsd_vi_read();
	ybs_gf = ybs_gi * v + ybs_di;
	v = v * ybs_go + ybs_do;
	ybsd_set_vo((int) v);
}

void ADC_IRQHandler(void)
{
	ybs_a_update();
}

int mb_hreg_cb(int addr, char *buf, int nregs, int flag_read)
{
	return 0;
}

#include "shell/cmd.h"
#if 1
static int cmd_ybs_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"ybs -i			ybs info read\n"
		"ybs -k			emulate reset key been pressed\n"
		"ybs -f	[ms]		read mgf10, binary mode\n"
		"ybs -F [ms]		read gf, float, text mode\n"
		"ybs -c Gi Di Go Do	set calibration paras, text mode\n"
		"ybs -S			save settings to nvm\n"
	};

	int ms;
	ybs_mode.digital_out = 0;
	for(int i = 1; i < argc ; i ++) {
		int e = (argv[i][0] != '-');
		if(!e) {
			switch(argv[i][1]) {
			case 'I':
				printf("SN: %s, CAL: %08x-%08x-%08x-%08x, CFG: %08x-%08x-%08x-%08x, HW: %08x-%08x, SW: %s %s\n",
					ybs_cfg.sn,
					*((int *)&ybs_cfg.Gi),
					*((int *)&ybs_cfg.Di),
					*((int *)&ybs_cfg.Go),
					*((int *)&ybs_cfg.Do),
					*((int *)&ybs_cfg.GA),
					*((int *)&ybs_cfg.DA),
					*((int *)&ybs_cfg.GY),
					*((int *)&ybs_cfg.DY),
					*((int *)&ybs_cfg.hwgi),
					*((int *)&ybs_cfg.swdi),
					__TIME__,
					__DATE__
				);
				break;
			case 'i':
			printf("SN: %s, CAL: %.3f-%.3f-%.3f-%.3f, CFG: %.3f-%.3f-%.3f-%.3f, HW: %.3fuV/g, %.0f, SW: %s %s\n",
				ybs_cfg.sn,
				ybs_cfg.Gi,
				ybs_cfg.Di,
				ybs_cfg.Go,
				ybs_cfg.Do,
				ybs_cfg.GA,
				ybs_cfg.DA,
				ybs_cfg.GY,
				ybs_cfg.DY,
				ybs_cfg.hwgi * 1000000,
				ybs_cfg.swdi,
				__TIME__,
				__DATE__
			);
			break;
			case 'k':
				input_offset_update();
				algo_cache_update();
				break;
			case 'f':
			case 'F':
				if(argv[i][1] == 'F') {
					ybs_mode.digital_bin = 1;
				}
				else {
					ybs_mode.digital_bin = 0;
					printf("%.3f\n", ybs_gf);
				}

				if((i + 1 < argc) && (sscanf(argv[i + 1], "%d", &ms) == 1)) {
					i ++;
					if(ybs_ms >= 0) {
						ybs_ms = ms;
						ybs_mode.digital_out = 1;
					}
				}
				break;

			case 'c':
			case 'S':
				nvm_save();
				break;
			default:
				break;
			}
		}
	}
}

const cmd_t cmd_ybs = {"ybs", cmd_ybs_func, "ybs debug commands"};
DECLARE_SHELL_CMD(cmd_ybs)
#endif
