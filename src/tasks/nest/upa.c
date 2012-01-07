#include "config.h"
#include "ulp/debug.h"
#include "ulp_time.h"
#include "ulp/wl.h"
#include "spi.h"
#include "sys/task.h"
#include "shell/cmd.h"
#include "shell/shell.h"
#include "linux/list.h"

static int upa_wl_fd;
static struct console_s *upa_wl_cnsl;

static int upa_Init(void)
{
	struct console_s *cnsl;
	static const struct nrf_cfg_s nrf_cfg = {
		.spi = &spi1,
		.gpio_cs = SPI_1_NSS,
		.gpio_ce = SPI_CS_PC5,
	};

	dev_register("nrf", &nrf_cfg);
	upa_wl_fd = dev_open("wl0", 0);
	dev_ioctl(upa_wl_fd, WL_SET_FREQ, UPA_FREQ);
	dev_ioctl(upa_wl_fd, WL_ERR_FUNC, nest_wl_onfail);
	return 0;
}


static int upa_Update(void)
{
	if(upa_wl_cnsl == NULL) {
		//forward the received wireless data to host
		char c = 0;
		while(dev_poll(upa_wl_fd, POLLIN) > 0) {
			dev_read(upa_wl_fd, &c, 1);
			console_putch(c);
		}
	}
	return 0;
}

void main(void)
{
	task_Init();
	upa_Init();
	printf("\nPower Conditioning - UPA\n");
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
		if(!strcmp(argv[1], "monitor")) {
			dev_ioctl(upa_wl_fd, WL_SET_ADDR, NEST_WL_ADDR);
			if(!strcmp(argv[2], 'start')) {
				upa_wl_cnsl = console_register(upa_wl_fd);
				assert(upa_wl_cnsl != NULL);
				dev_ioctl(upa_wl_fd, WL_SET_MODE, WL_MODE_PRX);
				shell_register(upa_wl_cnsl);
			}
			else {
				dev_ioctl(upa_wl_fd, WL_SET_MODE, WL_MODE_PTX);
				shell_unregister(upa_wl_cnsl);
				console_unregister(upa_wl_cnsl);
				upa_wl_cnsl = NULL;
			}
			return 0;
		}

		if(!strcmp(argv[1], "select")) {
			int addr = 0;
			sscanf(argv[2], "%x", &addr);
			dev_ioctl(upa_wl_fd, WL_SET_ADDR, addr);
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
