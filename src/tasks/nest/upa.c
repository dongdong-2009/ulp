/* upa for nest, design rules:
 * 1) host <> upa <> nest(s)
 * 2) cmds = "upa list" + "upa sel xxx" + ... + "upa exit"
 * 3) bad nest handling =
 * 	miaofng@2012 initial version
 */
#include "config.h"
#include "ulp/debug.h"
#include "ulp_time.h"
#include "ulp/wl.h"
#include "spi.h"
#include "sys/task.h"
#include "shell/cmd.h"
#include "shell/shell.h"
#include "linux/list.h"
#include "lib/nest_wl.h"
#include <string.h>
#include "sys/malloc.h"
#include "uart.h"

struct upa_nest_s {
	unsigned addr;
	time_t timer; //update timer
	struct list_head list;
};

static int upa_wl_fd;
static struct console_s *upa_wl_cnsl;
#define upa_uart_cnsl ((struct console_s *) &uart2)
static LIST_HEAD(upa_nest_queue);

static union {
	struct {
		unsigned update_off : 1;
		unsigned shutup_ok : 1;
		unsigned wakeup_ok : 1;
	};
	unsigned value;
} upa_flag;

static time_t upa_mon_timer; /*to avoid chat with monitor in every loop*/
static struct upa_nest_s *upa_nest_mon; //monitor nest
static struct upa_nest_s *upa_nest_sel; //selected nest
static struct upa_nest_s *upa_nest_cur; //current access nest

/*not supported yet*/
static void upa_rssi_init(struct upa_nest_s *nest)
{
}

/*enable wl device rssi function*/
static void upa_rssi_enable(const struct upa_nest_s *nest)
{
}

/*return rssi value 0 - 99(best)*/
static int upa_rssi_get(const struct upa_nest_s *nest)
{
	return 0;
}

static struct upa_nest_s* upa_mon_elect(void)
{
	struct upa_nest_s *nest, *hero = NULL;
	struct list_head *pos;
	int rssi, max = -1;

	//to find the nest which has the biggest rssi
	list_for_each(pos, &upa_nest_queue) {
		nest = list_entry(pos, upa_nest_s, list);
		rssi = upa_rssi_get(nest);
		if(rssi > max) {
			hero = nest;
			max = rssi;
		}
	}

	if(upa_nest_mon != NULL) {
		rssi = upa_rssi_get(upa_nest_mon);
		if(max - rssi < 10) { //rssi may has 10% deviation
			hero = upa_nest_mon;
		}
	}

	return hero;
}

/* get nest through its address */
static struct upa_nest_s* upa_nest_get(unsigned addr)
{
	struct upa_nest_s *nest, *found = NULL;
	struct list_head *pos;

	list_for_each(pos, &upa_nest_queue) {
		nest = list_entry(pos, upa_nest_s, list);
		if(nest->addr == addr) {
			found = nest;
			break;
		}
	}
	return found;
}

/* a new nest is born */
static int upa_nest_new(unsigned addr)
{
	struct upa_nest_s *nest = NULL;

	//search the nest in the list to avoid duplication
	nest = upa_nest_get(addr);
	if(nest != NULL)
		return 0; //already exist

	//create it
	nest = sys_malloc(sizeof(struct upa_nest_s));
	if(nest != NULL) {
		nest->addr = addr;
		nest->timer = 0;
		upa_rssi_init(nest);
		list_add_tail(&nest->list, &upa_nest_queue);
		return 0;
	}

	return -1; //no enough memory
}

/* bad boy, neglect it ... */
static int upa_nest_bad(unsigned addr)
{
	struct upa_nest_s *nest = upa_nest_get(addr);
	if(upa_nest_mon == nest) {
		//change it, too disappointed ...
		upa_nest_mon = NULL;
	}

	//remove it? yes!
	return 0;
}

/*there is no active nest on the queue ?*/
int upa_nest_empty(void)
{
	return list_empty(&upa_nest_queue);
}

/* redirect printf output to current active nest
 * !!!do not write a string longer than 128 bytes
 */
int upa_nest_printf(const char *fmt, ...)
{
	va_list args;
	time_t timer = time_get(NEST_WL_MS/10);
	char str[128];
	int n;

	//target?
	if(upa_nest_cur == NULL) {
		return -1;
	}

	//output to a string buf
	va_start(args, fmt);
	n = vsnprintf(str, 128, fmt, args);
	va_end(args);

	//ensure there's enough space in tbuf
	while(dev_poll(upa_wl_fd, POLLOBUF) < n) {
		task_Update();
		if(time_left(timer) < 0) {
			return -1;
		}
	}

	//write to tbuf
	dev_write(upa_wl_fd, str, n);
	return 0;
}

int upa_nest_switch(struct upa_nest_s *nest)
{
	int ecode = 0;
	time_t timer;

	if(upa_nest_cur == nest)
		return 0;

	//close cur nest first
	timer = time_get(NEST_WL_MS/10);
	if(upa_nest_cur != NULL) {
		upa_flag.shutup_ok = 0;
		ecode = upa_nest_printf("monitor shutup\r");
		if(ecode) {
			return ecode; //send timeout
		}

		//wait for cmd "upa shutup\r" sent by target nest
		while(upa_flag.shutup_ok == 0) {
			task_Update();
			if(time_left(timer) < 0) {
				//wait for nest response timeout
				dev_ioctl(upa_wl_fd, WL_FLUSH);
				upa_nest_bad(upa_nest_cur->addr);
				upa_nest_cur = NULL;
				return -1;
			}
		}

		dev_ioctl(upa_wl_fd, WL_FLUSH);
		upa_nest_cur = NULL;
	}

	//switch to new one
	timer = time_get(NEST_WL_MS/10);
	if(nest != NULL) {
		dev_ioctl(upa_wl_fd, WL_STOP);
		dev_ioctl(upa_wl_fd, WL_SET_MODE, WL_MODE_PTX);
		dev_ioctl(upa_wl_fd, WL_SET_ADDR, nest->addr);
		dev_ioctl(upa_wl_fd, WL_START);
		upa_rssi_enable(nest);

		//send cmd "monitor wakeup\r"
		upa_flag.wakeup_ok = 0;
		ecode = upa_nest_printf("monitor wakeup\r");
		if(ecode) {
			return ecode; //send timeout
		}

		//wait for cmd "upa wakeup\r" sent by target nest
		while(upa_flag.wakeup_ok == 0) {
			task_Update();
			if(time_left(timer) < 0) {
				//wait for nest response timeout
				dev_ioctl(upa_wl_fd, WL_FLUSH);
				upa_nest_bad(nest->addr);
				return -1;
			}
		}
		upa_nest_cur = nest;
	}
	return 0;
}

static int upa_wl_onfail(int ecode, ...)
{
	unsigned addr;
	va_list args;

	va_start(args, ecode);
	switch(ecode) {
	case WL_ERR_TX_TIMEOUT:
		break;
	default:
		break;
	}
	va_end(args);

	console_set(upa_uart_cnsl);
	printf("ecode = %d\n", ecode);
	console_restore();

	return 0;
}

static int upa_Init(void)
{
	static const struct nrf_cfg_s nrf_cfg = {
		.spi = &spi1,
		.gpio_cs = SPI_1_NSS,
		.gpio_ce = SPI_CS_PC5,
	};

	dev_register("nrf", &nrf_cfg);
	upa_wl_fd = dev_open("wl0", 0);
	if(upa_wl_fd == 0) {
		printf("init: device wl0 open failed!!!\n");
		assert(0);
	}
	upa_wl_cnsl = console_register(upa_wl_fd);
	assert(upa_wl_cnsl != NULL);
	shell_register(upa_wl_cnsl);
	shell_mute(upa_wl_cnsl, 1);
	shell_prompt(upa_uart_cnsl, "upa# ");
	dev_ioctl(upa_wl_fd, WL_FLUSH);
	upa_nest_mon = NULL;
	upa_nest_sel = NULL;
	upa_nest_cur = NULL;
	upa_flag.value = 0;
	return 0;
}

void main(void)
{
	task_Init();
	upa_Init();
	printf("Power Conditioning - UPA\n");
	printf("IAR C Version v%x.%x, Compile Date: %s,%s\n", (__VER__ >> 24),((__VER__ >> 12) & 0xfff),  __TIME__, __DATE__);

	struct upa_nest_s *nest;
	struct list_head *pos;

	while(1){
		//1, switch to prx mode
		shell_lock(upa_uart_cnsl, 0);
		shell_lock(upa_wl_cnsl, 0);
		dev_ioctl(upa_wl_fd, WL_STOP);
		dev_ioctl(upa_wl_fd, WL_SET_MODE, WL_MODE_PRX);
		dev_ioctl(upa_wl_fd, WL_SET_ADDR, NEST_WL_ADDR);
		dev_ioctl(upa_wl_fd, WL_START);
		while(upa_nest_empty() || upa_flag.update_off) {
			task_Update();
		}

		//2, normal work mode
		while(!upa_nest_empty() && (upa_flag.update_off == 0)) {
			//2-1, handle  monitor
			shell_lock(upa_uart_cnsl, 1);
			shell_lock(upa_wl_cnsl, 0);
			if(upa_nest_mon != NULL) {
				//2-1-1, fetch newbie info
				upa_nest_switch(upa_nest_mon);
				upa_nest_printf("monitor get\r");
			}

			//2-1-2, elect a new hero??
			nest = upa_nest_mon;
			if((nest == NULL) || (time_left(upa_mon_timer) < 0)) {
				nest = upa_mon_elect();
				upa_mon_timer = time_get(NEST_WL_MS/2);
			}

			if(nest != upa_nest_mon) {
				//2-1-2-1, dismiss old monitor
				if(upa_nest_mon != NULL) {
					upa_nest_switch(upa_nest_mon);
					upa_nest_printf("monitor clr\r");
				}

				//2-1-2-2, new monitor takes office
				upa_nest_mon = nest;
				upa_nest_switch(nest);
				upa_nest_printf("monitor set\r");
			}

			//2-2: update each nest
			list_for_each(pos, &upa_nest_queue) {
				nest = list_entry(pos, upa_nest_s, list);
				if(time_left(nest->timer) < 0) {
					nest->timer = time_get(NEST_WL_MS/2);
					shell_lock(upa_uart_cnsl, 1);
					shell_lock(upa_wl_cnsl, 0);
					upa_nest_switch(nest);
					upa_nest_printf("monitor ping\r");
				}
			}

			//2-3: a nest is selected?
			if(upa_nest_sel != NULL) {
				upa_nest_switch(upa_nest_sel);
			}
			shell_lock(upa_wl_cnsl, 0);
			shell_lock(upa_uart_cnsl, 0);
			task_Update();
		}
	}
} // main

/*upa cmd handling*/
static int cmd_upa_func(int argc, char *argv[]);
static int cmd_monitor_func(int argc, char *argv[]);

const static cmd_t cmd_upa = {"upa", cmd_upa_func, "universal wireless program adapter for nest"};
DECLARE_SHELL_CMD(cmd_upa)
const static cmd_t cmd_monitor = {"monitor", cmd_monitor_func, "nest wireless monitor cmds"};
DECLARE_SHELL_CMD(cmd_monitor)

static int cmd_upa_func(int argc, char *argv[])
{
	struct upa_nest_s *nest = NULL;
	struct list_head *pos;
	unsigned addr;
	const char *usage = {
		"upa select 1234567a		select current nest\n"
		"upa exit			exit upa from foward status\n"
		"upa list			list all nests online\n"
		"upa rssi			dynamic display current rssi value\n"
		"upa update on/off		enable/disable upa_update, debug purpose\n"
	};

	if(argc == 3) {
		if(!strncmp(argv[1], "select", 3)) {
			sscanf(argv[2], "%x", &addr);
			nest = upa_nest_get(addr);
			if(nest != NULL) {
				upa_nest_sel = nest;
				shell_trap(upa_uart_cnsl, &cmd_upa); //all cmds redirect to cmd_upa in uart console
				shell_trap(upa_wl_cnsl, &cmd_monitor); //all cmds redirect to cmd_monitor in wireless console
				shell_lock(upa_uart_cnsl, 1);
				shell_prompt(upa_uart_cnsl, "nest# ");
				printf("upa select ok\n");
			}
			else
				printf("upa select fail\n");
			return 0;
		}

		if(!strcmp(argv[1], "update")) {
			upa_flag.update_off = (!strcmp(argv[2], "on")) ? 0 : 1;
			return 0;
		}
	}

	if(argc == 2) {
		if(!strcmp(argv[1], "exit")) {
			upa_nest_sel = NULL;
			shell_prompt(upa_uart_cnsl, "upa# ");
			shell_trap(upa_wl_cnsl, NULL);
			shell_trap(upa_uart_cnsl, NULL);
			printf("upa exit ok\n");
			return 0;
		}

		if(!strcmp(argv[1], "list")) {
			list_for_each(pos, &upa_nest_queue) {
				nest = list_entry(pos, upa_nest_s, list);
				printf("upa nest %08x %02d\n", nest->addr, upa_rssi_get(nest));
			}
			return 0;
		}

		if(!strncmp(argv[1], "rssi", 4)) {
			if(upa_nest_sel != NULL) {
				printf("\n");
				return 1;
			}
			else {
				printf("no nest is selected\n");
				return 0;
			}
		}
	}

	if(argc == 0 && (!strncmp(argv[1], "rssi", 4))) {
		printf("\rssi[%08x] = %d    ", upa_nest_sel->addr, upa_rssi_get(upa_nest_sel));
		return 1;
	}

	/* forward commands to wireless except "upa xxx"
	 * ASK: why does only argv[0] been forwarded?
	 * ACK: pls refer api shell_trap() */
	if((upa_nest_sel != NULL) && strcmp(argv[0], "upa")) {
		dev_write(upa_wl_fd, argv[0], strlen(argv[0]));
		dev_write(upa_wl_fd, "\r", 1);
		return 0;
	}

	printf("%s", usage);
	return 0;
}

/*monitor cmd handling*/
static int cmd_monitor_func(int argc, char *argv[])
{
	const char *usage = {
		"monitor newbie 1234567A		newbie nest request\n"
	};

	if((argc == 3) && (!strcmp(argv[1], "newbie"))) {
		unsigned addr;
		sscanf(argv[2], "%x", &addr);
		upa_nest_new(addr);
		if(1) {
			console_set(upa_uart_cnsl);
			printf("%s %s %s\n", argv[0], argv[1], argv[2]);
			console_set(upa_wl_cnsl);
		}
		return 0;
	}

	/* forward response from wireless to uart console except*/
	if(strcmp(argv[0], "monitor")) {
		console_set(upa_uart_cnsl);
		printf("%s\n", argv[0]);
		console_set(upa_wl_cnsl);
		return 0;
	}

	if(console_get() == upa_uart_cnsl)
		printf("%s", usage);

	return 0;
}
