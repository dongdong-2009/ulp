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
static int ybs_cal(int *pgs, int *pvs)
{
	int vi, vr, ve, vl, vh, i, vm;
	int vr1, vr2, vi1, vi2, gs, gr, vs;

	//test vr = 0
	vr = v2d(0.0);
	ve = v2d(3.0);
	ybs_set_vr(vr);
	ybs_mdelay(10);
	vi = ybs_get_vi_mean(); //got it
	if(vi < ve) {
		printf("fail: vr = %dmv, vi = %dmv(<%dmv!)\n", d2mv(vr), d2mv(vi), d2mv(ve));
		return -1;
	}

	//test vr = 2v5
	vr = v2d(2.5);
	ve = v2d(0.3);
	ybs_set_vr(vr);
	ybs_mdelay(10);
	vi = ybs_get_vi_mean(); //got it
	if(vi > ve) {
		printf("fail: vr = %dmv, vi = %dmv(>%dmv!)\n", d2mv(vr), d2mv(vi), d2mv(ve));
		return -1;
	}

	//search the mid point of Vref where vi = 2.5v(range 2.5v->0.5v, best for OPA)
	vm = v2d(2.5);
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
		printf("cal %02d: vr = %08duv, vi = %04dmv\n", i, d2uv(vr), d2mv(vi));
	}

	vr1 = vr;
	vi1 = vi;

	//search the mid point of Vref where vi = 0.5v(range 2.5v->0.5v, best for OPA)
	vm = v2d(0.5);
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
		printf("cal: vr = %08duv, vi = %04dmv\n", d2uv(vr), d2mv(vi));
	}

	vr2 = vr;
	vi2 = vi;

	//to calculate the gain, gs = 1 - gr
	gr = (vi2 - vi1) / (vr2 - vr1);
	gs = 1 - gr;

	//gs * vs + gr * vr = vi
	vs = (vi1 - gr * vr1) / gs;

	*pgs = gs;
	*pvs = vs;

	printf("cal: gs = %d, vs = %dmV\n", gs, d2mv(vs));
	return 0;
}

static int ybs_gs;
static int ybs_vs0; //measured dc offset of stress sensor output

static void ybs_init(void)
{
	ybs_drv_init();
	ybs_stress_release();
}

/*read-dsp-write loop*/
void ybs_update(void)
{
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
	ybs_init();

	printf("Power Conditioning - YBS\n");
	printf("IAR C Version v%x.%x, Compile Date: %s,%s\n", (__VER__ >> 24),((__VER__ >> 12) & 0xfff),  __TIME__, __DATE__);
	if(!ybs_cal(&ybs_gs, &ybs_vs0))
		ybs_start();

	while(1){
		task_Update();
	}
}

//ybs shell command
static int cmd_ybs_func(int argc, char *argv[])
{
	const char *usage = {
		"ybs help            usage of ybs commond\n"
		"ybs stress            simulator resistor button,usage:pmos on/off\n"
	};
	printf("%s", usage);
	return 0;
}

static const cmd_t cmd_ybs = {"ybs", cmd_ybs_func, "ybs debug command"};
DECLARE_SHELL_CMD(cmd_ybs)
