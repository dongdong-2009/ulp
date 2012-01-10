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

#define UPA_FLAG_MON (1<<0)
#define UPA_FLAG_SEL (1<<1)

struct upa_nest_s {
	unsigned addr;
	unsigned flag;
	unsigned fail;
	time_t timer; //update timer
	struct list_head list;
};

static time_t upa_mon_timer;
static int upa_wl_fd;
static struct console_s *upa_wl_cnsl;
#define upa_uart_cnsl ((struct console_s *) &uart2)
static LIST_HEAD(upa_nest_queue);
static enum {
	UPA_STATUS_INVALID,
	UPA_STATUS_INIT,
	UPA_STATUS_PRX,
	UPA_STATUS_PTX_MON, //update monitor nest
	UPA_STATUS_PTX_SEL, //update selected nest
	UPA_STATUS_PTX_UPD, //update each nest
} upa_status, upa_status_next;

static int upa_wl_onfail(int ecode, ...)
{
	unsigned addr;
	va_list args;
	struct upa_nest_s *nest;
	struct list_head *pos;

	va_start(args, ecode);
	switch(ecode) {
	case WL_ERR_TX_TIMEOUT:
		addr = va_arg(args, unsigned);
		//search the nest with the specified addr
		list_for_each(pos, &upa_nest_queue) {
			nest = list_entry(pos, upa_nest_s, list);
			if(nest->addr == addr) {
				nest->fail ++;
				dev_ioctl(upa_wl_fd, WL_FLUSH);
				break;
			}
		}
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
			dev_ioctl(upa_wl_fd, WL_ERR_FUNC, upa_wl_onfail);
			dev_ioctl(upa_wl_fd, WL_SET_MODE, WL_MODE_PRX);
			dev_ioctl(upa_wl_fd, WL_SET_ADDR, NEST_WL_ADDR);
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
		if(upa_status_next != upa_status) {
			list_for_each(pos, &upa_nest_queue) {
				nest = list_entry(pos, upa_nest_s, list);
				if((nest->flag & UPA_FLAG_MON) && (time_left(upa_mon_timer) < 0)) {
					upa_mon_timer = time_get(NEST_WL_MS);
					upa_status = upa_status_next;
					shell_lock(upa_uart_cnsl, 1);
					dev_ioctl(upa_wl_fd, WL_STOP);
					dev_ioctl(upa_wl_fd, WL_SET_MODE, WL_MODE_PTX);
					dev_ioctl(upa_wl_fd, WL_SET_ADDR, nest->addr);
					dev_ioctl(upa_wl_fd, WL_START);
					shell_lock(upa_wl_cnsl, 0);
					break;
				}
			}
		}
		upa_status_next = UPA_STATUS_PTX_SEL;
		if(list_empty(&upa_nest_queue))
			upa_status_next = UPA_STATUS_PRX;
		break;
	case UPA_STATUS_PTX_SEL:
		if(upa_status_next != upa_status) {
			upa_status = upa_status_next;
			list_for_each(pos, &upa_nest_queue) {
				nest = list_entry(pos, upa_nest_s, list);
				if(nest->flag & UPA_FLAG_SEL)
					break;
			}
			dev_ioctl(upa_wl_fd, WL_STOP);
			dev_ioctl(upa_wl_fd, WL_SET_MODE, WL_MODE_PTX);
			dev_ioctl(upa_wl_fd, WL_SET_ADDR, nest->addr);
			dev_ioctl(upa_wl_fd, WL_START);
			shell_lock(upa_wl_cnsl, 0);
			shell_lock(upa_uart_cnsl, 0);
		}
		upa_status_next = UPA_STATUS_PTX_UPD;
		if(list_empty(&upa_nest_queue))
			upa_status_next = UPA_STATUS_PRX;
		break;
	case UPA_STATUS_PTX_UPD:
		if(upa_status_next != upa_status) {
			list_for_each(pos, &upa_nest_queue) {
				nest = list_entry(pos, upa_nest_s, list);
				if(time_left(nest->timer) < 0) {
					nest->timer = time_get(NEST_WL_MS/2);
					upa_status = upa_status_next;
					shell_lock(upa_uart_cnsl, 1);

					dev_ioctl(upa_wl_fd, WL_STOP);
					dev_ioctl(upa_wl_fd, WL_SET_MODE, WL_MODE_PTX);
					dev_ioctl(upa_wl_fd, WL_SET_ADDR, nest->addr);
					dev_ioctl(upa_wl_fd, WL_START);
					shell_lock(upa_wl_cnsl, 0);

					//send ping req to nest
					n = sprintf(frame, "monitor ping\r");
					dev_write(upa_wl_fd, frame, n);
					break;
				}
			}
		}
		upa_status_next = UPA_STATUS_PTX_MON;
		if(list_empty(&upa_nest_queue))
			upa_status_next = UPA_STATUS_PRX;
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
	const char *usage = {
		"upa select 1234567a		select current nest\n"
		"upa deselect			exit upa from foward status\n"
		"upa list			list all nests online\n"
		"upa update on/off		enable/disable upa_update, debug purpose\n"
	};

	if(argc == 3) {
		if(!strcmp(argv[1], "select")) {
			sscanf(argv[2], "%x", &addr);
			list_for_each(pos, &upa_nest_queue) {
				nest = list_entry(pos, upa_nest_s, list);
				if(nest->addr == addr) {
					shell_trap(upa_uart_cnsl, &cmd_upa); //all cmds redirect to cmd_upa in uart console
					shell_trap(upa_wl_cnsl, &cmd_monitor); //all cmds redirect to cmd_monitor in wireless console
					shell_lock(upa_uart_cnsl, 1);
					upa_status = UPA_STATUS_INVALID; //force mode change in upa_update
					break;
				}
			}

			printf("upa select ok\n");
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
		if(!strcmp(argv[1], "deselect")) {
			list_for_each(pos, &upa_nest_queue) {
				nest = list_entry(pos, upa_nest_s, list);
				if(nest->flag & UPA_FLAG_SEL) {
					nest->flag &= ~UPA_FLAG_SEL;
					break;
				}
			}
			shell_trap(upa_wl_cnsl, NULL);
			shell_trap(upa_uart_cnsl, NULL);
			printf("upa deselect ok\n");
			return 0;
		}

		if(!strcmp(argv[1], "list")) {
			list_for_each(pos, &upa_nest_queue) {
				nest = list_entry(pos, upa_nest_s, list);
				printf("upa nest %08x\n", nest->addr);
			}
			return 0;
		}
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
		struct upa_nest_s *nest = NULL;
		struct list_head *pos;
		unsigned addr;

		//search the nest in the list to avoid duplication
		sscanf(argv[2], "%x", &addr);
		list_for_each(pos, &upa_nest_queue) {
			nest = list_entry(pos, upa_nest_s, list);
			if(nest->addr == addr)
				break;
		}

		//create a new one if not found in current list
		if(pos == &upa_nest_queue) {
			nest = sys_malloc(sizeof(struct upa_nest_s));
			if(nest != NULL) {
				nest->addr = addr;
				nest->fail = 0;
				nest->flag = (list_empty(&upa_nest_queue)) ? UPA_FLAG_MON : 0;
				list_add_tail(&nest->list, &upa_nest_queue);
			}
		}
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
