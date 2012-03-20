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

#define UPA_MS_DEF 1000
#define UPA_MS_MON 30000
#define UPA_MS_UPD 1000

static union {
	struct {
		unsigned update_off : 1;
		unsigned monitor_shutup : 1;
		unsigned monitor_wakeup : 1;
		unsigned monitor_over : 1;
		unsigned monitor_clr : 1;
		unsigned monitor_set : 1;
		unsigned monitor_ping : 1;
	};
	unsigned value;
} upa_flag;

static time_t upa_mon_timer; /*to avoid chat with monitor in every loop*/
static struct upa_nest_s *upa_nest_mon; //monitor nest
static struct upa_nest_s *upa_nest_sel; //selected nest
static struct upa_nest_s *upa_nest_cur; //current access nest

#define debug(lvl, ...) do { \
	if(lvl >= 0) { \
		console_set(NULL); \
		printf("%s: ", __func__); \
		printf(__VA_ARGS__); \
		console_restore(); \
	} \
} while (0)

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
static int upa_nest_bad(struct upa_nest_s *nest)
{
	assert(nest != NULL);
	debug(0, "%08x\n", nest->addr);
	if(upa_nest_mon == nest) {
		//change it, too disappointed ...
		upa_nest_mon = NULL;
	}

	if(upa_nest_cur == nest) {
		//dev_ioctl(upa_wl_fd, WL_FLUSH);
		upa_nest_cur = NULL;
	}

	//remove it? yes!
	return 0;
}

/*there is no active nest on the queue ?*/
int upa_nest_empty(void)
{
	return list_empty(&upa_nest_queue);
}

/* redirect printf output to wireless console
 * !!!do not write a string longer than 128 bytes
 */
int upa_nest_printf(const char *fmt, ...)
{
	va_list args;
	time_t timer = time_get(NEST_WL_MS/10);
	static char str[128];
	int n, ret;

	if(upa_nest_cur == NULL)
		return -1;

	//output to a string buf
	va_start(args, fmt);
	n = vsnprintf(str, 128, fmt, args);
	va_end(args);

	//ensure there's enough space in tbuf
	while(dev_poll(upa_wl_fd, POLLOBUF) < n) {
		task_Update();
		if(time_left(timer) < 0) {
			debug(1, "obuf full\n");
			upa_nest_bad(upa_nest_cur);
			return -1;
		}
	}

	//write to tbuf
	ret = dev_write(upa_wl_fd, str, n);
	ret = (ret == n) ? 0 : -1;
	return ret;
}

int upa_nest_switch(struct upa_nest_s *nest)
{
	int ecode = 0;
	time_t timer;

	if(upa_nest_cur == nest)
		return 0;

	//close cur nest first
	if(upa_nest_cur != NULL) {
		//wait for cmd "upa shutup\r" sent by target nest
		timer = time_get(UPA_MS_DEF);
		upa_flag.monitor_shutup = 0;
		//"i am go now", it is  just a notice, may lost ...
		upa_nest_printf("monitor shutup\r");
		while(upa_flag.monitor_shutup == 0) {
			task_Update();
			if(time_left(timer) < 0) {
				//wait for nest response timeout
				debug(1, "shutup fail\n");
				upa_nest_bad(upa_nest_cur);
				break;
			}
		}
		upa_nest_cur = NULL;
	}

	//switch to new one
	if(nest != NULL) {
		dev_ioctl(upa_wl_fd, WL_STOP);
		dev_ioctl(upa_wl_fd, WL_SET_MODE, WL_MODE_PTX);
		dev_ioctl(upa_wl_fd, WL_SET_ADDR, nest->addr);
		dev_ioctl(upa_wl_fd, WL_START);
		upa_rssi_enable(nest);

		upa_nest_cur = nest;

		//send cmd "monitor wakeup\r"
		upa_flag.monitor_wakeup = 0;
		timer = time_get(UPA_MS_DEF);
		ecode = upa_nest_printf("monitor wakeup\r");
		if(!ecode) {
			//wait for cmd "upa wakeup\r" sent by target nest
			while(upa_flag.monitor_wakeup == 0) {
				task_Update();
				if(time_left(timer) < 0) {
					//wait for nest response timeout
					debug(1, "wakeup fail\n");
					upa_nest_bad(nest);
					ecode = -1;
					break;
				}
			}
		}
	}

	upa_nest_cur = (ecode) ? NULL : nest;
	return ecode;
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
	shell_mute(upa_wl_cnsl);
	shell_prompt(upa_uart_cnsl, "upa# ");
	dev_ioctl(upa_wl_fd, WL_FLUSH);
	upa_nest_mon = NULL;
	upa_nest_sel = NULL;
	upa_nest_cur = NULL;
	upa_flag.value = 0;
	return 0;
}

//poll monitor
static void upa_update_mon(void)
{
	struct upa_nest_s *nest;
	int ecode = 0;
	time_t timer;

	//fetch newbie info
	if(upa_nest_mon != NULL) {
		timer = time_get(UPA_MS_DEF);
		upa_flag.monitor_over = 0;
		ecode = upa_nest_switch(upa_nest_mon);
		ecode += upa_nest_printf("monitor get\r");
		if(!ecode) { //success, wait for monitor response until monitor say "monitor over" or timeout
			while(upa_flag.monitor_over == 0) {
				task_Update();
				if(time_left(timer) <= 0) {
					debug(1, "monitor get fail\n");
					upa_nest_bad(upa_nest_mon);
					ecode = -1;
					break;
				}
			}
		}
	}

	// :( monitor not response, dismiss it without say goodbye
	if(ecode) {
		upa_nest_mon = NULL;
	}

	//do we need a new monitor?
	nest = upa_nest_mon;
	if((nest == NULL) || (time_left(upa_mon_timer) < 0)) {
		nest = upa_mon_elect();
		upa_mon_timer = time_get(UPA_MS_MON);
	}

	if(nest == NULL || nest == upa_nest_mon)
		return;

	//dismiss old monitor, try our best to say goodbye ..
	if(upa_nest_mon != NULL) {
		timer = time_get(UPA_MS_DEF);
		upa_flag.monitor_clr = 0;
		ecode = upa_nest_switch(upa_nest_mon);
		ecode += upa_nest_printf("monitor clr\r");
		if(!ecode) { //success, wait for monitor response until monitor say "monitor clr" or timeout
			while(upa_flag.monitor_clr == 0) {
				task_Update();
				if(time_left(timer) <= 0) {
					debug(1, "monitor clr fail\n");
					upa_nest_bad(upa_nest_mon);
					ecode = -1;
					break;
				}
			}
		}
	}

	//new monitor takes office
	timer = time_get(UPA_MS_DEF);
	upa_flag.monitor_set = 0;
	ecode = upa_nest_switch(nest);
	ecode += upa_nest_printf("monitor set\r");
	if(!ecode) { //success, wait for monitor response until monitor say "monitor set" or timeout
		while(upa_flag.monitor_set == 0) {
			task_Update();
			if(time_left(timer) <= 0) {
				debug(1, "monitor set fail\n");
				upa_nest_bad(upa_nest_mon);
				ecode = -1;
				break;
			}
		}
	}

	//OK, Congratulations!!!
	upa_nest_mon = nest;
}

static void upa_Update(void)
{
	struct upa_nest_s *nest;
	struct list_head *pos;
	int ecode = 0;
	time_t timer;

	//lock the uart console to avoid cmd_upa_func foward the commands to wrong nest
	if(upa_nest_sel != NULL)
		shell_lock(upa_uart_cnsl);

	//poll monitor
	upa_update_mon();

	//update each nest
	list_for_each(pos, &upa_nest_queue) {
		nest = list_entry(pos, upa_nest_s, list);
		if(time_left(nest->timer) < 0) {
			timer = time_get(UPA_MS_DEF);
			upa_flag.monitor_ping = 0;
			ecode = upa_nest_switch(nest);
			ecode += upa_nest_printf("monitor ping\r");
			if(!ecode) { //success, wait for monitor response until monitor say "monitor ping" or timeout
				while(upa_flag.monitor_ping == 0) {
					task_Update();
					if(time_left(timer) <= 0) {
						debug(1, "monitor ping fail\n");
						upa_nest_bad(nest);
						ecode = -1;
						break;
					}
				}
				if(upa_flag.monitor_ping) {
					nest->timer = time_get(UPA_MS_UPD);
				}
			}
		}
	}

	//poll selected nest
	if(upa_nest_sel != NULL) {
		upa_nest_switch(upa_nest_sel);
	}

	//always unlock the uart cnsl before exit upa_Update
	shell_unlock(upa_uart_cnsl);
	task_Update();
}

void main(void)
{

	task_Init();
	upa_Init();
	printf("Power Conditioning - UPA\n");
	printf("IAR C Version v%x.%x, Compile Date: %s,%s\n", (__VER__ >> 24),((__VER__ >> 12) & 0xfff),  __TIME__, __DATE__);

	while(1){
		//switch to prx mode, wait for newbie request ...
		dev_ioctl(upa_wl_fd, WL_STOP);
		dev_ioctl(upa_wl_fd, WL_SET_MODE, WL_MODE_PRX);
		dev_ioctl(upa_wl_fd, WL_SET_ADDR, NEST_WL_ADDR);
		dev_ioctl(upa_wl_fd, WL_START);
		while(upa_nest_empty() || upa_flag.update_off) {
			task_Update();
		}

		//2,  i am a boss now :)
		while(!upa_nest_empty() && (upa_flag.update_off == 0)) {
			upa_Update();
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
		upa_nest_printf("%s\r", argv[0]);
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
