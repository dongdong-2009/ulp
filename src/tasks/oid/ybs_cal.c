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
#include "oid_mcd.h"
#include <string.h>
#include "led.h"
#include "stm32f10x.h"
#include "uart.h"
#include "shell/cmd.h"
#include "shell/shell.h"
#include "nvm.h"

#include "ybs_mon.h"
#include "oid_mcd.h"
#include "ybs_dio.h"
//#include "ybs_cal.h"

#include "common/tstep.h"
#include "common/debounce.h"
#include "common/polyfit.h"

#define YBS_NONLINEARITY_MAX	10000
static const float mgf_f[] = {0, 40000, 5000, 35000, 10000, 30000, 15000, 25000, 20000};
time_t test_timer = 0;

static struct {
	struct ybs_info_s ybs;
	int mv_det;
	int mv_asig;
	float *mgf_i; //final test result, will report to database
	float *mgf_o;
} test_result;

static int test_start(void)
{
	int e, mv;
	struct debounce_s power;

	//monitor ybs detection pin signal level - 8.62v
	debounce_init(&power, 5, 0);
	while(1) {
		e = mcd_xread(MCD_CH_DET, &mv);
		if(e) return -1;

		debounce(&power, mv > 5000);
		if(power.on) break;

		sys_update();
	}

	printf("ybs sensor is detected!!!\n");
	test_timer = time_get(0);
	return 0;
}

static int ybs_pre_check(void)
{
	int e, mv;

	e = mov_p(0);
	if(e) return -1;

	e = sgm_init();
	if(e) return -1;

	e = sgm_reset_to_zero();
	if(e) return -1;

	e = mcd_xread(MCD_CH_DET, &mv);
	if(e) return -1;

	test_result.mv_det = mv;
	printf("mv_det = %d\n", mv);
	if(OV_RANGE(mv, 8000, 9000)) {
		sys_error("ybs DET pin level over-range [%d, %d)", 8000, 9000);
		return -1;
	}

	e = mcd_xread(MCD_CH_ASIG, &mv);
	if(e) return -1;
	test_result.mv_asig = mv;
	printf("mv_asig = %d\n", mv);
	if(OV_RANGE(mv, 1500, 2500)) {
		sys_error("ybs ASIG pin level over-range [%d, %d)", 1500, 2500);
		return -1;
	}

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
	printf("Di: %04x(%3.0f mgf)\n", (unsigned) D2Y(Di) & 0xffff, Di);
	printf("Go: %04x(%f)\n", (unsigned) G2Y(Go) & 0xffff, Go);
	printf("Do: %04x(%3.0f mgf)\n", (unsigned) D2Y(Do) & 0xffff, Do);
	return 0;
}

static int ybs_cal_dac(void)
{
	int e, mv, i, n = sizeof(mgf_f)/sizeof(int);
	float *mgf_o = sys_malloc(sizeof(mgf_f));
	sys_assert(mgf_o != NULL);

	for(i = 0; i < n; i ++) {
		ybs_cal_write((int) mgf_f[i]);
		sys_mdelay(500);
		e = mcd_xread(MCD_CH_ASIG, &mv);
		if(e) {
			sys_free(mgf_o);
			return -1;
		}
		mgf_o[i] = MV2MGF(mv);
		printf("%5.0f mgf => %d mv(%5.0f mgf)\n", mgf_f[i], mv, mgf_o[i]);
	}

	float c0, c1, sumsq;
	gsl_fit_linear(mgf_f, mgf_o, n, &c0, &c1, &sumsq);
	printf("DAC: y = %.4f*x + %.0f (mgf), sumsq = %.0f\n", c1, c0, sumsq);

	/* calculation cal para
	y = (x * C1 + C0) * c1 + c0 = x * C1 * c1 + C0 * c1 + c0 = x
	=> C1 * c1 = 1; C0 * c1 + c0 = 0;
	=> C1 = 1/c1; C0 = -c0/c1;
	*/
	float C0 = - c0 / c1;
	float C1 = 1 / c1;
	printf("CAL: y = %.4f*x + %.0f (mgf)\n", C1, C0);

	test_result.ybs.Go = C1;
	test_result.ybs.Do = C0;

	float Go = C1;
	float Do = C0;
	printf("CAL: Go = %04x(%f), Do = %04x(%.0f)\n", (unsigned) G2Y(Go) & 0xffff, Go, (unsigned) D2Y(Do) & 0xffff, Do);

	if(OV_RANGE(c1, 0.5, 1.5)) {
		sys_error("ybs output stage gain(%.0f) over range[%f, %f)", c1, 0.5, 1.5);
		//sys_free(mgf_o);
		//return -1;
	}
	if(OV_RANGE(c0, 100, 300)) {
		sys_error("ybs output stage offset(%.0f) over range[%d, %d)", c0, 100, 300);
		//sys_free(mgf_o);
		//return -1;
	}
	if(sumsq > YBS_NONLINEARITY_MAX) {
		sys_error("ybs output stage linearity(%.0f) is too bad(> %d)", sumsq, YBS_NONLINEARITY_MAX);
		//sys_free(mgf_o);
		//return -1;
	}
	sys_free(mgf_o);
	return 0;
}

static int ybs_cal_adc(void)
{
	int e, mgf, i, n = sizeof(mgf_f)/sizeof(int);
	float *mgf_i = sys_malloc(sizeof(mgf_f));
	sys_assert(mgf_i != NULL);

	//disable ybs input stage calibration function
	e = ybs_cal_config(1, 0, test_result.ybs.Go, test_result.ybs.Do);
	if(e) return -1;

	for(i = 0; i < n; i ++) {
		if(mov_f((int) mgf_f[i], 0)) {
			sys_free(mgf_i);
			return -1;
		}
		e = ybs_read(&mgf);
		if(e) {
			sys_free(mgf_i);
			return -1;
		}
		mgf_i[i] = mgf;
		printf("%5.0f mgf => %5.0f mgf\n", mgf_f[i], mgf_i[i]);
	}

	printf("input stage performance data:\n");
	for(i = 0; i < n; i ++) {
		printf("%5.0f mgf => %5.0f mgf\n", mgf_f[i], mgf_i[i]);
	}

	float c0, c1, sumsq;
	gsl_fit_linear(mgf_f, mgf_i, n, &c0, &c1, &sumsq);
	printf("ADC: y = %.4f*x + %.0f (mgf), sumsq = %.0f\n", c1, c0, sumsq);

	/* calculation cal para
	y = (x * C1 + C0) * c1 + c0 = x * C1 * c1 + C0 * c1 + c0 = x
	=> C1 * c1 = 1; C0 * c1 + c0 = 0;
	=> C1 = 1/c1; C0 = -c0/c1;
	*/
	float C0 = - c0 / c1;
	float C1 = 1 / c1;
	printf("CAL: y = %.4f*x + %.0f (mgf)\n", C1, C0);

	test_result.ybs.Gi = C1;
	test_result.ybs.Di = C0;

	float Gi = C1;
	float Di = C0;
	printf("CAL: Gi = %04x(%f), Di = %04x(%.0f)\n", (unsigned) G2Y(Gi) & 0xffff, Gi, (unsigned) D2Y(Di) & 0xffff, Di);

	if(OV_RANGE(c1, 0.5, 1.5)) {
		sys_error("ybs input stage gain(%.0f) over range[%f, %f)", c1, 0.5, 1.5);
		//sys_free(mgf_i);
		//return -1;
	}
	if(OV_RANGE(c0, -5000, 5000)) {
		sys_error("ybs input stage offset(%.0f mgf) over range[%d, %d)", c0, -5000, 5000);
		//sys_free(mgf_i);
		//return -1;
	}
	/*
	if(sumsq > YBS_NONLINEARITY_MAX) {
		sys_error("ybs input stage linearity(%.0f) is too bad(> %d)", sumsq, YBS_NONLINEARITY_MAX);
		sys_free(mgf_i);
		return -1;
	}
	*/
	sys_free(mgf_i);
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
	//if(e) return -1;

	//e = ybs_reset();
	sys_mdelay(300);
	return e;
}

static int ybs_measure(void)
{
	int e, mv, mgf, i, n = sizeof(mgf_f)/sizeof(int);
	float *mgf_i = sys_malloc(sizeof(mgf_f));
	float *mgf_o = sys_malloc(sizeof(mgf_f));
	sys_assert((mgf_i != NULL) && (mgf_o != NULL));

	test_result.mgf_i = NULL;
	test_result.mgf_o = NULL;

	for(i = 0; i < n; i ++) {
		mov_f((int) mgf_f[i], 0);
		e = ybs_read(&mgf);
		if(e) {
			sys_free(mgf_i);
			sys_free(mgf_o);
			return -1;
		}
		mgf_i[i] = mgf;

		e = mcd_xread(MCD_CH_ASIG, &mv);
		if(e) {
			sys_free(mgf_i);
			sys_free(mgf_o);
			return -1;
		}
		mgf_o[i] = MV2MGF(mv);
		printf("%5.0f mgf => %5.0f mgf => %5.0f mgf(%d mv)\n", mgf_f[i], mgf_i[i], mgf_o[i], mv);
	}

	float c0, c1, sumsq;
	gsl_fit_linear(mgf_f, mgf_i, n, &c0, &c1, &sumsq);
	printf("YBS_DIGITAL: y = %.4f*x + %.0f (mgf), sumsq = %.0f\n", c1, c0, sumsq);
	if(0){//sumsq > YBS_NONLINEARITY_MAX) {
		sys_error("ybs digital output linearity(%.0f) is too bad(> %d)", sumsq, YBS_NONLINEARITY_MAX);
		sys_free(mgf_i);
		sys_free(mgf_o);
		return -1;
	}

	gsl_fit_linear(mgf_f, mgf_o, n, &c0, &c1, &sumsq);
	printf("YBS_ANALOG: y = %.4f*x + %.0f (mgf), sumsq = %.0f\n", c1, c0, sumsq);
	if(0) {//sumsq > YBS_NONLINEARITY_MAX) {
		sys_error("ybs analog output linearity(%.0f) is too bad(> %d)", sumsq, YBS_NONLINEARITY_MAX);
		sys_free(mgf_i);
		sys_free(mgf_o);
		return -1;
	}

	test_result.mgf_i = mgf_i;
	test_result.mgf_o = mgf_o;
	return 0;
}

static int ybs_report(void)
{
	float Gi = test_result.ybs.Gi;
	float Di = test_result.ybs.Di;
	float Go = test_result.ybs.Go;
	float Do = test_result.ybs.Do;

	printf("Test Result:\n");
	printf("SW: %s\n", test_result.ybs.sw);
	printf("SN: %s\n", test_result.ybs.sn);
	printf("Gi: %04x(%f)\n", (unsigned) G2Y(Gi) & 0xffff, Gi);
	printf("Di: %04x(%3.0f mgf)\n", (unsigned) D2Y(Di) & 0xffff, Di);
	printf("Go: %04x(%f)\n", (unsigned) G2Y(Go) & 0xffff, Go);
	printf("Do: %04x(%3.0f mgf)\n", (unsigned) D2Y(Do) & 0xffff, Do);

	int n = sizeof(mgf_f)/sizeof(int);
	printf("Performance Curve:\n");
	for(int i = 0; i < n; i ++) {
		printf("%5.0f mgf => %5.0f mgf => %5.0f mgf\n", mgf_f[i], test_result.mgf_i[i], test_result.mgf_o[i]);
	}

	sys_free(test_result.mgf_i);
	sys_free(test_result.mgf_o);
	return 0;
}

static int test_stop(void)
{
	int e, mv;
	struct debounce_s power;

	//reset ybs
	//ybs_reset();

	//mov back origin point
	mov_p(0);

	//
	int ms = -time_left(test_timer);
	float sec = ms / 1000;
	printf("Test finished in %.3f S\n", sec);

	//monitor ybs detection pin signal level - 8.62v
	debounce_init(&power, 3, 1);
	while(1) {
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
	if(mcd_init()) {
		sys_error("mcd init error");
		while(1) {
			sys_update();
		}
	}
	mcd_mode(DMM_V_AUTO);
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

