/*
 *	miaofng@2012 uart debugger initial version
 */

#include "config.h"
#include "shell/cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "uart.h"
#include "ulp_time.h"
#include "linux/list.h"
#include "ulp/sys.h"
#include "common/debounce.h"
#include <ctype.h>

static struct {
	unsigned baud;
	unsigned intf_ms : 16; //inter-frame ms
	unsigned port : 8;
	unsigned tx : 1; //send enable
	unsigned hex : 1; //text mode or hex mode?
	unsigned debug : 1;
} uart_termios;
static const uart_bus_t *uart = NULL;
static time_t uart_timer;

static void uart_cmd_init(void)
{
	uart_cfg_t cfg;
	cfg.baud = uart_termios.baud;
#ifdef CONFIG_DRIVER_UART0
	uart = (uart_termios.port == 'g') ? &uartg : uart;
#endif
#ifdef CONFIG_DRIVER_UART0
	uart = (uart_termios.port == 0) ? &uart0 : uart;
#endif
#ifdef CONFIG_DRIVER_UART1
	uart = (uart_termios.port == 1) ? &uart1 : uart;
#endif
#ifdef CONFIG_DRIVER_UART2
	uart = (uart_termios.port == 2) ? &uart2 : uart;
#endif
#ifdef CONFIG_DRIVER_UART3
	uart = (uart_termios.port == 3) ? &uart3 : uart;
#endif
#ifdef CONFIG_DRIVER_UART3
	uart = (uart_termios.port == 3) ? &uart3 : uart;
#endif
	uart->init(&cfg);
	uart_timer = 0;
}

static void uart_cmd_update(void)
{
	const char *format = "%c";
	if(uart_termios.debug) { //send debug frame
		static time_t uart_debug_timer;
		if(time_left(uart_debug_timer) < 0) {
			uart_debug_timer = time_get(uart_termios.intf_ms);
			for(int i = 0; i < 10; i ++) {
				uart->putchar(i);
			}
		}
	}

	#if 1
	if(uart_termios.hex) {
		format = " %02x";
	}

	while(uart->poll()) {
		//insert frame header
		if(time_left(uart_timer) < 0) {
			printf("\n%08d :", time_get(0));
		}

		int c = uart->getchar();
		printf(format, ((unsigned)c) & 0xff);
		uart_timer = time_get(uart_termios.intf_ms);
	}
	#endif

	#if 0
	while(uart->poll()) {
		static int ms, ms_min = 0, ms_max = 0, ms_avg = 0;
		if(uart_timer != 0) {
			ms = -time_left(uart_timer);
			if(ms > uart_termios.intf_ms) {
				if(ms_min == 0) {
					ms_min = ms;
					ms_avg = ms;
				}
				ms_min = (ms < ms_min) ? ms : ms_min;
				ms_max = (ms > ms_max) ? ms : ms_max;
				ms_avg = (ms_avg + ms) / 2;
				printf("T = %dmS, %dmS, %dmS\n", ms_avg, ms_min, ms_max);
			}
		}
		uart->getchar();
		uart_timer = time_get(0);
	}
	#endif
}

static int cmd_uart_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"uart			uart monitor with uart1, 19200bps, rx only and hex mode\n"
		"uart -p uart2		uart monitor with port = uart2\n"
		"uart -b 10400		uart monitor with port = uart1 and baudrate = 10400\n"
		"uart -p uart2 -i 10	uart monitor with inter-frame not more than 10mS\n"
		"uart -w  		uart monitor send support\n"
		"uart -m txt		uart monitor in txt(or hex) mode\n"
		"uart -d -i 10		uart periodly send 0-9 10bytes frame, T=10ms"
	};

	if(argc > 0) {
		uart_termios.port = 1;
		uart_termios.baud = 19200;
		uart_termios.intf_ms = 5;
		uart_termios.tx = 0;
		uart_termios.hex = 1;
		uart_termios.debug = 0;

		int v, e = 0;
		for(int i = 1; i < argc; i ++) {
			e += (argv[i][0] != '-');
			switch(argv[i][1]) {
			case 'p':
				i ++;
				v = (isdigit(argv[i][4])) ? atoi(argv[i]+4) : argv[i][4];
				if(((v >= 0) && (v <= 15)) || (v == 'g')) uart_termios.port = v;
				else e ++;
				break;
			case 'b':
				v = atoi(argv[++i]);
				if(v > 0) uart_termios.baud = v;
				else e++;
				break;
			case 'i':
				v = atoi(argv[++i]);
				if(v > 0) uart_termios.intf_ms = v;
				else e ++;
				break;
			case 'w':
				uart_termios.tx = 1;
				break;
			case 'm':
				i ++;
				if(!strcmp(argv[i], "txt")) uart_termios.hex = 0;
				else if (!strcmp(argv[i], "hex")) uart_termios.hex = 1;
				else e ++;
				break;
			case 'd':
				uart_termios.debug = 1;
				break;
			default:
				e ++;
			}
		}

		if(e) {
			printf("%s", usage);
			return 0;
		}

		uart_cmd_init();
		return 1;
	}

	uart_cmd_update();
	return 1;
}


const cmd_t cmd_uart = {"uart", cmd_uart_func, "uart debug cmds"};
DECLARE_SHELL_CMD(cmd_uart)
