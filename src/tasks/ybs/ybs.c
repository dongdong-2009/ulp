/*
*
* 1, during power-up, if calibration data is incorrect, err mode is entered
* 2, ybs do continously adc
* 3, according to 485 command, do corresponding response
* 4, pls unlock first, or ybs command will not function
*/

#include "ulp/sys.h"
#include "ybs.h"
#include "ybsd.h"
#include "ybsmb.h"
#include "nvm.h"
#include "led.h"
#include "common/debounce.h"
#include <string.h>
#include "uart.h"
#include "shell/shell.h"

#ifndef CONFIG_YBS_MODBUS
#define CONFIG_YBS_MODBUS 0
#endif

static struct ybs_mfg_data_s mfg_data __nvm;
static float ybs_gf;
static float ybs_gi, ybs_di, ybs_go, ybs_do; /*cache for ybs fast algorithm*/
static int ybs_ms = -1;
static time_t ybs_timer = 0;
static struct debounce_s ybs_key;
static int ybs_unlock = 0;
static struct {
	int analog_vout : 1; /*continue analog voltage out*/
	int digital_out	: 1; /*continue digital gf out*/
	int digital_bin : 1; /*binary mgf10 mode or text mode*/
} ybs_cfg;

static void ybs_reset_swdi(void)
{
	int v;
	int status = IRQEN;
	float avg = 0;

	IRQCLR |= IRQ_ADC;
	for(int i = 0; i < 32; i ++) {
		while(ybsd_vi_read(&v) == 0);
		avg = (avg * i + v) / (i + 1);
	}
        mfg_data.swdi = -avg;
	IRQEN |= (status & IRQ_ADC);
}

static void ybs_reset_cache(void)
{
	float swgi, swgo, swdi, swdo;

	/*HW: Vybs = (gf_rough * swgo + swdo) * gdac * gamp + damp, note:
	1, amp = amplifier = ybs_a
	2, gdac = vout/din = vout_1v/din_1v = 1v/v2d(1v)
	*/
	swdo = (mfg_data.DY - mfg_data.DA) / mfg_data.GA * YBSD_DAC_V2D(1.00);
	swgo = mfg_data.GY / mfg_data.GA * YBSD_DAC_V2D(1.00);

	/*HW: gf_rough = ((gf * gh + vh) * gadc + swdi) * swgi
	= (gf * gh * gadc + (vh * gadc + swdi)) * swgi =>
	1) vh * gadc + swdi = 0
	2) gf * gh * gadc * swgi = gf => swgi = 1 / gh / gadc
	3) adc_gain = d_1 / vof(1) = 1 / YBSD_ADC_D2V(1)
	*/
	swdi = mfg_data.swdi;
	swgi = YBSD_ADC_D2V(1) / mfg_data.hwgi;

	/*CAL: gf_rough = (adc_value + swdi) * swgi;
	ybs_gf = gf_rough * Gi + Di = adc_value * (swgi * Gi) + (swdi * swgi * Gi + Di);
	*/
	ybs_gi = swgi * mfg_data.Gi;
	ybs_di = swgi * mfg_data.Gi * swdi + mfg_data.Di;

	/*CAL: dac_value = (gf * Go + Do) * swgo + swdo
	= gf * (Go * swgo) + (Do * swgo + swdo)
	*/
	ybs_go = mfg_data.Go * swgo;
	ybs_do = mfg_data.Do * swgo + swdo;
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
	mfg_data.cksum = 0;
	mfg_data.cksum = -cksum(&mfg_data, sizeof(mfg_data));
	nvm_save();
}

static int gf_format_output(float gf)
{
	char buf[32], n;
	if(ybs_cfg.digital_bin) { //bin mode
		short mgf10 = (short)(gf * 100);
		uart_send(&uart0, &mgf10, sizeof(short));
		return 0;
	}

	//text mode
	n = snprintf(buf, 32, "%.3f gf\n", gf);
	uart_puts(&uart0, buf);
	return n;
}

static void ybs_init(void)
{
	led_flash(LED_RED);
	if((mfg_data.date == 0) || cksum(&mfg_data, sizeof(mfg_data))) {
		printf("manufacture data is corrupted!!!\nsystem reset now...\n");
		led_flash(LED_RED);

		//try to use default parameters
		sprintf(mfg_data.sn, "default");
		mfg_data.Gi = mfg_data.Go = 1;
		mfg_data.Di = mfg_data.Do = 0;

		mfg_data.GA = AGAIN;
		mfg_data.DA = AVOFS;
		mfg_data.GY = YGAIN;
		mfg_data.DY = YVOFS;

		mfg_data.hwgi = 0.4/100/512; //400mv/100g/512;
		ybs_reset_swdi();
	}

	debounce_init(&ybs_key, 300, ybsd_rb_get());
	ybs_reset_cache();
}

static void ybs_update(void)
{
	if(debounce(&ybs_key, ybsd_rb_get()) && ybs_key.on) {
		ybs_reset_swdi();
		ybs_reset_cache();
		config_save();
	}

	if(ybs_ms > 0) {
		if(time_left(ybs_timer) < 0) {
			ybs_timer = time_get(ybs_ms);
			gf_format_output(ybs_gf);
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
#if CONFIG_YBS_MODBUS
	mb_init(MB_RTU, 0x00, 9600);
#endif
	printf("ybs v2.0d, SW: %s %s\n\r", __DATE__, __TIME__);
	shell_mute((const struct console_s *) &uart0);
	IRQEN |= IRQ_ADC;
	while(1) {
		sys_update();
		ybs_update();
#if CONFIG_YBS_MODBUS
		mb_update();
#endif
	}
}

void ybs_isr(void)
{
	int v;

	ybsd_vi_read(&v);
	ybs_gf = ybs_gi * v + ybs_di;
	v = (int)(ybs_gf * ybs_go + ybs_do);
	ybsd_set_vo(v);

	if(ybs_ms == 0) {
		gf_format_output(ybs_gf);
	}
}

#include "shell/cmd.h"
#if 1
static int cmd_ybs_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"ybs -i			ybs info read\n"
		"ybs -k			emulate reset key been pressed\n"
		"ybs -F[/f] [ms]	read gf, float in text mode[/mgf10 in binary mode] in every ms\n"
		"ybs -r[/w]		read[/write] mfg data in binary mode\n"
		"ybs -S			save settings to nvm\n"
	};

	if(argc > 1) {
		if(strcmp(argv[1], "uuuuu") >= 0) { //unlock
			ybs_unlock = YBS_UNLOCK_ALL;
			uart0.putchar('0');
			return 0;
		}

		if(!strcmp(argv[1], "-u")) {
			const char *passwd[] = {
				"none", //YBS_UNLOCK_NONE
				//please add new ybs unlock passwd here ...
			};

			for(int i = 0; i < YBS_UNLOCK_ALL; i ++) {
				if(!strcmp(argv[2], passwd[i])) { //unlock success
					ybs_unlock = i;
					uart0.putchar('0');
					return 0;
				}
			}
		}
	}

	if(!ybs_unlock) {
		return 0;
	}

	int n, ms, e = -1;
	//ybs_cfg.digital_out = 0;
	ybs_cfg.digital_bin = 0; //default to text mode
	ybs_ms = -1; //disable ybs continue digital output mode
	char *p;

	for(int i = 1; i < argc ; i ++) {
		e = (argv[i][0] != '-');
		if(!e) {
			switch(argv[i][1]) {
			case 'i':
				printf("SN: %s, HW: %.3fuV/g(@%.0f), CAL: %.3f-%.3f-%.3f-%.3f, SW: %s %s\n",
					mfg_data.sn,
					mfg_data.hwgi * 1000000,
					mfg_data.swdi,
					mfg_data.Gi,
					mfg_data.Di,
					mfg_data.Go,
					mfg_data.Do,
					__TIME__,
					__DATE__
				);
				break;
			case 'k':
				uart0.putchar('0');
				ybs_reset_swdi();
				ybs_reset_cache();
				break;
			case 'f':
				ybs_cfg.digital_bin = 1;
			case 'F':
				if((i + 1 < argc) && (sscanf(argv[i + 1], "%d", &ms) == 1)) {
					i ++;
					ybs_ms = ms;
				}

				//output current value immediately
				ybs_timer = time_get(ybs_ms);
				gf_format_output(ybs_gf);
				break;
			case 'd': //debug purpose
				for(int i = 0; i < sizeof(mfg_data); i ++) {
					unsigned char *p = (unsigned char *) &mfg_data;
					printf("%02x ", p[i]);
				}
				break;
			case 'r':
				uart_send(&uart0, &mfg_data, sizeof(mfg_data));
				break;
			case 'w':
				p = (char *) &mfg_data;
				ybs_timer = time_get(100);
				for(n = 0; n < sizeof(mfg_data);) {
					if(time_left(ybs_timer) < 0)
						break;

					if(uart0.poll() > 0) {
						p[n] = (char) uart0.getchar();
						n ++;
					}
				}
				if((n != sizeof(mfg_data)) || cksum(&mfg_data, sizeof(mfg_data))) { //fail
					uart0.putchar('1');
					break;
				}
				//ok
				uart0.putchar('0');
				ybs_reset_cache();
				break;
			case 'S':
				config_save();
				uart0.putchar('0');
				break;
			default:
				break;
			}
		}
	}
	if(e) {
		printf("%s", usage);
	}
	return 0;
}

const cmd_t cmd_ybs = {"ybs", cmd_ybs_func, "ybs debug commands"};
DECLARE_SHELL_CMD(cmd_ybs)
#endif
