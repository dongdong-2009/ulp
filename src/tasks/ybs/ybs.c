/* yarn break sensor
 * design concept:
 * 	1, powerup (or triggered by reset key) calibration(to calcu Gs and Vs0)
 * 	2, read-dsp-write loop updated by hw timer isr (Vo = Voref + G/Gs/Go * (Vs - Vs0))
 * name conventions:
 * 	vr, reference voltage, output by ch1 of ad5663
 * 	vs, the "in+" of opa, which is the very weak stress sensor voltage
 * 	vi, opa output, that is mcu adc input, digital or analog voltage
 * 	vo, output by ch2 of ad5663(range: 0~2v5), or ybs output(range: 0-24v)
 * authors:
 * 	junjun@2011 initial version
 * 	feng.ni@2012 the concentration is the essence
 */

#include <stdio.h>
#include "config.h"
#include "sys/task.h"
#include "stm32f10x.h"
#include "ulp/device.h"
#include "ulp/dac.h"
#include "ybs.h"
#include "ybs_drv.h"
#include "ulp_time.h"
#include "shell/cmd.h"
#include <string.h>
#include <stdlib.h>
#include "nvm.h"

/*calibrate the opa offset & gain:
 * input: Vs(stress sensor output) , Vofs(offset voltage, output by DAC CH1)
 * output: D(ADC read back)
 * equation: Vs * Gs + Vofs * Gofs = D (note: according schematic, Gs = Gofs + 1)
 * solution:
 * 	1, Vs0 * Gs + Vofs0 *Gofs = Vi0
 * 	2, Vs0 * Gs + Vofs1 *Gofs = Vi1
 * 	3, eq2-eq1 => (Vofs1 - Vofs0) * Gofs = Vi1 - Vi0 => then we can got  Gs and Vs
 * note:
 * 	gain of stress sensor will be changed with the static dc resitance and  its pull down resistor
 * 	it's considered as an constant dueto the very weak influence in a real system.
 */
static int ybs_cal()
{
	int vi, vr, ve, vl, vh, i, vm;
	int vr1, vr2, vi1, vi2, gs, gr, vs;

#if 0
	for(vr = 0; vr < v2d(2.6); vr += mv2d(10)) {
		ybs_set_vr(vr);
		ybs_mdelay(10);
		vi = ybs_get_vi_mean();
		printf("cal: vr = %d mv, vi = %d mv\n", d2mv(vr), d2mv(vi));
	}
	for(vr = mv2d(1370); vr < mv2d(1380); vr += uv2d(20)) {
		ybs_set_vr(vr);
		ybs_mdelay(10);
		vi = ybs_get_vi_mean();
		printf("cal: vr = %d uv, vi = %d mv\n", d2uv(vr), d2mv(vi));
	}
#endif

#if 0
	//adc noise performance
	vr = mv2d(1370);
	ybs_set_vr(vr);
	ybs_mdelay(10);
	vi = ybs_get_vi_mean();
	printf("cal: vr = %d mv, vi = %d mv\n", d2mv(vr), d2mv(vi));
	for(i = 0; i < 128; i ++) {
		vi = ybs_get_vi();
		printf("cal: vr = %d mv, vi = %d mv\n", d2mv(vr), d2mv(vi));
	}
#endif

	//test vr = 0
	vr = v2d(0.0);
	ve = v2d(3.0);
	ybs_set_vr(vr);
	ybs_mdelay(10);
	vi = ybs_get_vi_mean(); //got it
	printf("cal: vr = %dmv, vi = %dmv", d2mv(vr), d2mv(vi));
	if(vi < ve) {
		printf("(<%dmv)...fail!!!\n", d2mv(ve));
		return -1;
	}
	printf("\n");

	//test vr = 2v5
	vr = v2d(2.5);
	ve = v2d(1.0);
	ybs_set_vr(vr);
	ybs_mdelay(10);
	vi = ybs_get_vi_mean(); //got it
	printf("cal: vr = %dmv, vi = %dmv", d2mv(vr), d2mv(vi));
	if(vi > ve) {
		printf("(>%dmv)...fail!!!\n", d2mv(vr), d2mv(vi));
		return -1;
	}
	printf("\n");

	//search the mid point of Vref where vi = 2.5v(range 2.5v->0.5v, best for OPA)
	vm = v2d(1.0);
	vl = v2d(0.0);
	vh = v2d(3.3);
	for(i = 0; i < 24; i ++) { //to try 24 times, can break in advance, but not necessary
		vr = (vl + vh) / 2;
		ybs_set_vr(vr);
		ybs_mdelay(10);
		vi = ybs_get_vi_mean();
		if(vi < vm) { //opa output too low, vref should down
			vh = vr;
		}
		else { //vref should up
			vl = vr;
		}

		//display
		//printf("cal: vr = %08duv, vi = %04dmv\n", d2uv(vr), d2mv(vi));
	}

	printf("cal: vr = %08duv, vi = %04dmv\n", d2uv(vr), d2mv(vi));
	vr1 = vr;
	vi1 = vi;

	//search the mid point of Vref where vi = 0.5v(range 2.5v->0.5v, best for OPA)
	vm = v2d(3.0);
	vl = v2d(0.0);
	vh = v2d(3.3);
	for(i = 0; i < 24; i ++) { //to try 24 times, can break in advance, but not necessary
		vr = (vl + vh) / 2;
		ybs_set_vr(vr);
		ybs_mdelay(10);
		vi = ybs_get_vi_mean();
		if(vi < vm) { //opa output too low, vref should down
			vh = vr;
		}
		else { //vref should up
			vl = vr;
		}

		//display
		//printf("cal: vr = %08duv, vi = %04dmv\n", d2uv(vr), d2mv(vi));
	}

	printf("cal: vr = %08duv, vi = %04dmv\n", d2uv(vr), d2mv(vi));
	vr2 = vr;
	vi2 = vi;

	//to calculate the gain, gs = 1 - gr
	gr = (vi2 - vi1) / (vr2 - vr1);
	gs = 1 - gr;

	//gs * vs + gr * vr = vi
	vs = (((vi1 >> 2) - gr * (vr1 >> 2)) / gs) << 2;
	printf("cal: gs = %d, vs = %duV\n", gs, d2uv(vs));
	return 0;
}

static int ybs_vo0 __nvm;
static int ybs_vi0;
static int ybs_gain __nvm; //g * 1024
#define _bn 14
#define _an 14
static struct filter_s ybs_vi_filter = { //fp/fs = 500Hz/20KHz
	.bn = _bn,
	.b0 = (int)(0.073120421271418046 * (1 << _bn)),
	.b1 = (int)(0.073120421271418046 * (1 << _bn)),
	.an = _an,
	.a0 = (int)(1.00000000000000000 * (1 << _an)),
	.a1 = (int)(-0.85375915745716391 * (1 << _an)),

	.xn_1 = 0,
	.yn_1 = 0,
};

static void ybs_init(void)
{
	ybs_vi0 = ybs_get_vi_mean();
	ybs_set_vo(ybs_vo0);
}

int filt(struct filter_s *f, int xn)
{
	int vb, va;
	vb = f->b0 * xn;
	vb += f->b1 * f->xn_1;
	vb >>= f->bn;

	va = f->a1 * f->yn_1;
	va = 0 - va;
	va >>= f->an;

	vb += va;

	f->xn_1 = xn;
	f->yn_1 = vb;
	return vb;
}

/*read-dsp-write loop*/
void ybs_update(void)
{
	int vo, vi = ybs_get_vi();
	vi >>= 10; // uV -> mV
	vi = filt(&ybs_vi_filter, vi);
	vi <<= 10;
	vi = ybs_vi0 - vi;
	vo = (vi >> 10) * ybs_gain;
	vo += ybs_vo0;
	ybs_set_vo(vo);
}

void ybs_mdelay(int ms)
{
	time_t timer = time_get(ms);
	while(time_left(timer) > 0) {
		task_Update();
	}
}

void main(void)
{
	task_Init();
	ybs_drv_init();
	ybs_stress_release();

	printf("Power Conditioning - YBS\n");
	printf("IAR C Version v%x.%x, Compile Date: %s,%s\n", (__VER__ >> 24),((__VER__ >> 12) & 0xfff),  __TIME__, __DATE__);
	if(!ybs_cal()) {
		ybs_init();
		ybs_start();
	}

	while(1){
		task_Update();
	}
}

//ybs shell command
static int cmd_ybs_func(int argc, char *argv[])
{
	const char *usage = {
		"ybs help                   usage of ybs commond\n"
		"ybs stress 0/1             simulator stress resistor off/on\n"
		"ybs vr uv                  set vr output voltage\n"
		"ybs vi n                   get filted and n times of rough vi in mv\n"
		"ybs gain g                 gain setting(gain = g/1024)\n"
		"ybs vo uv                  output offset setting\n"
		"ybs save                   save current settings\n"
	};

	if(argc == 3 && !strcmp(argv[1], "stress")) {
		int on = atoi(argv[2]);
		if(on & 0x01)
			ybs_stress_press();
		else
			ybs_stress_release();

		int vi = ybs_get_vi_mean();
		printf("vi(mv) = %d\n", d2mv(vi));
		return 0;
	}

	if(argc == 3 && !strcmp(argv[1], "vr")) {
		int uv = atoi(argv[2]);
		ybs_set_vr(uv2d(uv));
		return 0;
	}

	if(argc == 2 && !strcmp(argv[1], "save")) {
		nvm_save();
		return 0;
	}

	if(argc >= 2 && !strcmp(argv[1], "gain")) {
		if(argc > 2) {
			ybs_gain = atoi(argv[2]);
		}
		printf("ybs_gain = %d/1024\n", ybs_gain);
		return 0;
	}

	if(argc >= 2 && !strcmp(argv[1], "vo")) {
		if(argc > 2) {
			ybs_vo0 = uv2d(atoi(argv[2]));
			ybs_set_vo(ybs_vo0);
		}
		printf("ybs_vo0 = %dmv\n", d2mv(ybs_vo0));
		return 0;
	}

	if(argc >= 2 && !strcmp(argv[1], "vi")) {
		int vi, n = (argc > 2) ? atoi(argv[2]) : 0;
		vi = ybs_get_vi_mean();
		printf("vi(mv) = %d ", d2mv(vi));
		while(n > 0) {
			vi = ybs_get_vi();
			printf("%d ", d2mv(vi));
			n --;
		}
		printf("\n");
		return 0;
	}

	printf("%s", usage);
	return 0;
}

static const cmd_t cmd_ybs = {"ybs", cmd_ybs_func, "ybs debug command"};
DECLARE_SHELL_CMD(cmd_ybs)
