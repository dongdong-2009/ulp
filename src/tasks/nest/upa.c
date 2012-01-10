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

#define UPA_FLAG_MON (1<<0)
#define UPA_FLAG_SEL (1<<1)
#define UPA_FLAG_UPD (1<<2)

struct upa_nest_s {
	unsigned addr;
	unsigned flag;
	struct list_head list;
};

static int upa_wl_fd;
static struct console_s *upa_wl_cnsl;
static LIST_HEAD(upa_nest_queue);
static enum {
	UPA_STATUS_INIT,
	UPA_STATUS_PRX,
	UPA_STATUS_PTX_MON, //update monitor nest
	UPA_STATUS_PTX_SEL, //update selected nest
	UPA_STATUS_PTX_UPD, //update each nest
} upa_status;

static int upa_Init(void)
{
	static const struct nrf_cfg_s nrf_cfg = {
		.spi = &spi1,
		.gpio_cs = SPI_1_NSS,
		.gpio_ce = SPI_CS_PC5,
	};

	dev_register("nrf", &nrf_cfg);
	upa_wl_fd = dev_open("wl0", 0);
	dev_ioctl(upa_wl_fd, WL_SET_FREQ, NEST_WL_FREQ);
	//dev_ioctl(upa_wl_fd, WL_ERR_FUNC, nest_wl_onfail);
	upa_wl_cnsl = console_register(upa_wl_fd);
	assert(upa_wl_cnsl != NULL);
	shell_register(upa_wl_cnsl);
	shell_mute(upa_wl_cnsl, 1);
	dev_ioctl(upa_wl_fd, WL_FLUSH);
	upa_status = UPA_STATUS_INIT;
	return 0;
}


static int upa_Update(void)
{
	int ntx, nrx;
	struct list_head *pos;
	struct upa_nest_s *nest;
	ntx = dev_poll(upa_wl_fd, POLLOUT);
	nrx = dev_poll(upa_wl_fd, POLLIN);
	if(ntx != 0 || nrx != 0) //to avoid tbuf&rbuf flush
		return 0;

	//state machine change?
	if(upa_status == UPA_STATUS_PRX) {
		if(!list_empty(&upa_nest_queue)) {
			//switch to PTX mode
			dev_ioctl(upa_wl_fd, WL_STOP);
			dev_ioctl(upa_wl_fd, WL_SET_MODE, WL_MODE_PTX);
			dev_ioctl(upa_wl_fd, WL_SET_ADDR, NEST_WL_ADDR);
			dev_ioctl(upa_wl_fd, WL_START);
			upa_status = UPA_STATUS_PTX_MON;
		}
	}
	else {
		if(list_empty(&upa_nest_queue)) {
			//switch to PRX mode
			dev_ioctl(upa_wl_fd, WL_STOP);
			dev_ioctl(upa_wl_fd, WL_SET_MODE, WL_MODE_PRX);
			dev_ioctl(upa_wl_fd, WL_SET_ADDR, NEST_WL_ADDR);
			dev_ioctl(upa_wl_fd, WL_START);
			upa_status = UPA_STATUS_PRX;
		}
	}

	//At least one nest exist when upa in ptx mode!!!
	switch (upa_status) {
	case UPA_STATUS_PRX:
		break;
	case UPA_STATUS_PTX_MON:
		list_for_each(pos, &upa_nest_queue) {
			nest = list_entry(pos, upa_nest_s, list);
			if(nest->flag & UPA_FLAG_MON)
				break;
		}
		dev_ioctl(upa_wl_fd, WL_STOP);
		dev_ioctl(upa_wl_fd, WL_SET_ADDR, nest->addr);
		dev_ioctl(upa_wl_fd, WL_START);
		upa_status = UPA_STATUS_PTX_SEL;
		break;
	case UPA_STATUS_PTX_SEL:
		list_for_each(pos, &upa_nest_queue) {
			nest = list_entry(pos, upa_nest_s, list);
			if(nest->flag & UPA_FLAG_SEL)
				break;
		}
		dev_ioctl(upa_wl_fd, WL_STOP);
		dev_ioctl(upa_wl_fd, WL_SET_ADDR, nest->addr);
		dev_ioctl(upa_wl_fd, WL_START);
		upa_status = UPA_STATUS_PTX_UPD;
		break;
	case UPA_STATUS_PTX_UPD:
		//seach for current
		list_for_each(pos, &upa_nest_queue) {
			nest = list_entry(pos, upa_nest_s, list);
			if(nest->flag & UPA_FLAG_UPD) {
				nest->flag &= ~UPA_FLAG_UPD;
				break;
			}
		}

		//search for next nest in list
		pos = pos->next;
		if(pos == &upa_nest_queue) {
			pos = pos->next;
		}
		nest = list_entry(pos, upa_nest_s, list);
		nest->flag |= UPA_FLAG_UPD;
		dev_ioctl(upa_wl_fd, WL_STOP);
		dev_ioctl(upa_wl_fd, WL_SET_ADDR, nest->addr);
		dev_ioctl(upa_wl_fd, WL_START);
		upa_status = UPA_STATUS_PTX_MON;
		break;
	default:
		upa_status = UPA_STATUS_INIT;
		break;
	}
	return 0;
}

void main(void)
{
	task_Init();
	upa_Init();
	printf("Power Conditioning - UPA\n");
	printf("IAR C Version v%x.%x, Compile Date: %s,%s\n", (__VER__ >> 24),((__VER__ >> 12) & 0xfff),  __TIME__, __DATE__);

	while(1){
		task_Update();
		upa_Update();
	}
} // main

/*upa cmd handling*/
static int cmd_upa_func(int argc, char *argv[])
{
	const char *usage = {
		"upa monitor start/stop		monitor mode start/stop\n"
		"upa select 1234567a		select current nest\n"
		"upa forward 'nest id'		forward command to nest\n"
	};

	if(argc == 3) {
		if(!strcmp(argv[1], "select")) {
			int addr = 0;
			sscanf(argv[2], "%x", &addr);
			return 0;
		}

		if(!strcmp(argv[1], "forward")) {
			int n = strlen(argv[2]);
			argv[2][n] = '\r';
			dev_write(upa_wl_fd, argv[2], n + 1);
			return 0;
		}
	}

	printf("%s", usage);
	return 0;
}


const static cmd_t cmd_upa = {"upa", cmd_upa_func, "universal wireless program adapter for nest"};
DECLARE_SHELL_CMD(cmd_upa)

/*monitor cmd handling*/
static int cmd_monitor_func(int argc, char *argv[])
{
	const char *usage = {
		"monitor newbie 1234567A		newbie nest request\n"
	};

	if((argc == 3) && (!strcmp(argv[1], "newbie"))) {
		console_select(NULL);
		printf("%s %s %s\n", argv[0], argv[1], argv[2]);
		console_select(upa_wl_cnsl);
		return 0;
	}

	printf("%s", usage);
	return 0;
}


const static cmd_t cmd_monitor = {"monitor", cmd_monitor_func, "nest wireless monitor cmds"};
DECLARE_SHELL_CMD(cmd_monitor)
