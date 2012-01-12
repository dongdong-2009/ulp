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
	unsigned short retry;
	unsigned short retry_temp;
	time_t timer; //update timer
	struct list_head list;
};

static time_t upa_mon_timer;
static int upa_wl_fd;
static struct console_s *upa_wl_cnsl;
#define upa_uart_cnsl ((struct console_s *) &uart2)
static LIST_HEAD(upa_nest_queue);

//cache, to avoid search upa_nest_queue every time
static struct upa_nest_s *upa_nest_mon; //monitor nest
static struct upa_nest_s *upa_nest_sel; //selected nest
static struct upa_nest_s *upa_nest_cur; //current access nest

static enum {
	UPA_STATUS_INVALID,
	UPA_STATUS_INIT,
	UPA_STATUS_PRX,
	UPA_STATUS_PTX_MON, //update monitor nest
	UPA_STATUS_PTX_SEL, //update selected nest
	UPA_STATUS_PTX_UPD, //update each nest
} upa_status, upa_status_next;

static struct upa_nest_s* upa_mon_elect(void)
{
	struct upa_nest_s *nest, *hero = NULL;
	struct list_head *pos;
	unsigned retry = 0xffff;
	list_for_each(pos, &upa_nest_queue) {
		nest = list_entry(pos, upa_nest_s, list);
		if(nest->retry < retry) { //max 32767
			hero = nest;
			retry = nest->retry;
		}
	}

	if(upa_nest_mon != NULL) {
		if(upa_nest_mon->retry - hero->retry > 50) //do not change the hero too frequently
			hero = upa_nest_mon;
	}
	return hero;
}

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
		nest->retry = 0x7fff; //i hate newbie!!! :)
		nest->retry_temp = 0;
		nest->timer = 0;
		list_add_tail(&nest->list, &upa_nest_queue);
		return 0;
	}

	return -1; //no enough memory
}

static int upa_nest_bad(unsigned addr)
{
	struct upa_nest_s *nest = upa_nest_get(addr);
	if(upa_nest_mon == nest) {
		//change it, too disappointed ...
		upa_nest_mon = NULL;
		upa_status_next = UPA_STATUS_PTX_MON;
	}
	return 0;
}

static void upa_retry_fetch(struct upa_nest_s *nest)
{
	unsigned retry;
	if(nest != NULL) {
		dev_ioctl(upa_wl_fd, WL_GET_FAIL, &retry); //max 32767
		retry += nest->retry_temp;
		if(nest->retry_temp != 0) {
			retry >>= 1;
		}
		nest->retry_temp = retry;
	}
}

static void upa_retry_update(struct upa_nest_s *nest)
{
	if(nest != NULL) {
		nest->retry = nest->retry_temp;
		nest->retry_temp = 0;
	}
}

static int upa_wl_onfail(int ecode, ...)
{
	unsigned addr;
	va_list args;

	va_start(args, ecode);
	switch(ecode) {
	case WL_ERR_TX_TIMEOUT:
		addr = va_arg(args, unsigned);
		upa_nest_bad(addr);
		dev_ioctl(upa_wl_fd, WL_FLUSH);
		break;
	default:
		break;
	}
	va_end(args);

	console_select(upa_uart_cnsl);
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
	upa_wl_cnsl = console_register(upa_wl_fd);
	assert(upa_wl_cnsl != NULL);
	shell_register(upa_wl_cnsl);
	shell_mute(upa_wl_cnsl, 1);
	shell_prompt(upa_uart_cnsl, "upa# ");
	dev_ioctl(upa_wl_fd, WL_FLUSH);
	upa_status = UPA_STATUS_INVALID;
	upa_status_next = UPA_STATUS_INIT;
	return 0;
}


static int upa_Update(void)
{
	int ntx, nrx, n;
	struct list_head *pos;
	struct upa_nest_s *nest;
	char frame[32];
	ntx = dev_poll(upa_wl_fd, POLLOUT);
	nrx = dev_poll(upa_wl_fd, POLLIN);
	if(ntx != 0 || nrx != 0) //to avoid tbuf&rbuf flush
		return 0;

	//At least one nest exist when upa in ptx mode!!!
	switch (upa_status_next) {
	case UPA_STATUS_INIT:
		if(upa_status_next != upa_status) {
			upa_status = upa_status_next;
			shell_lock(upa_uart_cnsl, 1);
			shell_lock(upa_wl_cnsl, 1);
			shell_trap(upa_uart_cnsl, NULL);
			shell_trap(upa_wl_cnsl, NULL);
			dev_ioctl(upa_wl_fd, WL_SET_FREQ, NEST_WL_FREQ);
			dev_ioctl(upa_wl_fd, WL_ERR_TXMS, NEST_WL_TXMS);
			dev_ioctl(upa_wl_fd, WL_ERR_FUNC, upa_wl_onfail);
			dev_ioctl(upa_wl_fd, WL_SET_MODE, WL_MODE_PRX);
			dev_ioctl(upa_wl_fd, WL_SET_ADDR, NEST_WL_ADDR);
			upa_nest_cur = NULL;
			upa_nest_sel = NULL;
			upa_nest_mon = NULL;
		}
		upa_status_next = UPA_STATUS_PRX;
		break;
	case UPA_STATUS_PRX:
		if(upa_status_next != upa_status) {
			upa_status = upa_status_next;
			//switch to PRX mode
			dev_ioctl(upa_wl_fd, WL_STOP);
			dev_ioctl(upa_wl_fd, WL_SET_MODE, WL_MODE_PRX);
			dev_ioctl(upa_wl_fd, WL_SET_ADDR, NEST_WL_ADDR);

			dev_ioctl(upa_wl_fd, WL_START);
			shell_lock(upa_uart_cnsl, 0);
			shell_lock(upa_wl_cnsl, 0);
		}
		else {
			if(!list_empty(&upa_nest_queue))
				upa_status_next = UPA_STATUS_PTX_MON;
		}
		break;
	case UPA_STATUS_PTX_MON:
		if((upa_nest_mon == NULL) || (time_left(upa_mon_timer) < 0)) {
			//maybe we need to elect a hero here?
			upa_nest_mon = upa_mon_elect();
		}

		if(time_left(upa_mon_timer) < 0) {
			upa_mon_timer = time_get(NEST_WL_MS/2);
			upa_status = upa_status_next;
			shell_lock(upa_uart_cnsl, 1);
			shell_lock(upa_wl_cnsl, 0);
			if(upa_nest_cur != upa_nest_mon) {
				upa_retry_fetch(upa_nest_cur);
				upa_nest_cur = upa_nest_mon;
				dev_ioctl(upa_wl_fd, WL_STOP);
				dev_ioctl(upa_wl_fd, WL_SET_MODE, WL_MODE_PTX);
				dev_ioctl(upa_wl_fd, WL_SET_ADDR, upa_nest_mon->addr);
				dev_ioctl(upa_wl_fd, WL_START);
			}
			break;
		}
		upa_status_next = UPA_STATUS_PTX_SEL;
		break;
	case UPA_STATUS_PTX_SEL:
		upa_status = upa_status_next;
		shell_lock(upa_wl_cnsl, 0);
		shell_lock(upa_uart_cnsl, 0);
		if(upa_nest_sel != NULL) {
			if(upa_nest_cur != upa_nest_sel) {
				upa_retry_fetch(upa_nest_cur);
				upa_nest_cur = upa_nest_sel;
				dev_ioctl(upa_wl_fd, WL_STOP);
				dev_ioctl(upa_wl_fd, WL_SET_MODE, WL_MODE_PTX);
				dev_ioctl(upa_wl_fd, WL_SET_ADDR, upa_nest_sel->addr);
				dev_ioctl(upa_wl_fd, WL_START);
			}
		}
		upa_status_next = UPA_STATUS_PTX_UPD;
		break;
	case UPA_STATUS_PTX_UPD:
		list_for_each(pos, &upa_nest_queue) {
			nest = list_entry(pos, upa_nest_s, list);
			if(time_left(nest->timer) < 0) {
				nest->timer = time_get(NEST_WL_MS/2);
				upa_status = upa_status_next;
				shell_lock(upa_uart_cnsl, 1);
				shell_lock(upa_wl_cnsl, 0);
				upa_retry_fetch(upa_nest_cur);
				upa_retry_update(nest);
				if(upa_nest_cur != nest) {
					upa_nest_cur = nest;
					dev_ioctl(upa_wl_fd, WL_STOP);
					dev_ioctl(upa_wl_fd, WL_SET_MODE, WL_MODE_PTX);
					dev_ioctl(upa_wl_fd, WL_SET_ADDR, nest->addr);
					dev_ioctl(upa_wl_fd, WL_START);
				}

				//send ping req to nest
				n = sprintf(frame, "monitor ping\r");
				dev_write(upa_wl_fd, frame, n);
				break;
			}
		}
		upa_status_next = UPA_STATUS_PTX_MON;
		break;
	default:
		upa_status_next = UPA_STATUS_INIT;
		break;
	}
	return 0;
}

static char upa_update_disable = 0;
void main(void)
{
	task_Init();
	upa_Init();
	printf("Power Conditioning - UPA\n");
	printf("IAR C Version v%x.%x, Compile Date: %s,%s\n", (__VER__ >> 24),((__VER__ >> 12) & 0xfff),  __TIME__, __DATE__);

	while(1){
		task_Update();
		if(!upa_update_disable)
			upa_Update();
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
	static char *cmd_upa_sub = NULL;
	const char *usage = {
		"upa select 1234567a		select current nest\n"
		"upa exit			exit upa from foward status\n"
		"upa list			list all nests online\n"
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
			upa_update_disable = strcmp(argv[2], "on");
			if(upa_update_disable) {
				shell_lock(upa_wl_cnsl, 1);
				shell_lock(upa_uart_cnsl, 0);
			}
			else { 	//return
				upa_status = UPA_STATUS_INVALID;
				upa_status = UPA_STATUS_INIT;
			}
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
				printf("upa nest %08x %02d\n", nest->addr, nest->retry);
			}
			return 0;
		}

		if(!strncmp(argv[1], "signal", 3)) {
			if(upa_nest_sel != NULL) {
				cmd_upa_sub = argv[1];
				printf("\n");
				return 1;
			}
			else {
				printf("no nest selected\n");
				return 0;
			}
		}
	}

	if(argc == 0 && (!strncmp(cmd_upa_sub, "signal", 3))) {
		printf("\rsignal[%08x] = %d    ", upa_nest_sel->addr, upa_nest_sel->retry);
		return 1;
	}

	if(strcmp(argv[0], "upa") && (upa_status == UPA_STATUS_PTX_SEL)) { //commands to be forwarded to wireless
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
			console_select(upa_uart_cnsl);
			printf("%s %s %s\n", argv[0], argv[1], argv[2]);
			console_select(upa_wl_cnsl);
		}
		return 0;
	}

	if(strcmp(argv[0], "monitor") && (upa_status == UPA_STATUS_PTX_SEL)) { //commands to be forwarded to uart
		console_select(upa_uart_cnsl);
		printf("%s\n", argv[0]);
		console_select(upa_wl_cnsl);
		return 0;
	}

	//printf("%s", usage);
	return 0;
}
