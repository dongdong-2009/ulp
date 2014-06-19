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

#define YBS_FILTER_ORDER4

//chebyshev filter: Fs = 100Hz, Fc = 10Hz order = 2
#ifdef YBS_FILTER_ORDER2
#define CHEB1_DEN1 -1.1996775682021836f
#define CHEB1_DEN2 0.51573875617675269f
#define CHEB1_GAIN 0.079015296993642237f
#endif

//chebyshev filter: Fs = 100Hz, Fc = 10Hz order = 4
#ifdef YBS_FILTER_ORDER4
#define CHEB1_DEN1 -1.4995544968104402
#define CHEB1_DEN2 0.84821868171669579
#define CHEB1_GAIN 0.087166046226563931

#define CHEB2_DEN1 -1.5547851795965149
#define CHEB2_DEN2 0.64929543813658097
#define CHEB2_GAIN 0.023627564635016512
#endif

static struct ybs_mfg_data_s mfg_data __nvm;
static int ybs_vadc = 0;
static float ybs_gf;
static float ybs_gi, ybs_di, ybs_go, ybs_do; /*cache for ybs fast algorithm*/
static short ybs_ms = -1;
static time_t ybs_timer = 0;
static struct debounce_s ybs_key;
static char ybs_unlock = 0;
static char ybs_ecode;
static char ybs_ecode_new;
static time_t ybs_ecode_timer = 0;
static struct cheb2_s ybs_cheb1;
#ifdef YBS_FILTER_ORDER4
static struct cheb2_s ybs_cheb2;
#endif
static char ybs_temp[sizeof(struct ybs_mfg_data_s)]; /*temp buffer*/

static struct {
	int analog_vout : 1; /*continue analog voltage out*/
	int digital_out	: 1; /*continue digital gf out*/
	int digital_bin : 1; /*binary mgf10 mode or text mode*/
} ybs_cfg;

static char cksum(const void *data, int n)
{
	const char *p = (const char *) data;
	char sum = 0;

	for(int i = 0; i < n; i ++) sum += p[i];
	return sum;
}

void cheb2_init(struct cheb2_s *f, float d1, float d2, float gain)
{
	memset(f, 0x00, sizeof(struct cheb2_s));
	f->den1 = (int)(d1 * (1 << 13));
	f->den2 = (int)(d2 * (1 << 13));
	f->gain = (int)(gain * (1 << 13));
}

int cheb2(struct cheb2_s *f, int x)
{
	int x2, x1;
	int y2, y1, y;
	int v, v1, v2;

	x2 = f->x2;
	x1 = f->x1;
	y2 = f->y2;
	y1 = f->y1;

	y = x + x1 + x1 + x2;
	//v = f->den1 * y1 + f->den2 * y2;
	v1 = f->den1 * (y1 >> 13);
	v2 = f->den2 * (y2 >> 13);
	v = v1 + v2;
	y -= v;

	f->x2 = x1;
	f->x1 = x;
	f->y2 = y1;
	f->y1 = y;

	//output
	y = (y >> 13) * f->gain;
	return y;
}

static void ybs_error_init(void)
{
	ybs_ecode = YBS_E_INVALID;
	ybs_ecode_new = YBS_E_OK;
	ybs_ecode_timer = time_get(-1);
}

static void ybs_error_update(void)
{
	int update = 0;

	//handle error state switching
	if(ybs_ecode != ybs_ecode_new) {
		if(ybs_ecode_new == YBS_E_ADC_OVLD) { //adc ovld has the lowest priority
			if(ybs_ecode == YBS_E_OK) {
				ybs_ecode = YBS_E_ADC_OVLD;
				update = 1;
			}

			if(ybs_ecode == YBS_E_ADC_OVLD) { //trig ...
				ybs_ecode_timer = time_get(1000);
			}
		}
		else {
			ybs_ecode = ybs_ecode_new;
			update = 1;
		}
	}

	//auto clear adc ovld error
	if(ybs_ecode == YBS_E_ADC_OVLD) {
		if(time_left(ybs_ecode_timer) < 0) {
			ybs_ecode_timer = time_get(0);
			ybs_ecode_new = YBS_E_OK;
		}
	}

	//handle led display
	if(update) {
		led_error(ybs_ecode);
		if(ybs_ecode == YBS_E_OK) {
			led_flash(LED_RED);
		}
	}
}

void ybs_error(int ecode)
{
	ybs_ecode_new = ecode;
}

static int ybs_reset_swdi(void)
{
	int v, f;

	//chebyshev iir filter: fs=100Hz fc=1Hz
	//cheb2_init(&ybs_cheb1, -1.9291698173542138f, 0.93337555158934971f, 0.0010514335587839682f);
	cheb2_init(&ybs_cheb1, CHEB1_DEN1, CHEB1_DEN2, CHEB1_GAIN);
	#ifdef YBS_FILTER_ORDER4
	cheb2_init(&ybs_cheb2, CHEB2_DEN1, CHEB2_DEN2, CHEB2_GAIN);
	#endif

	IRQCLR |= IRQ_ADC;
	ybs_timer = time_get(50);
	for(int i = 0; i < 100; i ++) {
		while((ybsd_vi_read(&v) & 0x01) == 0){
			if(time_left(ybs_timer) < 0) {
				ybs_timer = time_get(50);
				led_inv(LED_RED);
			}
		}

		f = cheb2(&ybs_cheb1, v);
		#ifdef YBS_FILTER_ORDER4
		f = cheb2(&ybs_cheb2, f);
		#endif
		//printf("%d %d\n", v, f);
	}
	IRQEN |= IRQ_ADC;

	if(1) {
		mfg_data.swdi = -f;

		//update cksum
		mfg_data.cksum = 0;
		mfg_data.cksum = -cksum(&mfg_data, sizeof(mfg_data));
	}

	//restore filter parameters
	cheb2_init(&ybs_cheb1, CHEB1_DEN1, CHEB1_DEN2, CHEB1_GAIN);
	#ifdef YBS_FILTER_ORDER4
	cheb2_init(&ybs_cheb2, CHEB2_DEN1, CHEB2_DEN2, CHEB2_GAIN);
	#endif
	return 0;
}

static void ybs_reset_cache(void)
{
	float swgi, swgo, swdi, swdo;
	IRQCLR |= IRQ_ADC;
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
	IRQEN |= IRQ_ADC;
}

static void config_save(void)
{
	nvm_save();
}

static int gf_format_output(float gf)
{
	if(ybs_cfg.digital_bin) { //bin mode
		short mgf10 = (short)(gf * 100);
		uart_send(&uart0, &mgf10, sizeof(short));
		return 0;
	}

	//text mode
	char *line = ybs_temp;
	int n = snprintf(line, 64, "%.3f gf( %d )\n", gf, ybs_vadc);
	uart_puts(&uart0, line);
	return n;
}

static void ybs_load_default_mfg_data(void)
{
	//try to use default parameters
	sprintf(mfg_data.sn, "default");

	mfg_data.adcflt = ADCFLT_DEF;
	mfg_data.Gi = mfg_data.Go = 1;
	mfg_data.Di = mfg_data.Do = 0;

	mfg_data.GA = AGAIN;
	mfg_data.DA = AVOFS;
	mfg_data.GY = YGAIN;
	mfg_data.DY = YVOFS;

	mfg_data.hwgi = 0.4/100/512; //400mv/100g/512;
	mfg_data.swdi = 0;
}

static void ybs_init(void)
{
	ybs_error_init();
	if((mfg_data.sn[0] == 0) || (cksum(&mfg_data, sizeof(mfg_data)) != 0)) {
		ybs_error(YBS_E_MFG_DATA);
		ybs_load_default_mfg_data();
	}

	debounce_init(&ybs_key, 300, ybsd_rb_get());
	ybs_reset_cache();
	cheb2_init(&ybs_cheb1, CHEB1_DEN1, CHEB1_DEN2, CHEB1_GAIN);
	#ifdef YBS_FILTER_ORDER4
	cheb2_init(&ybs_cheb2, CHEB2_DEN1, CHEB2_DEN2, CHEB2_GAIN);
	#endif
}

static void ybs_update(void)
{
	ybs_error_update();

	//handle reset key
	if(debounce(&ybs_key, ybsd_rb_get()) && ybs_key.off) {
		ybs_reset_swdi();
		ybs_reset_cache();
		config_save();
		ybs_error_init();
	}

	//handle gf display
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
	printf("ybs v2.0d, SW: %s %s\n\r", __DATE__, __TIME__);
	ybs_init();
	ybsd_rb_init();
	ybsd_vi_init(mfg_data.adcflt);
	ybsd_vo_init();
#if CONFIG_YBS_MODBUS
	mb_init(MB_RTU, 0x00, 9600);
#endif
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
	#define ADC0CERR (1 << 12)
	int digi, status = ybsd_vi_read(&ybs_vadc);
	if (status & ADC0CERR) {
		ybs_error(YBS_E_ADC_OVLD);
	}
	else {
		digi = cheb2(&ybs_cheb1, ybs_vadc);
		#ifdef YBS_FILTER_ORDER4
		digi = cheb2(&ybs_cheb2, digi);
		#endif
		ybs_gf = ybs_gi * digi + ybs_di;
		digi = (int)(ybs_gf * ybs_go + ybs_do);
		ybsd_set_vo(digi);
	}

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
		"ybs -c [flt_hex/[af sf chop]]	aduc adcflt reg config\n"
		"ybs -I			load default mfg data\n"
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

	int n, v, ms, e = -1;
	ybs_cfg.digital_bin = 0; //default to text mode
	ybs_ms = -1; //disable ybs continue digital output mode
	char *p;

	for(int i = 1; i < argc ; i ++) {
		e = (argv[i][0] != '-');
		if(!e) {
			switch(argv[i][1]) {
			case 'i':
				printf("SN: %s, HW: %.3fuV/g(@%.0f), FLT: 0x%04x, CAL: %.3f-%.3f-%.3f-%.3f, SW: %s %s\n",
					mfg_data.sn,
					mfg_data.hwgi * 1000000,
					mfg_data.swdi,
					((unsigned) mfg_data.adcflt) & 0xffff,
					mfg_data.Gi,
					mfg_data.Di,
					mfg_data.Go,
					mfg_data.Do,
					__TIME__,
					__DATE__
				);
				break;
			case 'I':
				ybs_load_default_mfg_data();
				uart0.putchar('0');
				break;
			case 'k':
				ybs_reset_swdi();
				ybs_reset_cache();
				ybs_error_init();
				uart0.putchar('0');
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
				p = ybs_temp;
				ybs_timer = time_get(500);
				uart_flush(&uart0);

				//read sizeof(mfg_data) bytes
				IRQCLR |= IRQ_ADC;
				for(n = 0; n < sizeof(mfg_data);) {
					if(time_left(ybs_timer) < 0)
						break;

					if(uart0.poll() > 0) {
						p[n] = (char) uart0.getchar();
						uart0.putchar(p[n]); //echo back
						n ++;
					}
				}
				IRQEN |= IRQ_ADC;
				//verify data received
				if(n == sizeof(mfg_data)) {
					if(cksum(p, sizeof(mfg_data)) == 0) { //pass
						uart0.putchar('0');
						memcpy(&mfg_data, p, sizeof(mfg_data));
						ybs_reset_cache();
						break;
					}
				}
				//fail
				uart0.putchar('1');
				break;
			case 'S':
				config_save();
				uart0.putchar('0');
				break;
			case 'c':
				v = mfg_data.adcflt;
				if(argc == 3) { //ybs -c adcflt
					v = htoi(argv[2]);
					i ++;
				}
				if(argc == 5) { //ybs -c af sf chop
					int af = atoi(argv[2]);
					int sf = atoi(argv[3]);
					int chop = atoi(argv[4]);
					v &= ~((1 << 15) | (0x3f << 8) | 0x7f);
					v |= (af & 0x3f) << 8;
					v |= (sf & 0x7f);
					v |= chop & 0x01;
					i += 3;
				}
				mfg_data.adcflt = v;
				//update cksum
				mfg_data.cksum = 0;
				mfg_data.cksum = -cksum(&mfg_data, sizeof(mfg_data));
				IRQCLR |= IRQ_ADC;
				ybsd_vi_init(v);
				IRQEN |= IRQ_ADC;
				printf("adcflt = 0x%02x\n", v);
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
