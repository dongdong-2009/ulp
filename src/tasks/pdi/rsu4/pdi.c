/*
	feng.ni 2017/04/08 initial version
 */

#include "ulp/sys.h"
#include "shell/cmd.h"
#include "stdio.h"
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include "uart.h"
#include "can.h"
#include "bsp.h"
#include "pdi.h"

/*current pos*/
static int pdi_ecode;
static int pdi_busy; //tesing is busy
static int pdi_mask; //bitor, indicates which rsu is to be test
static int pdi_passed; //bitor, indicates which rsu passed the test
static pdi_report_t pdi_report;
static int pdi_vbat_mv = 13500;

/*in order to speeding up the test, host will scan position B
during pdi is tesing A, so pdi_scan_pos is async with pdi_pos

host changes its scan position only when:
1) pdi ack "UUTA" or "UUTB"
2) GUI panel force selection

*/
static int pdi_pos; //current pdi pos, 0=left, 1=right
static int pdi_scan_pos; //current host scan pos
static int pdi_scan_mask[2];
static char pdi_host_buf[128];
static int pdi_host_itac; //flag of itac "OO" or "NN" cmd received cmd received

static int led_status;
static int led_flash;
static time_t led_timer;

static void pdi_led_init(void)
{
	led_status = 0;
	led_flash = 0;
	led_timer = 0;
}

static void pdi_led_update(void)
{
	if(led_timer == 0)
		return;

	if(time_left(led_timer) > 0)
		return;

	led_timer = time_get(PDI_LED_MS);
	led_status ^= led_flash;
	bsp_led(LED_ALL, led_status);
}

void pdi_led_y(int led_mask)
{
	led_status |= led_mask;
	led_flash &= ~led_mask;
	bsp_led(LED_ALL, led_status);
}

void pdi_led_n(int led_mask)
{
	led_status &= ~led_mask;
	led_flash &= ~led_mask;
	bsp_led(LED_ALL, led_status);
}

void pdi_led_flash(int led_mask)
{
	led_flash |= led_mask;
	led_status |= led_flash;
	bsp_led(LED_ALL, led_status);
	led_timer = time_get(PDI_LED_MS);
}

int pdi_can_tx(const can_msg_t *msg) {
	hexdump("CAN_TX: ", msg->data, msg->dlc);
	bsp_can_bus->flush();
	return bsp_can_bus->send(msg);
}

int pdi_can_rx(can_msg_t *msg) {
	time_t deadline = time_get(PDI_ECU_MS);
	while(1) {
		pdi_mdelay(10);
		if(!bsp_can_bus->recv(msg)) {
			hexdump("CAN_RX: ", msg->data, msg->dlc);
			return 0;
		}

		if(time_left(deadline) < 0) { //timeout
			return -1;
		}
	}
}

static void pdi_host_send(const char *data, int nbytes)
{
	hexdump("HOST_TX: ", data, nbytes);
	for(int i = 0; i < nbytes; i ++) {
		bsp_host_bus.putchar(data[i]);
	}
}

static int pdi_host_printf(const char *fmt, ...)
{
	va_list ap;
	char *pstr = sys_malloc(512);
	int n = 0;

	va_start(ap, fmt);
	n += vsnprintf(pstr + n, 512 - n, fmt, ap);
	va_end(ap);

	pdi_host_send(pstr, n);
	return n;
}

static void pdi_init_action(void)
{
	pdi_led_y(LED_ALL);
	bsp_beep(1);
	pdi_mdelay(200);
	pdi_led_n(LED_ALL);
	bsp_beep(0);
	pdi_mdelay(100);

	for(int i = 0; i < 5; i ++) {
		pdi_led_y(LED_ALL);
		pdi_mdelay(200);
		pdi_led_n(LED_ALL);
		pdi_mdelay(100);
	}
}

static void pdi_start_action(void)
{
	if(pdi_pos == 0) {
		pdi_led_n(LED_LA);
		pdi_led_flash(LED_LY);
	}
	else {
		pdi_led_n(LED_RA);
		pdi_led_flash(LED_RY);
	}
}

static void pdi_stop_action(int passed)
{
	int r = (~passed) & pdi_mask;
	int	g = passed & pdi_mask;

	switch(pdi_ecode) {
	case ERROR_NO:
	case ERROR_FUNC:
		break;
	default:
		r = 0x0f; //all failed
		break;
	}

	//led
	if(pdi_pos == 0) {
		pdi_led_n(LED_LA);
		pdi_led_y((g << 8) | (r << 0));
	}
	else {
		pdi_led_n(LED_RA);
		pdi_led_y((g << 12) | (r << 4));
	}

	//beep
	if(pdi_ecode == ERROR_NO) {
		bsp_beep(1);
		pdi_mdelay(200);
		bsp_beep(0);
		pdi_mdelay(200);
		bsp_beep(1);
		pdi_mdelay(200);
		bsp_beep(0);
	}
	else {
		bsp_beep(1);
		pdi_mdelay(1000);
		bsp_beep(0);
	}
}

static int pdi_host_get(void)
{
	static time_t host_deadline = 0;
	static int host_bytes = 0;
	int bytes = 0;

	while(bsp_host_bus.poll() > 0) {
		host_deadline = time_get(PDI_HOST_MS);
		pdi_host_buf[host_bytes] = bsp_host_bus.getchar();
		host_bytes ++;
	}

	if(host_bytes > 0) {
		if (time_left(host_deadline) < 0) {
			//interframe interval is found
			bytes = host_bytes;
			host_bytes = 0;
			host_deadline = time_get(0);
			hexdump("HOST_RX: ", pdi_host_buf, bytes);
		}
	}

	return bytes;
}

static void pdi_host_update(void)
{
	int nbytes = pdi_host_get();
	if(nbytes == 0)
		return;

	if(!strncmp(pdi_host_buf, "3U", 2)) { //read fixture id
		pdi_host_printf(pdi_fixture_id);
		//debug("PDI: fixture id = %s\n", pdi_fixture_id);
	}
	else if(!strncmp(pdi_host_buf, "AB", 2)) { //read vbat
		float vbat = pdi_vbat_mv / 1000.0;
		pdi_host_printf("%3.1f", vbat);
		//debug("PDI: vbat = %3.1f V\n", vbat);
	}
	else if(!strncmp(pdi_host_buf, "\x01\x01", 2)) { //enter into download mode
		pdi_host_printf("DL-OK");
		//debug("PDI: DL-OK\n");
	}
	else if(!strncmp(pdi_host_buf, "\x0c\x0a", 2)) { //barcode been scanned
		pdi_scan_mask[pdi_scan_pos] <<= 1;
		pdi_scan_mask[pdi_scan_pos] |= 1;

		//flash yellow led
		if(pdi_scan_pos == 0) {
			pdi_led_n(LED_LA);
			pdi_led_flash(pdi_scan_mask[pdi_scan_pos] << 16);
		}
		else {
			pdi_led_n(LED_RA);
			pdi_led_flash(pdi_scan_mask[pdi_scan_pos] << 20);
		}
	}
	else if(!strncmp(pdi_host_buf, "SA", 2)) { //force into left pos
		if(pdi_busy == 0) {
			pdi_pos = 0; //???
			pdi_scan_pos = 0;
			pdi_scan_mask[0] = 0;
		}
	}
	else if(!strncmp(pdi_host_buf, "SB", 2)) { //force into left pos
		if(pdi_busy == 0) {
			pdi_pos = 1; //???
			pdi_scan_pos = 1;
			pdi_scan_mask[1] = 0;
		}
	}
	else if(!strncmp(pdi_host_buf, "OO", 2)) { //ITAC OK
		pdi_host_itac = 1;
	}
	else if(!strncmp(pdi_host_buf, "NN", 2)) { //ITAC NG
		pdi_host_itac = 1;
		pdi_ecode = ERROR_ITAC;
	}
	else if(!strncmp(pdi_host_buf, "RE", 2)) { //Read ECU Error Code
		pdi_host_send(pdi_report.datalog, 8);
	}
}

static void pdi_wait_host_itac(void)
{
#if PDI_HOST == 1
	time_t deadline = time_get(PDI_ITAC_MS);
	while(pdi_host_itac == 0) {
		pdi_mdelay(10);
		if(time_left(deadline) < 0) {
			//wait host itac response "OO"/"NN" timeout ... :(
			pdi_ecode = ERROR_ITAC;
			break;
		}
	}
#endif
}

static void pdi_init(void)
{
	pdi_busy = 0;
	pdi_pos = 0;
	pdi_scan_pos = 0;
	pdi_scan_mask[0] = 0; //nothing to test
	pdi_scan_mask[1] = 0; //nothing to test

	uart_cfg_t cfg = {
		.baud = 9600,
	};

	bsp_host_bus.init(&cfg);
	bsp_host_bus.putchar('!'); //for test the usart

	//default
	bsp_select_ecu(0);
	bsp_select_can(0);
	bsp_select_rsu(pdi_pos);

	pdi_led_init();
	pdi_init_action();
}

static void pdi_update(void)
{
	pdi_led_update();
	pdi_host_update();
}

void pdi_mdelay(int ms)
{
	time_t deadline = time_get(ms);
	while(time_left(deadline) > 0) {
		sys_update();
		pdi_update();
	}
}

static void pdi_wait_push(void)
{
	int push_timer = 0;
	pdi_busy = 0;

	/*wait for host config & fixture push*/
	while(push_timer < PDI_PUSH_MS) {
		pdi_mdelay(10);
		if(pdi_scan_mask[pdi_pos] > 0) {
			int ready = bsp_pos_status(pdi_pos);
			push_timer += (ready) ? 10 : -10;
			push_timer = (push_timer < 0) ? 0 : push_timer;
		}
	}

	pdi_busy = 1;
	pdi_ecode = ERROR_NO;
	pdi_mask = pdi_scan_mask[pdi_pos];
	pdi_scan_mask[pdi_pos] = 0;
}

static int pdi_analysis_report(void)
{
	int passed = 0, ecode = ERROR_NO;
	pdi_result_t result = pdi_report.result;

	const char *pos = (pdi_pos == 1) ? "R" : "L";

	if(pdi_mask & 0x01) {
		if(result.rsu1 != 0) {
			debug("%s-RSU1: %d, FAIL!\n",  pos, result.rsu1);
		}
		else {
			passed |= 0x01;
			debug("%s-RSU1: %d, PASS!\n",  pos, result.rsu1);
		}
	}

	if(pdi_mask & 0x02) {
		if(result.rsu2 != 0) {
			debug("%s-RSU2: %d, FAIL!\n",  pos, result.rsu2);
		}
		else {
			passed |= 0x02;
			debug("%s-RSU2: %d, PASS!\n",  pos, result.rsu2);
		}
	}

	if(pdi_mask & 0x04) {
		if(result.rsu3 != 0) {
			debug("%s-RSU3: %d, FAIL!\n",  pos, result.rsu3);
		}
		else {
			passed |= 0x04;
			debug("%s-RSU3: %d, PASS!\n",  pos, result.rsu3);
		}
	}

	if(pdi_mask & 0x08) {
		if(result.rsu4 != 0) {
			debug("%s-RSU4: %d, FAIL!\n",  pos, result.rsu4);
		}
		else {
			passed |= 0x08;
			debug("%s-RSU4: %d, PASS!\n",  pos, result.rsu4);
		}
	}

	pdi_passed = passed;
	if(pdi_mask ^ passed) {
		ecode = ERROR_FUNC;
	}
	return ecode;
}

static void pdi_verify(void)
{
	int pt204_mv[4];
	int ecode, status;
	const char *UUTx = (pdi_pos == 0) ? "UUTA" : "UUTB";

	//check the rsu placement
	status = bsp_rsu_status(pdi_pos, pt204_mv);
	if(status != pdi_mask) {
		int ns = bitcount(status);
		int nm = bitcount(pdi_mask);
		pdi_ecode = (ns != nm) ? ERROR_NUM : ERROR_POSITION;
		debug("rsu: expect %02x, detected %02x\n", pdi_mask, status);
		for(int i = 0; i < 4; i ++) debug("pt204: ch%d = %04d mV\n", i, pt204_mv[i]);
	}
	else {
		pdi_scan_pos = !pdi_scan_pos;
		pdi_host_printf(UUTx);
		bsp_select_rsu(pdi_pos);
		pdi_start_action();

		for(int try = 0; try < 2; try ++) {
			//power on
			bsp_swbat(1);
			pdi_mdelay(7000);

			ecode = pdi_test(pdi_pos, pdi_mask, &pdi_report);
			if(ecode) pdi_ecode = ERROR_ECUNG;
			else pdi_ecode = pdi_analysis_report();

			//power off
			bsp_swbat(0);

			//test passed?
			if(pdi_ecode == ERROR_NO) {
				break;
			}

			//fixture is pulled out???
			int ready = bsp_pos_status(pdi_pos);
			if(!ready) {
				pdi_ecode = ERROR_BACHU;
				break;
			}

			//retry ...
			pdi_mdelay(1000);
			pdi_host_itac = 0;
			pdi_host_printf("%sNG", UUTx);
			pdi_wait_host_itac();
			if(pdi_ecode != ERROR_ITAC) {
				break;
			}
		}
	}

	int pdi_pos_new = pdi_pos;

	//test finished
	switch(pdi_ecode) {
	case ERROR_NUM:
		pdi_host_printf("N-%s", UUTx);
		break;
	case ERROR_POSITION:
		pdi_host_printf("P-%s", UUTx);
		break;

	case ERROR_ECUNG: /*infact host not fully support it*/
		pdi_host_printf("ECU-NG");
		break;
	case ERROR_BACHU:
		pdi_pos_new = !pdi_pos;
		pdi_host_itac = 0;
		pdi_host_printf("%sNG-BRK", UUTx);
		pdi_wait_host_itac();
		break;
	case ERROR_FUNC:
		pdi_pos_new = !pdi_pos;
		//already sent to host during retry ...
		break;
	case ERROR_NO:
		pdi_pos_new = !pdi_pos;
		pdi_host_itac = 0;
		pdi_host_printf("%sOK", UUTx);
		pdi_wait_host_itac();
		break;

	case ERROR_ITAC:
		break;

	default:
		break;
	}

	//indicate to operator
	pdi_stop_action(pdi_passed);
	pdi_pos = pdi_pos_new;
}

int main(void)
{
	sys_init();
	bsp_init();
	pdi_init();
	while(1) {
		pdi_wait_push();
		pdi_verify();
	}
}

#if 1
static int cmd_pdi_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"pdi list		list all global var value\n"
		"pdi run [pos] [mask]\n"
		"pdi ledy mask		mask=0x00YYGGRR right|left\n"
		"pdi ledn mask		mask=0x00YYGGRR right|left\n"
		"pdi ledf mask		mask=0x00YYGGRR right|left\n"
	};

	if (argc < 2) {
		printf("%s", usage);
		return 0;
	}

	if (!strcmp(argv[1], "list")) {
		printf("%16s = %d\n", "pdi_ecode", pdi_ecode);
		printf("%16s = %d\n", "pdi_pos", pdi_pos);
		printf("%16s = %d\n", "pdi_scan_pos", pdi_scan_pos);
		printf("%16s = 0x%02x\n", "pdi_mask", pdi_mask);
		printf("%16s = [0x%02x, 0x%02x]\n", "pdi_scan_mask", pdi_scan_mask[0], pdi_scan_mask[1]);
	}

	if (!strcmp(argv[1], "run")) {
		int pos = 0;
		int msk = 0x0f;
		if(argc >= 3) {
			if(!strncmp(argv[2], "0x", 2)) sscanf(argv[2], "0x%x", &pos);
			else sscanf(argv[2], "%d", &pos);
		}
		if(argc >= 4) {
			if(!strncmp(argv[3], "0x", 2)) sscanf(argv[3], "0x%x", &msk);
			else sscanf(argv[3], "%d", &msk);
		}

		pdi_pos = pos;
		pdi_scan_mask[pos] = msk;
		//push the fixture, then .. go!
	}

	if (!strcmp(argv[1], "ledy")) {
		int mask = 0;
		if(!strncmp(argv[2], "0x", 2)) sscanf(argv[2], "0x%x", &mask);
		else sscanf(argv[2], "%d", &mask);
		pdi_led_y(mask);
	}

	if (!strcmp(argv[1], "ledn")) {
		int mask = 0;
		if(!strncmp(argv[2], "0x", 2)) sscanf(argv[2], "0x%x", &mask);
		else sscanf(argv[2], "%d", &mask);
		pdi_led_n(mask);
	}

	if (!strcmp(argv[1], "ledf")) {
		int mask = 0;
		if(!strncmp(argv[2], "0x", 2)) sscanf(argv[2], "0x%x", &mask);
		else sscanf(argv[2], "%d", &mask);
		pdi_led_flash(mask);
	}

	return 0;
}

const cmd_t cmd_pdi = {"pdi", cmd_pdi_func, "pdi debug cmds"};
DECLARE_SHELL_CMD(cmd_pdi)
#endif

