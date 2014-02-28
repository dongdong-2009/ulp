/*
 * 	miaofng@2013-3-20 initial version
 *	drivers for ybs monitor v1.0(aduc7061 based)
 *	board designed by xiaoming.xu&mingjing.ding
 *
 *	the most difficult problem is movement control.
 *	according to experiment, fast move needs about
 *	15s time to settle down. move_f's performance
 *	is better. which is more reliable? sliding bench
 *	or stain gauge meter?
 *
 */

#include "ulp/sys.h"
#include "shell/cmd.h"
#include "spi.h"
#include "common/vchip.h"
#include "ybs_mcd.h"
#include <string.h>
#include "led.h"
#include "common/debounce.h"
#include "stm32f10x.h"
#include "uart.h"
#include "shell/cmd.h"
#include "shell/shell.h"
#include "nvm.h"
#include "ybs_mon.h"
#include "ybs_dio.h"

#define ybs_motor_pusle 5000
static float ybs_gf_max = 0;

static int ybs_steps_max = -1;
static int ybs_steps_min = -1;
static int ybs_steps = -1; /*note: ybs_steps_min <= ybs_steps <= ybs_steps_max, and 0 is the rough touch point*/

int monitor_init(void)
{
	/* pinmap:
	PE2	DO	YBS_PASS
	PE3	DO	YBS_FAIL
	PC2	DI	MOTOR_NEAR
	PC3	DI	MOTOR_AWAY
	PA0	DO	MOTOR_PULSE
	PA1	DO	MOTOR_DIR
	PC4	DI	KEY_START
	PE0	DO	USART2_S0
	PE1	DO	USART2_S1
	*/
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOE, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_WriteBit(GPIOE, GPIO_Pin_0, Bit_RESET); //USART2_S0 = 0
	GPIO_WriteBit(GPIOE, GPIO_Pin_1, Bit_RESET); //USART2_S1 = 0
	GPIO_WriteBit(GPIOE, GPIO_Pin_2, Bit_RESET); //YBS_FAIL = 0
	GPIO_WriteBit(GPIOE, GPIO_Pin_3, Bit_RESET); //YBS_PASS = 0
	GPIO_WriteBit(GPIOA, GPIO_Pin_0, Bit_RESET); //MOTOR_PULSE = 0
	GPIO_WriteBit(GPIOA, GPIO_Pin_1, Bit_RESET); //MOTOR_DIR = 0
	return 0;
}

static void __sgm_init(void)
{
	//sgm uart init
#ifdef CONFIG_BRD_HWV20 //fuck JJ!!!
	GPIO_WriteBit(GPIOE, GPIO_Pin_1, Bit_RESET); //USART2_S0 = 1
	GPIO_WriteBit(GPIOE, GPIO_Pin_0, Bit_SET); //USART2_S1 = 1
#else
	GPIO_WriteBit(GPIOE, GPIO_Pin_0, Bit_SET); //USART2_S0 = 1
	GPIO_WriteBit(GPIOE, GPIO_Pin_1, Bit_SET); //USART2_S1 = 1
#endif

	uart_cfg_t cfg = {
		.baud = 19200,
	};
	uart2.init(&cfg);
	uart_flush(&uart2);
}

static int __sgm_simple_cmd(char *cmd)
{
	__sgm_init();
	uart_puts(&uart2, cmd);

	int i = 0;
	char echo[3];
	time_t deadline = time_get(10);
	while(1) {
		if(time_left(deadline) < 0) {
			sys_error("sgm response timeout(cmd = %s)", cmd);
			return -1;
		}

		sys_update();
		if(uart2.poll()) {
			char c = (char) uart2.getchar();
			echo[i ++] = c;
			if(c == '\r')
				break;
		}
	}

	if(echo[0] == 'E') {
		sys_error("sgm response invalid(%c, cmd = %s)", echo[0], cmd);
		return -1;
	}
	return 0;
}

static int sgm_init(void)
{
	int e0 = __sgm_simple_cmd("T\r"); //change to track mode
	int e1 = __sgm_simple_cmd("K\r"); //change the unit to kgf(gf)
	return (e0 || e1) ? -1 : 0;
}

static int sgm_reset_to_zero(void)
{
	return __sgm_simple_cmd("Z\r");
}

static int sgm_read(float *gf)
{
	__sgm_init();
	uart_puts(&uart2, "D\r");

	int i = 0;
	char echo[32];
	time_t deadline = time_get(10);
	while(1) {
		if(time_left(deadline) < 0) {
			sys_error("sgm read response timeout");
			return -1;
		}

		sys_update();
		if(uart2.poll()) {
			char c = (char) uart2.getchar();
			echo[i ++] = c;
			if(c == '\r')
				break;
		}
	}
	echo[i] = 0;

	int n = sscanf(echo, "%f", gf);
	if(n != 1) {
		sys_error("sgm read response error");
		return -1;
	}
	return 0;
}

/*ms: stable time, function will not return until sgm display
keep its value more than ms*/
int sgm_read_until_stable(float *gf, int ms)
{
	#define SGM_N 50
	float m[SGM_N];

	int i = 0, n = 0;
	while(1) {
		if(sgm_read(m + i)) return -1;
		sys_mdelay((ms + ms)/ SGM_N );

		i ++;
		i = (i == SGM_N) ? 0 : i;

		n ++;
		if(n >= SGM_N) {
			int eq, j;
			float v = m[SGM_N-1];
			for(eq = j = 0; j < SGM_N; j ++) {
				if(v == m[j]) eq ++;
			}
			if(eq >= (int)(SGM_N * 0.5)) {
				*gf = v;
				return 0;
			}
		}
	}
}

static void sgm_console(void)
{
	//sgm uart init
#ifdef CONFIG_BRD_HWV20 //fuck JJ!!!
	GPIO_WriteBit(GPIOE, GPIO_Pin_1, Bit_RESET); //USART2_S0 = 1
	GPIO_WriteBit(GPIOE, GPIO_Pin_0, Bit_SET); //USART2_S1 = 1
#else
	GPIO_WriteBit(GPIOE, GPIO_Pin_0, Bit_SET); //USART2_S0 = 1
	GPIO_WriteBit(GPIOE, GPIO_Pin_1, Bit_SET); //USART2_S1 = 1
#endif

	uart_cfg_t cfg = {
		.baud = 19200,
	};
	uart2.init(&cfg);
	uart_flush(&uart2);

	const char *usage = {
		"T		change to the track mode\n"
		"P		change to peak mode\n"
		"Z		reset the reading to zero\n"
		"D		read, return [value][unit][mode]\n"
		"K		change unit to kgf(gf)\n"
		"N		change unit to N\n"
		"O		change unit to lbf(ozf)\n"
		"EHHHHLLLL	change Hi&Lo comparator threshold\n"
		"E		read threshold settings\n"
		"Q		quit sgm console\n"
	};

	//printf("%s", usage);
	char *cmdline = sys_malloc(128);
	while(1) {
		if(shell_ReadLine("SGM# ", cmdline)) {
			if((cmdline[0] == 'Q') || (cmdline[0] == 'q')) {
				break;
			}
			else if((cmdline[0] == '?') || (cmdline[0] =='h')) {
				printf("%s", usage);
			}
			else if(cmdline[0] != 0) {
				uart_puts(&uart2, cmdline);
				uart2.putchar('\r');
			}
		}

		if(uart2.poll()) {
			char c = uart2.getchar();
			putchar(c);
			if(c == '\r') putchar('\n');
		}
	}
	sys_free(cmdline);
}

static int cmd_sgm_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"sgm -r		sgm read\n"
		"sgm -g		enable sgm debug console, type 'Q' to break\n"
	};

	float gf;
	int e = 1;
	if(argc > 1) {
		e = 0;
		for(int i = 1; i < argc; i ++) {
			e += (argv[i][0] != '-');
			switch(argv[i][1]) {
			case 'g':
				sgm_console();
				break;
			case 'r':
				sgm_read(&gf);
				printf("%f gf\n", gf);
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

const cmd_t cmd_sgm = {"sgm", cmd_sgm_func, "ybs monitor strain gauge meter ctrl"};
DECLARE_SHELL_CMD(cmd_sgm)

/*for initially positioning purpose, this routine must be called first*/
int mov_i(int distance, float gf_max)
{
	sys_assert((distance > 0) && (gf_max > 0.1) && (gf_max < 51.0));
	ybs_gf_max = gf_max;

	if(sgm_init()) return -1;

	/*it's dangerous!!! be careful ..*/
	ybs_steps = 0;
	ybs_steps_min = -50000;
	ybs_steps_max = 50000;

	//to find a rough touch point
	float gf, gf0;
	if(sgm_read(&gf0)) goto MOV_I_FAIL;
	while(1) {
		if(mov_n(20)) goto MOV_I_FAIL;
		if(sgm_read(&gf)) goto MOV_I_FAIL;
		if(gf - gf0 > 0.5) {
			ybs_steps = 0;
			break;
		}
	}

	//reset sgm
	if(mov_n(-1000)) goto MOV_I_FAIL;
	sys_mdelay(3000);
	if(sgm_reset_to_zero()) goto MOV_I_FAIL;
	if(mov_n(1000)) goto MOV_I_FAIL;

	//find a more precise zero point
	if(mov_f(0, 100)) goto MOV_I_FAIL;
	ybs_steps = 0;
	ybs_steps_min = -distance;

	if(mov_f(gf_max, 100)) goto MOV_I_FAIL;
	ybs_steps_max = ybs_steps;

	if(mov_p(-500)) goto MOV_I_FAIL;
	//if(mov_p(ybs_steps_min)) goto MOV_I_FAIL;
	return 0;

MOV_I_FAIL:
	ybs_steps = -1;
	ybs_steps_min = -1;
	ybs_steps_max = -1;
	return -1;
}

/*fast move forwrd/backward steps, with each step ms*/
int mov_n(int steps)
{
	if((ybs_steps_min == -1) || (ybs_steps_max == -1)) {
		sys_error("please try 'mov -i' first");
		return -1;
	}

	int target_pos = ybs_steps + steps;
	if((target_pos > ybs_steps_max) || (target_pos < ybs_steps_min)) {
		sys_error("target position over-range(%d ? [%d, %d])", target_pos, ybs_steps_min, ybs_steps_max);
		return -1;
	}

	//change direction?
	static int ybs_mov_dir = 0;
	int dir = (steps > 0) ? 1 : -1;
	if(dir != ybs_mov_dir) {
		ybs_mov_dir = dir;
		GPIO_WriteBit(GPIOA, GPIO_Pin_1, (ybs_mov_dir > 0) ? Bit_SET : Bit_RESET);
		for(int pw = ybs_motor_pusle; pw > 0; pw --);
	}

	//mov
	while(steps != 0) {
		GPIO_WriteBit(GPIOA, GPIO_Pin_0, Bit_SET);
		for(int pw = ybs_motor_pusle; pw > 0; pw --);
		GPIO_WriteBit(GPIOA, GPIO_Pin_0, Bit_RESET);
		for(int pw = ybs_motor_pusle; pw > 0; pw --);
		steps -= ybs_mov_dir;
		ybs_steps += ybs_mov_dir;
	}
	return 0;
}

int mov_p(int pos)
{
	//printf("INFO: %s(%d) ...\n", __FUNCTION__, pos);
	int steps = pos - ybs_steps;
	return mov_n(steps);
}

int mov_p_measure(int pos, float *gf_sgm, float *gf_ybs)
{
	int i, N = 4;
	float gf, sgm_l, ybs_l, sgm_r, ybs_r;
	for(int i = 0; i < N; i ++) {
		//dir = left
		if(mov_p(pos - 50)) return -1;
		sys_mdelay(20);
		if(mov_p(pos + 1)) return -1;
		sys_mdelay(50);
		if(mov_p(pos)) return -1;

		//sys_mdelay(500);
		//if(sgm_read(&gf)) return -1;
		if(sgm_read_until_stable(&gf, 200)) return -1;
		sgm_l = (sgm_l * i + gf) / (i + 1);
		if(ybs_read(&gf)) return -1;
		ybs_l = (ybs_l * i + gf) / (i + 1);

		//dir = right
		if(mov_p(pos + 50)) return -1;
		sys_mdelay(20);
		if(mov_p(pos - 1)) return -1;
		sys_mdelay(50);
		if(mov_p(pos)) return -1;

		//sys_mdelay(500);
		//if(sgm_read(&gf)) return -1;
		if(sgm_read_until_stable(&gf, 200)) return -1;
		sgm_r = (sgm_r * i + gf) / (i + 1);
		if(ybs_read(&gf)) return -1;
		ybs_r = (ybs_r * i + gf) / (i + 1);
	}

	if(gf_sgm != NULL) *gf_sgm = (sgm_l + sgm_r) / 2;
	if(gf_ybs != NULL) *gf_ybs = (ybs_l + ybs_r) / 2;
	return 0;
}

int mov_f(float target_gf, int gf_settling_ms)
{
	//printf("INFO: %s(%.3fgf, %dms) ...\n", __FUNCTION__, target_gf, gf_settling_ms);
	gf_settling_ms = (gf_settling_ms == 0) ? 1000 : gf_settling_ms;
	if((target_gf < 0) || (target_gf > ybs_gf_max)) {
		sys_error("target %fgf over-range[0, %f]", target_gf, ybs_gf_max);
		return -1;
	}

	float gf;
	if(sgm_read(&gf)) return -1;
	if(gf <= 0) { //fast move to nearby zero point first
		if(mov_p(100)) return -1;
		if(sgm_read_until_stable(&gf, 100)) return -1;
		if(gf <= 0) {
			sys_error("slidling bench zero point error\n");
			return -1;
		}
	}

	for(int micro_steps = 0; micro_steps < 15; ) {
		if(sgm_read(&gf)) return -1;

		float delta = target_gf - gf;
		delta = (delta > 0) ? delta : -delta;
		if(delta < 0.5) { //slow down the move speed
			micro_steps ++;
			if(sgm_read_until_stable(&gf, gf_settling_ms)) return -1;
			printf("gf = %.1f\n", gf);

			delta = target_gf - gf;
			delta = (delta > 0) ? delta : - delta;
			if(delta < 0.1) {
				printf("mov success, ybs_steps = %d\n", ybs_steps);
				return 0;
			}
		}

		int steps = (target_gf > gf) ? 1 : -1;
		if(mov_n(steps)) return -1;
	}

	sys_error("motor adjust time too long, pls check fixture");
	return -1;
}

static int mov_d(void)
{
	static const int p[] = {100, 200, 300, 400, 500, 600, 700, 800};
	float gf;
	#define method 0
#if method == 0
	for(int n = 0; n < 5; n ++) {
		for(int i = 0; i < 8; i ++) {
			mov_p(p[i]);
			sgm_read_until_stable(&gf, 1000);
			printf("%d steps => %.3f gf\n", p[i], gf);
		}
		mov_n(100);
		for(int i = 7; i >= 0; i --) {
			mov_p(p[i]);
			sgm_read_until_stable(&gf, 1000);
			printf("%d steps => %.3f gf\n", p[i], gf);
		}
		mov_n(-100);
	}
#endif
#if method == 1
	for(int n = 0; n < 5; n ++) {
		for(int i = 0; i < 8; i ++) {
			mov_p(0);
			mov_p(p[i]);
			sgm_read_until_stable(&gf, 1000);
			printf("%d steps => %.3f gf\n", p[i], gf);
		}
	}
#endif
#if method == 2
	for(int n = 0; n < 5; n ++) {
		for(int i = 0; i < 8; i ++) {
			mov_p(850);
			mov_p(p[i]);
			sgm_read_until_stable(&gf, 1000);
			printf("%d steps => %.3f gf\n", p[i], gf);
		}
	}
#endif
#if method == 3
	for(int n = 0; n < 5; n ++) {
		for(int i = 0; i < 8; i ++) {
			mov_p(p[i]);
			mov_p(450);
			sgm_read_until_stable(&gf, 1000);
			printf("%d steps => %.3f gf\n", p[i], gf);
		}
	}
#endif
#if method == 4
	for(int n = 0; n < 5; n ++) {
		for(int i = 0; i < 8; i ++) {
			mov_p(p[i]);
			mov_p(450);
			sys_mdelay(3000);
			gf = 0;
			for(int k = 0; k < 5; k ++) {
				float v = 0;
				ybs_read(&v);
				gf = (gf * k + v)/ (k + 1);
				sys_mdelay(100);
			}
			printf("%d steps => %.3f gf\n", p[i], gf);
		}
	}
#endif
#if method == 5
	for(int n = 0; n < 7; n ++) {
		for(int i = 0; i < 8; i ++) {
			mov_p(p[i]);
			mov_p(p[n] + 50);
			float sgm;
			sgm_read_until_stable(&sgm, 1000);
			gf = 0;
			for(int k = 0; k < 5; k ++) {
				float v = 0;
				ybs_read(&v);
				gf = (gf * k + v)/ (k + 1);
				sys_mdelay(100);
			}
			printf("%d steps: sgm = %.3f ybs = %.3f\n", p[i], sgm, gf);
		}
	}
#endif
#if method == 6
	for(int i = 0; i < 5; i ++) {
	if(mov_p(0)) return -1;
	for(int pos = 50; pos < 800; pos += 50) {
		mov_p(pos);
		float sgm;
		sgm_read_until_stable(&sgm, 1000);
		gf = 0;
		for(int k = 0; k < 100; k ++) {
			float v = 0;
			ybs_read(&v);
			gf = (gf * k + v)/ (k + 1);
			sys_mdelay(20);
		}
		printf("%d steps: sgm = %.3f ybs = %.3f\n", pos, sgm, gf);
	}
	}
#endif
#if method == 7
	int pmax = 350, step = 10;
	int i, j, n, N = pmax / step;
	float *sgm = sys_malloc(N * sizeof(float));
	float *ybs = sys_malloc(N * sizeof(float));

	memset(sgm, 0x00, N*sizeof(float));
	memset(ybs, 0x00, N*sizeof(float));

	for(i = 0; i < 6; i ++) {
		mov_p(0);

		//forward
		for(n = 0; n < N; n ++) {
			int p = step * (n + 1);
			mov_p(p);
			float gf, v;

			//measure sgm value
			sgm_read_until_stable(&gf, 1000);
			printf("%d steps: sgm = %.3f ", p, gf);
			sgm[n] = (sgm[n] * i + gf) / (i + 1);

			//measure ybs value
			for(gf = 0, j = 0; j < 100; j ++) {
				ybs_read(&v);
				gf = (gf * j + v) / (j + 1);
				sys_mdelay(20);
			}
			printf("ybs = %.3f \n", gf);
			ybs[n] = (ybs[n] * i + gf) / (i + 1);
		}

		i ++;
		mov_n(50);
		mov_n(-50);
		//backward
		for(n = N - 1; n >= 0; n --) {
			int p = step * (n + 1);
			mov_p(p);
			float gf, v;

			//measure sgm value
			sgm_read_until_stable(&gf, 100);
			printf("%d steps: sgm = %.3f ", p, gf);
			sgm[n] = (sgm[n] * i + gf) / (i + 1);

			//measure ybs value
			for(gf = 0, j = 0; j < 100; j ++) {
				ybs_read(&v);
				gf = (gf * j + v) / (j + 1);
				sys_mdelay(20);
			}
			printf("ybs = %.3f \n", gf);
			ybs[n] = (ybs[n] * i + gf) / (i + 1);
		}
	}
	for(n = 0; n < N; n ++) {
		int p = step * (n + 1);
		printf("%d steps: sgm = %.3f ybs = %.3f\n", p, sgm[n], ybs[n]);
	}
#endif
	return 0;
}

static int cmd_mov_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"mov -n steps			fast move forward/backward\n"
		"mov -f gf [ms]			precise movement until force = gf, settling time = ms\n"
		"mov -p [pos]			to move to and fetch current ybs_steps\n"
		"mov -i [steps] [gf_max]	to do initially positioning\n"
		"mov -d				sliding bench self test\n"
		"mov -m [pos]			to move to and fetch current gf\n"
	};

	int steps, ms, e = -1;
	float gf;

	if(argc > 1) {
		e = 0;
		for(int i = 1; i < argc; i ++) {
			e += (argv[i][0] != '-');
			switch(argv[i][1]) {
			case 'n':
				e = -1;
				if((i + 1 < argc) && (sscanf(argv[i + 1], "%d", &steps) == 1)) {
					mov_n(steps);
					i ++;
					e = 0;
				}
				break;
			case 'f': //mov -f gf [ms]
				e = -1; ms = 1000;
				if((i + 1 < argc) && (sscanf(argv[i + 1], "%f", &gf) == 1)) {
					i ++;
					e = 0;
				}
				if((i + 1 < argc) && (sscanf(argv[i + 1], "%d", &ms) == 1)) {
					i ++;
				}
				if(e == 0) {
					mov_f(gf, ms);
				}
				break;
			case 'i': //mov -i steps gf_max
				steps = 8000; gf = 45.0;
				if((i + 1 < argc) && (sscanf(argv[i + 1], "%d", &steps) == 1)) {
					i ++;
				}
				if((i + 1 < argc) && (sscanf(argv[i + 1], "%f", &gf) == 1)) {
					i ++;
				}
				mov_i(steps, gf);
				break;
			case 'm':
			case 'p': //mov -p [pos]
				if((i + 1 < argc) && (sscanf(argv[i + 1], "%d", &steps) == 1)) {
					if(argv[i][1] == 'p') {
						mov_p(steps);
						printf("current position: %d\n", ybs_steps);
					}
					else {
						float sgm, ybs;
						mov_p_measure(steps, &sgm, &ybs);
						printf("sgm,ybs= %.1f %.1f gf\n", sgm, ybs);
					}
					i ++;
				}
				break;
			case 'd':
				mov_d();
				break;
			case 'h':
				printf("%s", usage);
				break;
			default:
				e ++;
			}
		}
	}

	if(e) printf("%s", usage);
	return 0;
}

const cmd_t cmd_mov = {"mov", cmd_mov_func, "ybs monitor motor control"};
DECLARE_SHELL_CMD(cmd_mov)
