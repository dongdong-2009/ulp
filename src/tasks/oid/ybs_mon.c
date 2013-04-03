/*
 * 	miaofng@2013-3-20 initial version
 *	drivers for ybs monitor v1.0(aduc7061 based)
 *	board designed by xiaoming.xu&mingjing.ding
 *
 */

#include "ulp/sys.h"
#include "shell/cmd.h"
#include "spi.h"
#include "common/vchip.h"
#include "oid_mcd.h"
#include <string.h>
#include "led.h"
#include "common/debounce.h"
#include "stm32f10x.h"
#include "uart.h"
#include "shell/cmd.h"
#include "shell/shell.h"
#include "nvm.h"
#include "ybs_mon.h"

#define CONFIG_MOV_FAST 1
#define ybs_motor_pusle 3000
static int ybs_mgf_max __nvm;
static int ybs_steps = -1;

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
	GPIO_WriteBit(GPIOE, GPIO_Pin_0, Bit_SET); //USART2_S0 = 1
	GPIO_WriteBit(GPIOE, GPIO_Pin_1, Bit_SET); //USART2_S1 = 1

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

int sgm_init(void)
{
	int e0 = __sgm_simple_cmd("T\r"); //change to track mode
	int e1 = __sgm_simple_cmd("K\r"); //change the unit to kgf(gf)
	return (e0 || e1) ? -1 : 0;
}

int sgm_reset_to_zero(void)
{
	return __sgm_simple_cmd("Z\r");
}

static int sgm_read(int *mgf)
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

	//convert the data from float string to int
	float gf;
	int n = sscanf(echo, "%f", &gf);
	if(n != 1) {
		sys_error("sgm read response error");
		return -1;
	}

	gf = (gf > 0) ? gf : -gf;
	*mgf = (int) (gf * 1000);
	return 0;
}

int sgm_read_until_stable(int *mgf, int ms)
{
	#define SGM_N 5
	int m[SGM_N];
	time_t deadline = time_get(ms);

	int i = 0, n = 0;
	while(1) {
		if(time_left(deadline) < 0) {
			sys_error("SGM display unstable, stable time too long(> %dmS)", ms);
			return -1;
		}

		if(sgm_read(m + i)) return -1;
		sys_mdelay(20);

		i ++;
		i = (i == SGM_N) ? 0 : i;

		n ++;
		if(n >= SGM_N) {
			int j, v = m[0];
			for(j = 0; j < SGM_N; j ++) {
				if(v != m[j]) break;
			}
			if(j == SGM_N) {
				*mgf = v;
				return 0;
			}
		}
	}
}

static void sgm_console(void)
{
	//sgm uart init
	GPIO_WriteBit(GPIOE, GPIO_Pin_0, Bit_SET); //USART2_S0 = 1
	GPIO_WriteBit(GPIOE, GPIO_Pin_1, Bit_SET); //USART2_S1 = 1

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

	int mgf, e = 1;
	if(argc > 1) {
		e = 0;
		for(int i = 1; i < argc; i ++) {
			e += (argv[i][0] != '-');
			switch(argv[i][1]) {
			case 'g':
				sgm_console();
				break;
			case 'r':
				sgm_read(&mgf);
				printf("%d\n", mgf);
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

static int change_direction(int forward)
{
	if(ybs_steps < 0) {
		sys_error("motor system not init, pls do 'mov -i' first");
		return -1;
	}

	GPIO_WriteBit(GPIOA, GPIO_Pin_1, (forward > 0) ? Bit_SET : Bit_RESET);
	for(int pw = ybs_motor_pusle; pw > 0; pw --);
	return 0;
}

static int move_steps(int n)
{
	if(ybs_steps < 0) {
		sys_error("motor system not init, pls do 'mov -i' first");
		return -1;
	}

	n = (n > 0) ? n : - n;
	while(n > 0) {
		GPIO_WriteBit(GPIOA, GPIO_Pin_0, Bit_SET);
		for(int pw = ybs_motor_pusle; pw > 0; pw --);
		GPIO_WriteBit(GPIOA, GPIO_Pin_0, Bit_RESET);
		for(int pw = ybs_motor_pusle; pw > 0; pw --);
		n --;
	}
	return 0;
}

/*fast movement, until steps reached or mgf bigger than mgf_max*/
int mov_n(int steps, int mgf_max)
{
	int mgf;

	//change direction
	int n = (steps > 0) ? steps : - steps;
	int d = (steps > 0) ? 1 : -1;
	mgf_max = (mgf_max == 0) ? 1000 : mgf_max;

	if(change_direction(d)) return -1;
	if(sgm_read(&mgf)) return -1;
	if((mgf >= mgf_max) && (d == 1)) {
		printf("INFO: %s mgf limit reached(%d > %d)\n", __FUNCTION__, mgf, mgf_max);
		return 0;
	}

	for(int i = 0; i < n; i ++) {
		if(d < 0) { //backward, position limit reached?
			if(ybs_steps + d < 0) {
				printf("INFO: %s backward to position limit(ybs_steps = %d)\n", __FUNCTION__, ybs_steps);
				return 0;
			}
		}
		else { //forward to mgf limit?
			if(i % 100 == 0) {
				if(sgm_read(&mgf)) return -1;
				else if(mgf > mgf_max) {
					printf("INFO: %s forward to mgf limit(%d > %d)\n", __FUNCTION__, mgf, mgf_max);
					return 0;
				}
			}
		}

		/*move a step*/
		if(move_steps(1)) return -1;
		ybs_steps += d;
	}
	return 0;
}

/* measure sgm between each step(about 20ms), algo be used to find sgm-ybs contact point(relative zero point):
1, mov -n 50000 //to ensure sgm-ybs in contacting state
2, mov -f 1000 //mov to rough 1gf position
3, mov -f 0 //move back step-by-step to find the zero point precisely.
*/
int mov_f(int target, int ms_per_step)
{
	int d, mgf, delta;
	sys_assert((target >= 0) && (target < ybs_mgf_max));

	printf("INFO: %s(%d) ...\n", __FUNCTION__, target);
	if(sgm_read(&mgf)) return -1;
	if(mgf == 0) { //fast move to touch point
		if(mov_n(50000, 0)) return -1;
	}

	int micro_steps = 0;
	while(1) {
		if(sgm_read(&mgf)) return -1;
		sys_mdelay(ms_per_step);

		delta = mgf - target;
		delta = (delta > 0) ? delta : -delta;
		if(delta < 800) {
#if CONFIG_MOV_FAST
			if(sgm_read_until_stable(&mgf, 1000)) return -1;
#else
			//precise measurement mode
			int ms = (ms_per_step == 0) ? 1000 : ms_per_step;
			sys_mdelay(ms);

			int mgf0, mgf1, mgf2;
			if(sgm_read(&mgf0)) return -1;
			sys_mdelay(10);
			if(sgm_read(&mgf1)) return -1;
			sys_mdelay(10);
			if(sgm_read(&mgf2)) return -1;
			sys_mdelay(10);
			mgf = (mgf0 + mgf1 + mgf2) / 3;
#endif
			delta = mgf - target;
			delta = (delta > 0) ? delta : - delta;
			printf("mgf = %d\n", mgf);
			if(delta == 0) {
				printf("mov success, ybs_steps = %d\n", ybs_steps);
				return 0;
			}

			micro_steps ++;
			if(micro_steps > 30) {
				sys_error("motor adjust time too long, pls check fixture");
				return -1;
			}
		}

		//change direction
		d = (target < mgf) ? -1 : 1;
		if(change_direction(d)) return -1;

		/*move a step*/
		if(move_steps(1)) return -1;
		ybs_steps += d;
	}
}

int mov_p(int pos)
{
	printf("INFO: %s(%d) ...\n", __FUNCTION__, pos);
	int steps = pos - ybs_steps;
	return mov_n(steps, 0);
}

/*for initially positioning purpose*/
void mov_i(int distance)
{
	distance = (distance > 0) ? distance : 5000;
	ybs_steps = 20000;

	//find relative zero point
	mov_f(0, 0);

	//backward distance steps as start point(abs zero point)
	mov_n(- distance, 0);
	ybs_steps = 0;
}

static int cmd_mov_func(int argc, char *argv[])
{
	const char *usage = {
		"usage: (note: return cur pos or -1 when error)\n"
		"mov -n	steps [mgf_max]	fast forward/back, Tstep = ms, default mgf_max = 1000\n"
		"mov -f	mgf [ms]	precise movement until force = mgf, Tstep = ms\n"
		"mov -p			to fetch ybs_steps\n"
		"mov -l [mgf_max]	set/get limit of mgf_max\n"
		"mov -i	[steps]		to do initially positioning\n"
	};

	int mgf, ms, steps, pos, e = 1;
	if(argc > 1) {
		e = 0;
		for(int i = 1; i < argc; i ++) {
			e += (argv[i][0] != '-');
			switch(argv[i][1]) {
			case 'n':
				steps = 1; mgf = 0;
				if((i + 1 < argc) && (sscanf(argv[i + 1], "%d", &steps) == 1)) {
					i ++;
				}
				if((i + 1 < argc) && (sscanf(argv[i + 1], "%d", &mgf) == 1)) {
					i ++;
				}
				mov_n(steps, mgf);
				break;
			case 'f':
				mgf = 1; ms = 0;
				if((i + 1 < argc) && (sscanf(argv[i + 1], "%d", &mgf) == 1)) {
					i ++;
				}
				if((i + 1 < argc) && (sscanf(argv[i + 1], "%d", &ms) == 1)) {
					i ++;
				}
				mov_f(mgf, ms);
				break;
			case 'i':
				steps = 0;
				if((i + 1 < argc) && (sscanf(argv[i + 1], "%d", &steps) == 1)) {
					i ++;
				}
				mov_i(steps);
				break;
			case 'p':
				if((i + 1 < argc) && (sscanf(argv[i + 1], "%d", &pos) == 1)) {
					i ++;
					mov_p(pos);
				}
				printf("current position: %d\n", ybs_steps);
				break;
			case 'l':
				if((i + 1 < argc) && (sscanf(argv[i + 1], "%d", &mgf) == 1)) {
					i ++;
					ybs_mgf_max = mgf;
					nvm_save();
				}
				printf("%d\n", ybs_mgf_max);
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
