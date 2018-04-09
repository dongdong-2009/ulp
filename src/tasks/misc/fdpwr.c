/*
*
* miaofng@2018-02-07 routine for fdpwr
*
*/

#include "ulp/sys.h"
#include <string.h>
#include "shell/shell.h"
#include "gpio.h"
#include "pwm.h"

//#define CONFIG_FDPLC_V1P0
#if CONFIG_FDPLC_V1P0 == 1
#define PWM_VBAT_DIV (100.0/(100.0+200.0))
#define PWM_IREF_DIV (100.0/(100.0+100.0))
#else
#define PWM_VBAT_DIV (100.0/(100.0+330.0)) //330K+100K
#define PWM_IREF_DIV (100.0/(100.0+100.0)) //100K+100K
#endif

#define ELOAD_IMIN 0.001 //eload turn on limit
#define ELOAD_ISCL 3.000 //short circuit current - high
#define ELOAD_ISCH 5.000 //short circuit current - low
#define ELOAD_IMID ((ELOAD_ISCL + ELOAD_ISCH) / 2)
#define POWER_VMIN 4.000 //vbat turn on limit
#define POWER_VMAX 16.50

typedef struct {
	const pwm_bus_t *pwm;
	const char *ELOADx_YES;
	const char *ELOADx_SCP;
	int cal;
} eload_t;

#define NR_OF_ELOADS (sizeof(eload_list) / sizeof(eload_list[0]))
static const eload_t eload_list[] = {
	{.cal = -1, .pwm = &pwm42, .ELOADx_YES = "ELOAD1_YES", .ELOADx_SCP = "ELOAD1_SCP"}, //ELOAD1
	{.cal = -1, .pwm = &pwm43, .ELOADx_YES = "ELOAD2_YES", .ELOADx_SCP = "ELOAD2_SCP"}, //ELOAD2
	{.cal = -1, .pwm = &pwm44, .ELOADx_YES = "ELOAD3_YES", .ELOADx_SCP = "ELOAD3_SCP"}, //ELOAD3
};

static float eload_isc = ELOAD_ISCL;
static float eload_iset[NR_OF_ELOADS];
static float power_vbat;
static float vpwm = 0.0; //only for debug purpose

static float power_set(float vbat)
{
	float vset = (vbat > 0) ? vbat : -vbat;
	vset = (vset > POWER_VMAX) ? POWER_VMAX : vset;

	//off?
	if(vset < POWER_VMIN) {
		gpio_set("VBAT_BST", 0); //auto turn off eload by hw
		gpio_set("VBAT_YES", 0);
		power_vbat = 0;
		return power_vbat;
	}

	pwm41.set(0);
	gpio_set("VBAT_POL", vbat < 0);
	gpio_set("VBAT_YES", 1);

#if CONFIG_FDPLC_V1P0 == 1
	//vbat = iset * 10K + 1.25v
	//iset = (vbat - 1.25v) / 10K
	//vpwm = iset * 510 * 3
	float iset = (vset - 1.25) / 1.0e4;
	vpwm = iset * 510;
	int dpwm = (int)(vpwm / PWM_VBAT_DIV * 999 / 3.0);
#else
	float iset = (vset - 1.25) / 1.0e4;
	vpwm = iset * 470;
	int dpwm = (int)(vpwm / PWM_VBAT_DIV * 999 / 3.0);
#endif
	pwm41.set(dpwm);
	power_vbat = (vbat > 0) ? vset : -vset;
	return power_vbat;
}

static float eload_set(int idx, float iset)
{
	sys_assert((idx >= 0) && (idx < NR_OF_ELOADS));
	const eload_t *eload = &eload_list[idx];

	//short
	if(iset > eload_isc - 1e-6) {
		if(eload_isc > ELOAD_IMID) gpio_set("ELOADx_R5A", 1);
		else gpio_set("ELOADx_R5A", 0);

		gpio_set(eload->ELOADx_YES, 0);
		gpio_set(eload->ELOADx_SCP, 1);

		eload_iset[idx] = eload_isc;
		return eload_isc;
	}

	//open
	gpio_set(eload->ELOADx_SCP, 0);
	gpio_set(eload->ELOADx_YES, 0);
	eload->pwm->set(0);
	if(iset < ELOAD_IMIN) { //all open?
		eload_iset[idx] = 0;

		int mask_y = 0;
		for(int i = 0; i < NR_OF_ELOADS; i ++) {
			if(eload_iset[i] >= ELOAD_IMIN) mask_y |= 1 << i;
		}

		if(mask_y == 0) {
			gpio_set("VBAT_BST", 0);
		}
		return 0;
	}

	//load
	gpio_set("VBAT_BST", 1);
	gpio_set(eload->ELOADx_YES, 1);

	/*
	if(eload->cal >= 0) {
		const vw_cal_t *cal = &vw_cals[eload->cal];
		iset = (int) (cal->g * iset + cal->d);
	}
	*/

#if CONFIG_FDPLC_V1P0 == 1
	//iset * 10mR * 20 + vpwm = 1.229v
	//vpwm = 1.229v - iset * 10mR* 20
	vpwm = 0.800 - 0.010 * 20 * iset; //0.8 is used dueto hw bug!!
	int dpwm = (int)(vpwm / PWM_IREF_DIV * 999 / 3.0);
	dpwm = 999 - dpwm;
#else
	//iset * 10mR * 20 + vpwm = 1.229v
	//vpwm = 1.229v - iset * 10mR* 20
	vpwm = 1.229 - 0.010 * 20 * iset;
	int dpwm = (int)(vpwm / PWM_IREF_DIV * 999 / 3.0);
#endif
	eload->pwm->set(dpwm);
	eload_iset[idx] = iset;
	return iset;
}

void fdpwr_init(void)
{
	GPIO_BIND(GPIO_PP0, PC12, VBAT_YES)
	GPIO_BIND(GPIO_PP0, PC09, VBAT_POL)
	GPIO_BIND(GPIO_PP0, PA08, VBAT_BST)

	GPIO_BIND(GPIO_PP0, PB00, ELOADx_CLK)
	GPIO_BIND(GPIO_PP0, PB15, ELOADx_R5A) //high -> 5A
	GPIO_BIND(GPIO_PP0, PB14, ELOAD1_SCP)
	GPIO_BIND(GPIO_PP0, PB13, ELOAD2_SCP)
	GPIO_BIND(GPIO_PP0, PB12, ELOAD3_SCP)
	GPIO_BIND(GPIO_PP0, PC08, ELOAD1_YES)
	GPIO_BIND(GPIO_PP0, PC07, ELOAD2_YES)
	GPIO_BIND(GPIO_PP0, PC06, ELOAD3_YES)

	const pwm_cfg_t boost_clk_cfg = {.hz = 400000, .fs = 19};
	pwm33.init(&boost_clk_cfg);
	pwm33.set(10);

	const pwm_cfg_t cfg = {.hz = 100000, .fs = 999};
	pwm41.init(&cfg);
	power_set(0);

	for(int idx = 0; idx < NR_OF_ELOADS; idx ++) {
		eload_list[idx].pwm->init(&cfg);
		eload_set(idx, 0);
		eload_list[idx].pwm->set(500); //for hw debug purpose
	}
}

void fdpwr_update(void)
{
}

static int cmd_power_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"POWER VBAT +/-x			Vbat = +/-x, unit: V\n"
	};

	if(argc > 2) {
		float vbat = 0;
		int n = sscanf(argv[2], "%f", &vbat);
		if(n > 0) {
			vbat = power_set(vbat);
			printf("<+0, vbat = %.3f V, vpwm = %.3f V\n", vbat, vpwm);
			return 0;
		}
	}

	printf("<-1, para error!\n");
	printf("%s", usage);
	return 0;
}

cmd_t cmd_power = {"POWER", cmd_power_func, "POWER i/f commands"};
DECLARE_SHELL_CMD(cmd_power)

static int cmd_eload_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"ELOAD x y			ELOADx(1..3) = y, unit: A\n"
		"ELOAD 0 y			All ELOADS = y, unit: A\n"
		"ELOAD ISC z		short circuit current config 3/5A, unit: A\n"
		"note: \n"
		"1, auto turn on VBAT_BST\n"
		"2, auto turn off VBAT_BST when all eloads are off\n"
		"3, vbat must on in order to turn on eload\n"
		"4, auto switch eload and short circuit mode\n"
	};

	if(argc == 3) {
		if(!strcmp(argv[1], "ISC")) {
			float isc = 0;
			int n = sscanf(argv[2], "%f", &isc);
			eload_isc = (isc >= ELOAD_IMID) ? ELOAD_ISCH : ELOAD_ISCL;
			printf("<+0, Isc = %.3f A\n", eload_isc);
			return 0;
		}

		int n = 0, eloadx = 0;
		float iset = 0, iget = 0.0;

		n += sscanf(argv[1], "%d", &eloadx);
		n += sscanf(argv[2], "%f", &iset);
		if(n == 2) {
			if((iset > ELOAD_IMIN) && (power_vbat < POWER_VMIN)) {
				printf("<-2, vbat error(= %.3fv)\n", power_vbat);
				return 0;
			}

			if((eloadx >= 0) && (eloadx <= NR_OF_ELOADS)) {
				for(int idx = 0; idx < NR_OF_ELOADS; idx ++) {
					if((eloadx == 0) || (idx == eloadx - 1)) {
						iget = eload_set(idx, iset);
					}
				}
				printf("<+0, Iset = %.3f A, vpwm = %.3f V\n", iget, vpwm);
				return 0;
			}

			printf("<-1, invalid eload index\n");
		}
	}

	printf("%s", usage);
	return 0;
}

cmd_t cmd_eload = {"ELOAD", cmd_eload_func, "ELOAD i/f commands"};
DECLARE_SHELL_CMD(cmd_eload)
