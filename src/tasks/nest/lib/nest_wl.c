#include "config.h"
#include "ulp/wl.h"
#include "ulp_time.h"
#include "spi.h"
#include "nest_wl.h"
#include "shell/shell.h"
#include "shell/cmd.h"
#include <stdarg.h>
#include <string.h>
#include "sys/malloc.h"
#include "console.h"

#define debug(lvl, ...) do { \
	if(lvl >= 0) { \
		console_set(NULL); \
		printf("%s: ", __func__); \
		printf(__VA_ARGS__); \
		console_restore(); \
	} \
} while (0)

static time_t nest_wl_timer;
static unsigned nest_wl_fd;
static unsigned nest_wl_addr;
static struct console_s *nest_wl_cnsl;

static int nest_wl_onfail(int ecode, ...);

int nest_wl_start(void)
{
	nest_wl_fd = dev_open("wl0", 0);
	if(nest_wl_fd == 0) {
		printf("init: device wl0 open failed!!!\n");
		return -1;
	}

	//wireless configuration
	dev_ioctl(nest_wl_fd, WL_SET_FREQ, NEST_WL_FREQ);
	dev_ioctl(nest_wl_fd, WL_ERR_TXMS, NEST_WL_TXMS);
	dev_ioctl(nest_wl_fd, WL_ERR_FUNC, nest_wl_onfail);

	nest_wl_cnsl = console_register(nest_wl_fd);
	assert(nest_wl_cnsl != NULL);
	shell_register(nest_wl_cnsl);
	shell_mute(nest_wl_cnsl);
	dev_ioctl(nest_wl_fd, WL_FLUSH);
	return 0;
}

int nest_wl_init(void)
{
	static const struct nrf_cfg_s nrf_cfg = {
		.spi = &spi1,
		.gpio_cs = SPI_1_NSS,
		.gpio_ce = SPI_CS_PA12,
	};
	nest_wl_timer = 0;
	nest_wl_addr = *(unsigned *) 0x1ffff7e8; /*u_id 31:0*/

	dev_register("nrf", &nrf_cfg);
	return nest_wl_start();
}

/*wireless console realize*/
int nest_wl_update(void)
{
	char frame[33], n;
	if(nest_wl_fd == 0)
		return -1;

	if(time_left(nest_wl_timer) < 0) {
		/*add a unique value to timeout to avoid all nest request monitor at the same time*/
		nest_wl_timer = time_get(NEST_WL_MS +(nest_wl_addr & 0x0fff));

		//ping frame has not been received in timeout period, re connect ...
		dev_ioctl(nest_wl_fd, WL_STOP);
		dev_ioctl(nest_wl_fd, WL_SET_ADDR, NEST_WL_ADDR);
		dev_ioctl(nest_wl_fd, WL_SET_MODE, WL_MODE_PTX); //ptx
		dev_ioctl(nest_wl_fd, WL_START);
		n = sprintf(frame, "monitor newbie %x\r", nest_wl_addr);
		dev_write(nest_wl_fd, frame, n);
		//dev_ioctl(nest_wl_fd, WL_SYNC);

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
	case WL_ERR_TX_TIMEOUT: //something err, flush to avoid deadlock
		dev_ioctl(nest_wl_fd, WL_FLUSH);
		nest_wl_timer = 0; //force to reconnect
		break;
	case WL_ERR_RX_FRAME:
		frame = va_arg(args, unsigned char *);
		n = va_arg(args, char);
		if(frame[0] == WL_FRAME_PING) { //upa still remember me, hoho ...
			nest_wl_timer = time_get(NEST_WL_MS + (nest_wl_addr & 0x1fff));
		}
		else { //display messy codes
			char *str = sys_malloc(128);
			int n = sprintf(str, "rx strange frame(%02x", frame[0]);
			for(i = 1; i < n; i ++)
				n += sprintf(str + n, " %02x", frame[i]);
			sprintf(str + n, ")\n");
			debug(1, "%s", str);
			sys_free(str);
		}
		break;
	default:
		console_set(NULL);
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
		"monitor newbie 1234567A	newbie nest request\n"
		"monitor report			report newbie list to upa\n"
		"monitor wakeup/shutup/ping	normal nest wireless handshake signals\n"
		"monitor set/clr		monitor function enable or disable\n"
		"monitor stop/start		stop/start nest wireless\n"
	};

	//update timer
	int ms = time_left(nest_wl_timer);
	ms = NEST_WL_MS + (nest_wl_addr & 0x1fff) - ms;
	debug(1, "ms = %d\n", ms);
	nest_wl_timer = time_get(NEST_WL_MS + (nest_wl_addr & 0x1fff));

	//handle commands
	if(argc == 2) {
		if(!strcmp(argv[1], "help")) {
			printf("%s", usage);
			return 0;
		}

		if(!strcmp(argv[1], "start")) {
			if(nest_wl_fd == 0) {
				nest_wl_start();
			}
			return 0;
		}

		if(!strcmp(argv[1], "stop")) {
			if(nest_wl_fd != 0) {
				shell_unregister(nest_wl_cnsl);
				console_unregister(nest_wl_cnsl);
				dev_ioctl(nest_wl_fd, WL_STOP);
				dev_ioctl(nest_wl_fd, WL_ERR_FUNC, NULL);
				dev_close(nest_wl_fd);
				nest_wl_fd = 0;
				nest_wl_cnsl = NULL;
			}
			return 0;
		}

		if(!strcmp(argv[1], "report")) {
			for(int i = 0; i < NSZ; i ++) {
				if(newbie[i]) {
					printf("monitor newbie %x\n", newbie[i]);
					newbie[i] = 0;
				}
			}
			printf("upa over\n");
			return 0;
		}

		if(!strcmp(argv[1], "set")) { // i am the monitor :)
			printf("upa %s\n", argv[1]);
			return 0;
		}

		if(!strcmp(argv[1], "clr")) { // i am not the monitor any more :(
			printf("upa %s\n", argv[1]);
			return 0;
		}

		//echo back commands like "monitor wakeup/shutup/ping"
		printf("upa %s\n", argv[1]);
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

	return 0;
}


const static cmd_t cmd_monitor = {"monitor", cmd_monitor_func, "nest wireless monitor cmds"};
DECLARE_SHELL_CMD(cmd_monitor)
