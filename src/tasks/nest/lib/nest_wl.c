#include "config.h"
#include "ulp/wl.h"
#include "ulp_time.h"
#include "spi.h"
#include "nest_wl.h"
#include "shell/shell.h"
#include "shell/cmd.h"
#include <stdarg.h>
#include <string.h>

static time_t nest_wl_timer;
static unsigned nest_wl_fd;
static unsigned nest_wl_addr;

static int nest_wl_onfail(int ecode, ...);

int nest_wl_init(void)
{
	struct console_s *cnsl;
	static const struct nrf_cfg_s nrf_cfg = {
		.spi = &spi1,
		.gpio_cs = SPI_1_NSS,
		.gpio_ce = SPI_CS_PA12,
	};
	nest_wl_timer = 0;
	nest_wl_addr = *(unsigned *) 0x1ffff7e8; /*u_id 31:0*/

	dev_register("nrf", &nrf_cfg);
	nest_wl_fd = dev_open("wl0", 0);
	dev_ioctl(nest_wl_fd, WL_SET_FREQ, NEST_WL_FREQ);
	dev_ioctl(nest_wl_fd, WL_ERR_FUNC, nest_wl_onfail);

	cnsl = console_register(nest_wl_fd);
	assert(cnsl != NULL);
	shell_register(cnsl);
	shell_mute(cnsl, 1);
	dev_ioctl(nest_wl_fd, WL_FLUSH);
	return 0;
}

/*wireless console realize*/
int nest_wl_update(void)
{
	char frame[33], n;
	unsigned random;
	if(time_left(nest_wl_timer) < 0) {
		/*add a unique value to timeout to avoid all nest request monitor at the same time*/
		nest_wl_timer = time_get(NEST_WL_MS +(nest_wl_addr & 0x0fff));

		//ping frame has not been received in timeout period, re connect ...
		n = sprintf(frame + 2, "monitor newbie %x\r", nest_wl_addr);
		frame[0] = n + 2; //wl frame length
		frame[1] = WL_FRAME_DATA;
		dev_ioctl(nest_wl_fd, WL_STOP);
		dev_ioctl(nest_wl_fd, WL_SET_ADDR, NEST_WL_ADDR);
		dev_ioctl(nest_wl_fd, WL_SET_MODE, WL_MODE_PTX); //ptx
		dev_ioctl(nest_wl_fd, WL_START);
		dev_ioctl(nest_wl_fd, WL_SEND, frame, 0);

		//change to normal mode
		dev_ioctl(nest_wl_fd, WL_STOP);
		dev_ioctl(nest_wl_fd, WL_SET_ADDR, nest_wl_addr);
		dev_ioctl(nest_wl_fd, WL_SET_MODE, WL_MODE_PRX);
		dev_ioctl(nest_wl_fd, WL_START);
	}
	return 0;
}

static int nest_wl_onfail(int ecode, ...)
{
	unsigned char *frame;
	unsigned i, n, v;
	va_list args;

	va_start(args, ecode);
	switch(ecode) {
	case WL_ERR_TX_TIMEOUT:
		break; //nest is in prx mode,target monitor/upa,  flush is not needed
	case WL_ERR_RX_FRAME:
		frame = va_arg(args, unsigned char *);
		n = va_arg(args, char);
		if(frame[0] == WL_FRAME_PING) { //upa still remember me, hoho ...
			nest_wl_timer = time_get(NEST_WL_MS + (nest_wl_addr & 0x1fff));
		}
		break;
	default:
		console_select(NULL);
		printf("ecode = %d\n", ecode);
		console_restore();
		break;
	}
	va_end(args);
	//assert(0); //!!! impossible: got rx_dr but no payload?
	return 0;
}

/*monitor cmd handling*/
#define NSZ 16 /*monitor newbie queue size*/
static unsigned newbie[NSZ];

static int cmd_monitor_func(int argc, char *argv[])
{
	const char *usage = {
		"monitor newbie 1234567A		newbie nest request\n"
		"monitor report			report newbie list to upa\n"
		"monitor shutup			to shut nest's mouth\n"
		"monitor ping			periodically triged by upa\n"
	};

	if((argc == 2) && (!strcmp(argv[1], "ping"))) {
		nest_wl_timer = time_get(NEST_WL_MS + (nest_wl_addr & 0x1fff));
		return 0;
	}

	if((argc == 2) && (!strcmp(argv[1], "shutup"))) {
		printf("upa shutup\n");
		return 0;
	}

	if((argc == 2) && (!strcmp(argv[1], "report"))) {
		for(int i = 0; i < NSZ; i ++) {
			if(newbie[i]) {
				printf("monitor newbie %x\n", newbie[i]);
				newbie[i] = 0;
			}
		}
		return 0;
	}

	if((argc == 3) && (!strcmp(argv[1], "newbie"))) {
		unsigned val, to_be_cover = 0;
		sscanf(argv[2], "%x", &val);

		//add to queue and clear the duplicate one
		for(int i = 0; i < NSZ; i ++) {
			if(newbie[i] == to_be_cover) {
				newbie[i] = val;
				to_be_cover = (val == 0) ? to_be_cover : val;
				val = 0;
			}
		}
		return 0;
	}

	printf("%s", usage);
	return 0;
}


const static cmd_t cmd_monitor = {"monitor", cmd_monitor_func, "nest wireless monitor cmds"};
DECLARE_SHELL_CMD(cmd_monitor)
