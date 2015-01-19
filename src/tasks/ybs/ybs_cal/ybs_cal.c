/*
 * 	miaofng@2013-3-27 initial version
 *	instruments needed:
 *	1, android, takes charge of gui&database
 *	2, mcd, for multi-channel measurement
 *	3, stepper motor, to generate force
 *	4, sgm, for force measurement, unit: mgf
 *	5, ybs, calibration protocol is implemented
 *
 */

#include "ulp/sys.h"
#include "shell/cmd.h"
#include "spi.h"
#include "common/vchip.h"
#include "ybs_mcd.h"
#include <string.h>
#include "led.h"
#include "stm32f10x.h"
#include "uart.h"
#include "shell/cmd.h"
#include "shell/shell.h"
#include "nvm.h"

#include "ybs_mon.h"
#include "ybs_mcd.h"
#include "ybs_dio.h"
//#include "ybs_cal.h"

#include "common/tstep.h"
#include "common/debounce.h"
#include "common/polyfit.h"
#include <math.h>

time_t test_timer = 0;

static struct {
	struct ybs_info_s ybs;
	float vdet; //ybs present or not?
	float vasig;
	float gf_per_step;

	struct {
		#define DAC_N 10
		float gfi[DAC_N];
		float gfo[DAC_N];
		float C0;
		float C1;
		float gfe; //average error: unit: gf
	} dac;

	struct {
		#define ADC_N 16
		float pos[ADC_N]; //it is used to verify sgm's accurateness
		float gfi[ADC_N]; //measured by sgm
		float gfd[ADC_N]; //measured by ybs
		float C0;
		float C1;
		float gfe; //average error, unit: gf
	} adc;

	struct {
		#define YBS_N 16
		float pos[YBS_N]; //it is used to verify sgm's accurateness
		float gfi[YBS_N]; //measured by sgm
		float gfd[YBS_N]; //ybs digital output
		float gfa[YBS_N]; //ybs analog output
		float gfe_d; //average ditial output error
		float gfe_a; //average anglog output error
	};
} test_result;

static int test_start(void)
{
	int e, mv;
	struct debounce_s power;

	printf("\033]@IDLE\007");
	if(mcd_init()) {
		sys_error("mcd init error");
		sys_update();
		return -1;
	}
	mcd_mode(DMM_V_AUTO);

	//glvar init
	memset(&test_result, 0, sizeof(test_result));
	strcpy(test_result.ybs.sn, "20130000000");
	strcpy(test_result.ybs.sw, "ybs version ???");

	//monitor ybs detection pin signal level - 8.62v
	int points = 0;
	time_t timer = time_get(0);
	debounce_init(&power, 5, 0);
	char *str = "IDLE";
	while(1) {
		if(time_left(timer) < 0) {
			printf("\033]@");
			for(int i = 0; i < points; i ++) printf("%c", str[i]);
			printf("\007");
			points ++;
			points = (points > 4) ? 0 : points;
			timer = time_get(500);
		}

		e = mcd_xread(MCD_CH_DET, &mv);
		if(e) return -1;

		debounce(&power, mv > 5000);
		if(power.on) break;

		sys_update();
	}

	printf("\033]@BUSY\007");
	printf("ybs sensor is detected!!!\n");
	test_timer = time_get(0);
	return 0;
}

static int ybs_pre_check(void)
{
	int e, mv;

	printf("try to read ybs info ...\n");
	e = ybs_init(&test_result.ybs);
	if(e) return -1;

	float Gi = test_result.ybs.Gi;
	float Di = test_result.ybs.Di;
	float Go = test_result.ybs.Go;
	float Do = test_result.ybs.Do;

	printf("SW: %s\n", test_result.ybs.sw);
	printf("SN: %s\n", test_result.ybs.sn);
	printf("Gi: %04x(%f)\n", (unsigned) G2Y(Gi) & 0xffff, Gi);
	printf("Di: %04x(%.3f gf)\n", (unsigned) D2Y(Di) & 0xffff, Di);
	printf("Go: %04x(%f)\n", (unsigned) G2Y(Go) & 0xffff, Go);
	printf("Do: %04x(%.3f gf)\n", (unsigned) D2Y(Do) & 0xffff, Do);

	//initial positioning
	e = mov_i(5000, 40.0);
	if(e) return -1;

	e = mcd_xread(MCD_CH_DET, &mv);
	if(e) return -1;

	test_result.vdet = mv / 1000.0;
	printf("vdet = %.3f\n", test_result.vdet);
	if(OV_RANGE(test_result.vdet, 8.0, 9.0)) {
		sys_error("ybs DET pin level over-range [%.1fv, %.1fv)", 8.0, 9.0);
		//return -1;
	}

	e = mcd_xread(MCD_CH_ASIG, &mv);
	if(e) return -1;
	test_result.vasig = mv / 1000.0;
	printf("vasig = %.3f\n", test_result.vasig);
	if(OV_RANGE(test_result.vasig, 1.5, 2.5)) {
		sys_error("ybs ASIG pin level over-range [%.1fv, %.1fv)", 1.5, 2.5);
		//return -1;
	}

	float gf100, gf200;
	if(mov_p_measure(100, &gf100, NULL)) return -1;
	if(mov_p_measure(200, &gf200, NULL)) return -1;
	if((gf100 > 0) && (gf200 > gf100)) {
		test_result.gf_per_step = (gf200 - gf100) / 100;
		printf("gf_per_step = %.3f\n", test_result.gf_per_step);
		mov_p(-500);
		return 0;
	}
	sys_error("spring leaf abnormal(gf100=%.1f, gf200=%.1f)", gf100, gf200);
	return -1;
}

static int ybs_cal_dac(void)
{
	int mv, N = DAC_N;
	float c0, c1, sumsq;

	float *x = test_result.dac.gfi;
	float *y = test_result.dac.gfo;

	for(int i = 0; i < N; i ++) {
		x[i] = 40.0 * i / N;
		if(ybs_cal_write(x[i])) return -1;
		sys_mdelay(500);
		if(mcd_xread(MCD_CH_ASIG, &mv)) return -1;
		y[i] = MV2GF(mv);
		printf("%.1f gf => %.3f gf(%04d mv)\n", x[i], y[i], mv);
	}

	gsl_fit_linear(x, y, N, &c0, &c1, &sumsq);
	test_result.dac.C0 = c0;
	test_result.dac.C1 = c1;
	test_result.dac.gfe = powf(sumsq / N, 0.5);
	printf("DAC: y = %.3f*x + %.3f gf, gfe = %.3f\n", test_result.dac.C1, test_result.dac.C0, test_result.dac.gfe);

	/* calculation cal para
	y = (x * C1 + C0) * c1 + c0 = x * C1 * c1 + C0 * c1 + c0 = x
	=> C1 * c1 = 1; C0 * c1 + c0 = 0;
	=> C1 = 1/c1; C0 = -c0/c1;
	*/
	float C0 = - c0 / c1;
	float C1 = 1 / c1;

	test_result.ybs.Do = C0;
	test_result.ybs.Go = C1;
	printf("CAL: y = %.4f*x + %.3f gf\n", C1, C0);
	return 0;
}

//ezplot('(-(x-0.25)*0.8*exp(-((x-0.25)^4)*5)+x)*40', [0,1]); grid on; zoom on;
float mspread_func(float x, float x_vip)
{
	float xx = x - x_vip;
	float y = - powf(xx,4) * 5;
	y = expf(y);
	y *= -xx * 0.8;
	y += x;
	return y;
}

int mspread(float *gf, int N, float gf_min, float gf_max, float gf_vip)
{
	float gf_delta = (gf_max - gf_min) / (N - 1);
	for(int i = 0; i < N; i ++) {
		float x = gf_min + gf_delta * i;
		gf[i] = mspread_func(x/gf_max, gf_vip/gf_max) * gf_max;
		//printf("%.1f ", gf[i]);
	}
	return 0;
}

static int ybs_cal_adc(void)
{
	int i, k, N = ADC_N;

	//sample point position estimate
	float gf20_p = 20.0 / test_result.gf_per_step;
	float gf05_p = 05.0 / test_result.gf_per_step;
	mspread(test_result.adc.pos, N, 0, gf20_p, gf05_p);

	//change ybs to calibration mode
	if(ybs_cal_config(1.0, 0, 0, 0)) return -1;

	//measure
	for(i = 0; i < N; i ++) {
		float gfi, gfd;
		int steps = (int) test_result.adc.pos[i];
		if(mov_p_measure(steps, &gfi, &gfd)) return -1;
		test_result.adc.gfi[i] = gfi;
		test_result.adc.gfd[i] = gfd;
		printf("ADC: pos,sgm,ybs= %d %.1f %.1f\n", steps, gfi, gfd);
	}

	//sgm linearity identify
	float c0, c1, sumsq, gfe;
	gsl_fit_linear(test_result.adc.pos, test_result.adc.gfi, N, &c0, &c1, &sumsq);
	gfe = powf(sumsq / N, 0.5);

	float *p = test_result.adc.pos;
	float *x = sys_malloc(sizeof(float) * N);
	float *y = sys_malloc(sizeof(float) * N);
	sys_assert((x != NULL) && (y != NULL));

	gfe *= 2;
	printf("filtering good samples:\n");
	for(i = 0, k = 0; i < N; i ++) {
		float e = test_result.adc.gfi[i] - (p[i] * c1 + c0);
		e = (e > 0) ? e : 0;
		if(e < gfe) {
			x[k] = test_result.adc.gfi[k];
			y[k] = test_result.adc.gfd[k];
			printf("ADC: pos,sgm,ybs= %.0f %.1f %.1f\n", p[i], x[k], y[k]);
			k ++;
		}
	}

	//now we found k good samples
	gsl_fit_linear(x, y, k, &c0, &c1, &sumsq);
	sys_free(x);
	sys_free(y);

	test_result.adc.C0 = c0;
	test_result.adc.C1 = c1;
	test_result.adc.gfe = powf(sumsq / k, 0.5);
	printf("ADC: y = %.3f*x + %.3f gf, gfe = %.3f\n", test_result.adc.C1, test_result.adc.C0, test_result.adc.gfe);

	/* calculation cal para
	y = (x * c1 + c0) * C1 + C0 = x
	=> x * c1 * C1 = x; => C1 = 1/c1;
	=> c0 * C1 + C0 = 0; => C0 = - c0 * C1 = - c0 * 1/c1 = -c0/c1
	*/
	float C0 = - c0 / c1;
	float C1 = 1 / c1;

	test_result.ybs.Di = C0;
	test_result.ybs.Gi = C1;
	printf("CAL: y = %.4f*x + %.3f gf\n", C1, C0);
	return 0;
}

static int ybs_cal_cfg(void)
{
	float Gi = test_result.ybs.Gi;
	float Di = test_result.ybs.Di;
	float Go = test_result.ybs.Go;
	float Do = test_result.ybs.Do;
	int e = ybs_cal_config(Gi, Di, Go, Do);
	if(e) return -1;

	e = ybs_save();
	if(e) return -1;

	//e = ybs_reset();
	sys_mdelay(300);
	return e;
}

static int ybs_measure(void)
{
	int N = YBS_N;

	//sample point position estimate
	float gf20_p = 20.0 / test_result.gf_per_step;
	float gf05_p = 05.0 / test_result.gf_per_step;
	mspread(test_result.pos, N, 0, gf20_p, gf05_p);

	//measure
	for(int i = 0; i < N; i ++) {
		int mv;
		float gfi, gfd, gfa;
		int steps = (int) test_result.pos[i];
		if(mov_p_measure(steps, &gfi, &gfd)) return -1;
		test_result.gfi[i] = gfi;
		test_result.gfd[i] = gfd;

		if(mcd_xread(MCD_CH_ASIG, &mv)) return -1;
		gfa = MV2GF(mv);
		test_result.gfa[i] = gfa;
		printf("YBS: pos,sgm,gfd,gfa= %d %.1f %.1f %.1f\n", steps, gfi, gfd, gfa);
	}

	float *x = test_result.gfi;
	float *y = test_result.gfd;

	float c0, c1, sumsq;
	gsl_fit_linear(x, y, N, &c0, &c1, &sumsq);
	test_result.gfe_d = powf(sumsq / N, 0.5);

	y = test_result.gfa;
	gsl_fit_linear(x, y, N, &c0, &c1, &sumsq);
	test_result.gfe_a = powf(sumsq / N, 0.5);
	printf("ybs average measurement error: Digital %.1f Analog %.1f\n", test_result.gfe_d, test_result.gfe_a);
	return 0;
}

static void ybs_report_array(float *p, int n)
{
	printf("array(%.3f", p[0]);
	for(int i = 1; i < n; i ++) {
		printf(", %.3f", p[i]);
	}
	printf(")");
}

static int ybs_report(void)
{
	//escape seq start
	printf("\033]\007");
	printf("\033]#");

	printf("RECORD||");
	printf("%s||", test_result.ybs.sn);
	printf("CAL||");
	printf("%s||", (tstep_fail()) ? "FAIL" : "PASS");

	//test result detail start ===>
	struct ybs_info_s *ybs = &test_result.ybs;
	printf("\"sw\"=>\"%s\", \"Gi\"=>%f, \"Di\"=>%.3f, \"Go\"=>%f, \"Do\"=>%.3f", ybs->sw, ybs->Gi, ybs->Di, ybs->Go, ybs->Do);
	printf(", \"vdet\"=>%.3f", test_result.vdet);
	printf(", \"asig\"=>%.3f", test_result.vasig);
	printf(", \"gf_per_step\"=>%f", test_result.gf_per_step);
	printf(", \"gfe_d\"=>%.3f", test_result.gfe_d);
	printf(", \"gfe_a\"=>%.3f", test_result.gfe_a);

	printf(", \"pos\"=>"); ybs_report_array(test_result.pos, YBS_N);
	printf(", \"gfi\"=>"); ybs_report_array(test_result.gfi, YBS_N);
	printf(", \"gfd\"=>"); ybs_report_array(test_result.gfd, YBS_N);
	printf(", \"gfa\"=>"); ybs_report_array(test_result.gfa, YBS_N);
	//test result detail <=== end

	printf("||0");

	//escape seq end
	printf("\007");
	return 0;
}

static int test_stop(void)
{
	int e, mv;
	struct debounce_s power;

#ifdef CONFIG_BRD_HWV20
	mov_p(-2500);
#endif
#ifdef CONFIG_BRD_HWV10
	mov_p(-5000);
#endif

	//reset ybs
	//ybs_reset();

	//
	int ms = -time_left(test_timer);
	float sec = ms / 1000;
	printf("Test finished in %.0f S\n", sec);

	time_t timer = time_get(0);
	//monitor ybs detection pin signal level - 8.62v
	debounce_init(&power, 3, 1);
	while(1) {
		if(time_left(timer) <= 0) {
			timer = time_get(30 * 1000); //30s
			if(tstep_fail()) printf("\033]@FAIL\007");
			else printf("\033]@PASS\007");
		}

		e = mcd_xread(MCD_CH_DET, &mv);
		if(e) return -E_MCD_READ;

		debounce(&power, mv > 1000);
		if(power.off) break;

		sys_update();
	}
	return 0;
}

const struct tstep_s steps[] = {
	{.test = test_start, .desc = "wait for dut be placed"},
	{.test = ybs_pre_check, .desc  = "dut pre-check & get sn"},
	{.test = ybs_cal_dac, .desc = "ybs output stage calibration"},
	{.test = ybs_cal_adc, .desc = "ybs input stage calibration"},
	{.test = ybs_cal_cfg, .desc = "ybs cal para writeback & restart"},
	{.test = ybs_measure, .desc = "ybs finally performance test"},
	{.test = ybs_report, .desc = "ybs test result upload to database"},
	{.test = test_stop, .desc = "test stop, wait for dut be removed"},
};

void main(void)
{
	sys_init();
	led_flash(LED_RED);
	led_flash(LED_GREEN);
	monitor_init();
	while(0) {
		sys_update();
	}
	tstep_execute(steps, sizeof(steps)/sizeof(struct tstep_s));
}

void __sys_update(void)
{
}

static int cmd_cal_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"cal -o			to cal DAC stage\n"
		"cal -i			to cal ADC stage\n"
		"cal -r			to report result\n"
	};

	int e = 1, ecode;
	if(argc > 1) {
		e = 0;
		for(int i = 1; i < argc; i ++) {
			e += (argv[i][0] != '-');
			switch(argv[i][1]) {
			case 'o':
				ecode = ybs_cal_dac();
				printf("reset %s(%d)\n", (ecode) ? "FAIL" : "PASS", ecode);
				break;
			case 'i':
				ecode = ybs_cal_adc();
				break;
			case 'r':
				ybs_report();
				break;
			default:
				e ++;
			}
		}
	}

	if(e) {
		printf("%s", usage);
	}
	return 0;
}

const cmd_t cmd_cal = {"cal", cmd_cal_func, "ybs calibration cmds"};
DECLARE_SHELL_CMD(cmd_cal)

