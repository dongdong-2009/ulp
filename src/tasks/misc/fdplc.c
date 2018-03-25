/*
*
* miaofng@2017-07-16 routine for vwplc
* 1, vwplc system includes vwplc and vwhost
* 2, all cylinders are controled by vwhost, vwplc acts like a plc io extend module
* 3, handshake between test program and vwplc: rdy4tst/test_end/test_ng
* 4, vwplc can not request test unit ID
*
* miaofng@2017-10-01 arch change dueto poll mode too slow
* 1, cylinders are controlled by test unit, except pull/push by vwhost
* 2, vwplc send can msg to vwhost automatically in case of any sensor signal change
*
* miaofng@2018-02-20 port to fdplc/pwr system
* 1, add fdpwr board function into fdplc
*
* miaofng@2018-02-26 failure process system
* 1, iox spi communication fail > red flash - 1
* 2, nvm access/configuration fail > red flash - 2
* 3, eload calibration data fail > red flash - 3
* 4, can send fail > deadwait, red flash - 4
* 5, plc host lost > red flash - 5
* 6, test stand died > yellow on
*
*/

#include "ulp/sys.h"
#include "led.h"
#include <string.h>
#include <ctype.h>
#include "shell/shell.h"
#include "can.h"
#include "common/bitops.h"
#include "stm32f10x.h"
#include "gpio.h"
#include <stdlib.h>
#include "at24cxx.h"
#include "fdplc.h"

static const mcp23s17_t fdplc_mcp = {.bus = &spi1, .idx = SPI_1_NSS, .ck_mhz = 4, .hwaddr = 1};
static const at24cxx_t fdplc_24c02 = {.bus = &i2cs, .page_size = 8};
static const can_bus_t *fdplc_can = &can1;
static can_msg_t fdplc_rxmsg;
static can_msg_t fdplc_txmsg;
static fdcmd_t *fdplc_rxdat = (fdcmd_t *) fdplc_rxmsg.data;
static fdrpt_t *fdplc_txdat = (fdrpt_t *) fdplc_txmsg.data;
static int fdplc_id_assign;
static int fdplc_id_sql; //>= 0: rdy4tst
static time_t fdplc_wdt = 0;

static int fdplc_sensors;
static int fdplc_gpio_csel;
static int fdplc_gpio_rsel;

static fdplc_status_t fdplc_status;
static fdplc_eeprom_t fdplc_eeprom;

static int led_r, led_g, led_x;
static int ext_r, ext_g, ext_y;

//for fdiox board dynamic detection
static const unsigned char fdiox_magic = 0xca; //1100 1010
static const unsigned char fdiox_mask = 0xfe; //lsb is used by led_x

#define fdmdl_list_size (sizeof(fdmdl_list) / sizeof(fdmdl_list[0]))
#define fdplc_model_is_valid(model) (model < fdmdl_list_size)
static const fdmdl_t fdmdl_list[] = FDMDL_LIST;

static int fdplc_w_img;
static int fdplc_w_msk;

/*delayed write, todo gpio access always in main thread*/
static void fdplc_wimg(int img, int msk)
{
	fdplc_w_msk |= msk;
	fdplc_w_img &= ~ msk;
	fdplc_w_img |= img & msk;
	//gpio_wimg(img, msk);
}

static int fdplc_eeprom_save(int bytes)
{
	int alen = AT24C02_ALEN;
	int ecode = at24cxx_WriteBuffer(&fdplc_24c02, AT24C02_ADDR, 0, alen, &fdplc_eeprom, bytes);
	return ecode;
}

/*callback @sys_init*/
void __sys_init(void)
{
	gpio_mcp_init(&fdplc_mcp);

	//index 00-07, iox sensors
	GPIO_BIND_INV(GPIO_IPU, mcp0:PA00, UE ) //IOX-IN01
	GPIO_BIND_INV(GPIO_IPU, mcp0:PA01, S02) //IOX-IN02
	GPIO_BIND_INV(GPIO_IPU, mcp0:PA02, SC+) //IOX-IN03
	GPIO_BIND_INV(GPIO_IPU, mcp0:PA03, SC-) //IOX-IN04
	GPIO_BIND_INV(GPIO_IPU, mcp0:PA04, SR+) //IOX-IN05
	GPIO_BIND_INV(GPIO_IPU, mcp0:PA05, SR-) //IOX-IN06
	GPIO_BIND_INV(GPIO_IPU, mcp0:PA06, SF+) //IOX-IN07
	GPIO_BIND_INV(GPIO_IPU, mcp0:PA07, SF-) //IOX-IN08

	GPIO_FILT(UE , FDPLC_IOF_MS)
	GPIO_FILT(S02, FDPLC_IOF_MS)
	GPIO_FILT(SC+, FDPLC_IOF_MS)
	GPIO_FILT(SC-, FDPLC_IOF_MS)
	GPIO_FILT(SR+, FDPLC_IOF_MS)
	GPIO_FILT(SR-, FDPLC_IOF_MS)
	GPIO_FILT(SF+, FDPLC_IOF_MS)
	GPIO_FILT(SF-, FDPLC_IOF_MS)

	//index 08-15, iox sensors
	GPIO_BIND_INV(GPIO_IPU, mcp0:PB00, SB+) //IOX-IN09
	GPIO_BIND_INV(GPIO_IPU, mcp0:PB01, SB-) //IOX-IN10
	GPIO_BIND_INV(GPIO_IPU, mcp0:PB02, SA+) //IOX-IN11
	GPIO_BIND_INV(GPIO_IPU, mcp0:PB03, SA-) //IOX-IN12
	GPIO_BIND_INV(GPIO_IPU, mcp0:PB04, S13) //IOX-IN13
	GPIO_BIND_INV(GPIO_IPU, mcp0:PB05, S14) //IOX-IN14
	GPIO_BIND_INV(GPIO_IPU, mcp0:PB06, S15) //IOX-IN15
	GPIO_BIND_INV(GPIO_IPU, mcp0:PB07, S16) //IOX-IN16

	GPIO_FILT(SB+, FDPLC_IOF_MS)
	GPIO_FILT(SB-, FDPLC_IOF_MS)
	GPIO_FILT(S11, FDPLC_IOF_MS)
	GPIO_FILT(S12, FDPLC_IOF_MS)
	GPIO_FILT(S13, FDPLC_IOF_MS)
	GPIO_FILT(S14, FDPLC_IOF_MS)
	GPIO_FILT(S15, FDPLC_IOF_MS)
	GPIO_FILT(S16, FDPLC_IOF_MS)

	//index 16-19, plc sensors
	GPIO_BIND_INV(GPIO_IPU, PC14, SM+)
	GPIO_BIND_INV(GPIO_IPU, PC15, SM-)
	GPIO_BIND(GPIO_PP0, PC02, CM+)
	GPIO_BIND(GPIO_PP0, PC03, CM-)

	GPIO_FILT(SM+, FDPLC_IOF_MS)
	GPIO_FILT(SM-, FDPLC_IOF_MS)

	//index 20-27, iox valve ctrl
	GPIO_BIND(GPIO_PP0, mcp1:PA00, CC) //IOX-OUT01
	GPIO_BIND(GPIO_PP0, mcp1:PA01, CR) //IOX-OUT02
	GPIO_BIND(GPIO_PP0, mcp1:PA02, CF) //IOX-OUT03
	GPIO_BIND(GPIO_PP0, mcp1:PA03, CB) //IOX-OUT04
	GPIO_BIND(GPIO_PP0, mcp1:PA04, CA) //IOX-OUT05
	GPIO_BIND(GPIO_PP0, mcp1:PA05, C6) //IOX-OUT06
	GPIO_BIND(GPIO_PP0, mcp1:PA06, C7) //IOX-OUT07
	GPIO_BIND(GPIO_PP0, mcp1:PA07, C8) //IOX-OUT08

	//index 28-30, plc valve
	ext_r = GPIO_BIND(GPIO_PP0, PA00, LR)
	ext_g = GPIO_BIND(GPIO_PP0, PA01, LG)
	ext_y = GPIO_BIND(GPIO_PP0, PA02, LY)

	//misc
	fdplc_gpio_rsel = GPIO_BIND_INV(GPIO_IPU, PC10, RSEL)
	fdplc_gpio_csel = GPIO_BIND_INV(GPIO_IPU, PC11, CSEL)
	GPIO_FILT(RSEL, 10)
	GPIO_FILT(CSEL, 10)

	led_r = GPIO_BIND(GPIO_PP0, PC00, LED_R)
	led_g = GPIO_BIND(GPIO_PP0, PC01, LED_G)
	led_x = GPIO_BIND(GPIO_PP0, mcp1:PB00, LED_X) //iox rj45 indicator
}

/*callback @led driver*/
void led_hwSetStatus(led_t led, led_status_t status)
{
	int yes = (status == LED_ON) ? 1 : 0;
	switch(led) {
	case LED_G: gpio_set_h(led_g, yes); break;
	case LED_X: gpio_set_h(led_x, yes); break;

	case LED_ERR:
	case LED_EXT_R: gpio_set_h(ext_r, yes); break;
	case LED_EXT_G: gpio_set_h(ext_g, yes); break;
	case LED_EXT_Y: gpio_set_h(ext_y, yes); break;
	default:
		break;
	}
}

void fdplc_on_event(fdplc_event_t event)
{
	__disable_irq();
	switch(event) {
	case FDPLC_EVENT_WDTY:
		fdplc_wdt = time_get(FDPLC_WDT_MS);
		fdplc_status.ecode &= ~(1 << FDPLC_E_WDT);
		fdplc_status.wdt_y = 1;
		break;
	case FDPLC_EVENT_WDTN:
		fdplc_status.wdt_y = 0;
		fdplc_status.ecode &= ~(1 << FDPLC_E_WDT);
		break;
	case FDPLC_EVENT_WDT_TIMEOUT:
		if(fdplc_status.wdt_y) {
			fdplc_status.ecode |= (1 << FDPLC_E_WDT);
		}
		break;
	case FDPLC_EVENT_WDT_OK:
		if(fdplc_status.wdt_y) {
			fdplc_status.ecode &= ~(1 << FDPLC_E_WDT);
		}
		break;

	case FDPLC_EVENT_INIT:
		memset(&fdplc_status, 0x00, sizeof(fdplc_status));
		fdplc_status.wdt_y = 1;
		fdplc_status.ecode &= ~(1 << FDPLC_E_WDT);
		fdplc_wdt = time_get(FDPLC_WDT_MS);
		break;
	case FDPLC_EVENT_ASSIGN:
		fdplc_status.assigned = 1;
		break;
	case FDPLC_EVENT_PASS:
		fdplc_status.test_ng = 0;
		fdplc_status.test_end = 1;
		fdplc_status.test_status = FDPLC_STATUS_PASS;
		break;
	case FDPLC_EVENT_FAIL:
		fdplc_status.test_ng = 1;
		fdplc_status.test_end = 1;
		fdplc_status.test_status = FDPLC_STATUS_FAIL;
		break;
	case FDPLC_EVENT_TEST:
		fdplc_status.test_end = 0;
		fdplc_status.test_status = FDPLC_STATUS_TEST;
		break;

	case FDPLC_EVENT_OFFLINE: fdplc_status.test_offline = 1; break;
	case FDPLC_EVENT_ONLINE: fdplc_status.test_offline = 0; break;

	case FDPLC_EVENT_HOST_LOST: fdplc_status.ecode |= (1 << FDPLC_E_HOST); break;
	case FDPLC_EVENT_HOST_OK: fdplc_status.ecode &= ~(1 << FDPLC_E_HOST); break;
	case FDPLC_EVENT_CAN_SEND_TIMEOUT: fdplc_status.ecode |= (1 << FDPLC_E_SEND); break;
	case FDPLC_EVENT_CAN_SEND_OK: fdplc_status.ecode &= ~(1 << FDPLC_E_SEND); break;

	case FDPLC_EVENT_FAIL_CAL: fdplc_status.ecode |= (1 << FDPLC_E_CAL); break;
	case FDPLC_EVENT_FAIL_MCP: fdplc_status.ecode |= (1 << FDPLC_E_MCP); break;
	case FDPLC_EVENT_FAIL_NVM: fdplc_status.ecode |= (1 << FDPLC_E_NVM); break;
	case FDPLC_EVENT_GOOD_NVM: fdplc_status.ecode &= ~(1 << FDPLC_E_NVM); break;
	default:
		break;
	}
	__enable_irq();
}

static int fdplc_rimg(void)
{
	/*include all plc & iox valves and its sensors
	exclude gpio like: LR/LG/LY/CSEL/RSEL/..
	*/
	int img = gpio_rimg(0x0fffffff);
	img &= 0x0FFFFFFF;
	img |= fdplc_status.test_end ? (1 << 31) : 0;
	img |= fdplc_status.test_ng ? (1 << 30) : 0;
	img |= fdplc_status.test_offline ? (1 << 29) : 0;
	img |= fdplc_status.ecode ? (1 << 28) : 0;
	return img;
}

static void fdplc_can_send(const can_msg_t *msg)
{
	gpio_set_h(led_r, 1);
	while(fdplc_can->send(msg));
	gpio_set_h(led_r, 0);
}

static void fdplc_can_handler(void)
{
	int rsel, csel, img, msk;
	if(fdplc_status.assigned) {
		if((fdplc_rxdat->dst != 0) && (fdplc_rxdat->dst != fdplc_id_assign))
			return;
	}
	else {
		//only accept CMD_SCAN
		if(fdplc_rxdat->cmd != FDCMD_SCAN)
			return;
	}

	fdcmd_type_t cmd_type = (fdcmd_type_t) fdplc_rxdat->cmd;
	switch(cmd_type) {
	case FDCMD_SCAN:
		rsel = gpio_get_h(fdplc_gpio_rsel);
		csel = gpio_get_h(fdplc_gpio_csel);
		if((rsel == 1) && (csel == 1)) {
			fdplc_id_assign = fdplc_rxdat->dst;
			fdplc_txmsg.id = CAN_ID_UNIT(fdplc_id_assign);
			fdplc_txmsg.dlc = 8;
			fdplc_on_event(FDPLC_EVENT_ASSIGN);
		}

		//send the status to vwhost
		if(fdplc_status.assigned) {
			fdplc_txdat->src = fdplc_id_assign;
			fdplc_txdat->cmd = fdplc_rxdat->cmd;
			fdplc_txdat->model = fdplc_eeprom.model;
			fdplc_txdat->pushed = fdplc_eeprom.pushed;
			fdplc_txdat->sensors = fdplc_sensors;
			fdplc_can_send(&fdplc_txmsg);
		}
		break;

	case FDCMD_MOVE:
		//18 => index@CM+, right shifted by fdhost inside cmd move
		img = fdplc_rxdat->move.img << 18;
		msk = fdplc_rxdat->move.msk << 18;
		fdplc_wimg(img, msk);
		break;

	case FDCMD_TEST:
		fdplc_id_sql = fdplc_rxdat->test.sql_id;
		fdplc_on_event(FDPLC_EVENT_TEST);
		break;
	case FDCMD_PASS:
		fdplc_on_event(FDPLC_EVENT_PASS);
		break;
	case FDCMD_FAIL:
		fdplc_on_event(FDPLC_EVENT_FAIL);
		break;
	case FDCMD_WDTY:
		fdplc_on_event(FDPLC_EVENT_WDTY);
		break;
	case FDCMD_WDTN:
		fdplc_on_event(FDPLC_EVENT_WDTN);
		break;

	case FDCMD_UUID:
		if(fdplc_status.assigned) {
			unsigned uuid = *(unsigned *)(0X1FFFF7E8);
			fdplc_txdat->src = fdplc_id_assign;
			fdplc_txdat->cmd = fdplc_rxdat->cmd;
			fdplc_txdat->model = fdplc_eeprom.model;
			fdplc_txdat->pushed = fdplc_eeprom.pushed;
			fdplc_txdat->sensors = uuid;
			fdplc_can_send(&fdplc_txmsg);
		}
		break;

	case FDCMD_POLL:
		if(fdplc_status.assigned) {
			fdplc_txdat->src = fdplc_id_assign;
			fdplc_txdat->cmd = fdplc_rxdat->cmd;
			fdplc_txdat->model = fdplc_eeprom.model;
			fdplc_txdat->pushed = fdplc_eeprom.pushed;
			fdplc_txdat->sensors = fdplc_sensors;
			fdplc_can_send(&fdplc_txmsg);
		}
		break;

	default:
		//only identified cmd to be responsed
		return;
	}
}

static int fdplc_on_set_CMp(const gpio_t *gpio, int high)
{
	if(high) {
		gpio_set("CM-", 0);
	}
	return 0;
}

static int fdplc_on_set_CMn(const gpio_t *gpio, int high)
{
	if(high) {
		gpio_set("CM+", 0);
	}
	return 0;
}

static int fdplc_on_set_CF(const gpio_t *gpio, int high)
{
	if(high) {
		fdplc_eeprom.pushed += 1;
		fdplc_eeprom_save(sizeof(fdplc_eeprom.pushed));
	}
	return 0;
}

void fdplc_init(void)
{
	const can_cfg_t cfg = {.baud = CAN_BAUD, .silent = 0};
	const can_filter_t filter[] = {
		{.id = CAN_ID_HOST, .mask = 0xffff, .flag = 0},
		{.id = CAN_ID_SCAN, .mask = 0xffff, .flag = 0},
	};
	memset(&fdplc_rxmsg, 0x00, sizeof(fdplc_rxmsg));
	memset(&fdplc_txmsg, 0x00, sizeof(fdplc_txmsg));
	fdplc_can->init(&cfg);
	fdplc_can->filt(filter, 2);

	/*!!! to usePreemptionPriority, group must be config first
	systick priority !must! use  NVIC_SetPriority() to set
	*/
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	NVIC_SetPriority(SysTick_IRQn, 0);

	NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	CAN_ClearITPendingBit(CAN1, CAN_IT_FMP0);
	CAN_ITConfig(CAN1, CAN_IT_FMP0, ENABLE);

	//fixture init action
	fdplc_wdt = 0;
	fdplc_status.ecode = 0;
	fdplc_id_assign = 0;
	fdplc_sensors = fdplc_rimg();
	fdplc_w_img = 0;
	fdplc_w_msk = 0;

	//indicator init
	led_combine((1<<LED_EXT_R)|(1<<LED_EXT_G)|(1<<LED_EXT_Y)|(1 << LED_ERR));
	led_flash(LED_X);

	gpio_on_set("CM+", fdplc_on_set_CMp);
	gpio_on_set("CM-", fdplc_on_set_CMn);
	gpio_on_set("CF", fdplc_on_set_CF);

	//global status
	fdplc_on_event(FDPLC_EVENT_INIT);

	//for mcp detection, PortB@MCP1 of iox board is unused
	mcp23s17_WriteByte(&fdplc_mcp, ADDR_OLATB, fdiox_magic);
	if(led_x == GPIO_NONE) {
		fdplc_on_event(FDPLC_EVENT_FAIL_MCP);
		sys_error("iox mcp23s17!!!");
	}

	memset(&fdplc_eeprom, 0xff, sizeof(fdplc_eeprom));
	at24cxx_Init(&fdplc_24c02);
	at24cxx_ReadBuffer(&fdplc_24c02, AT24C02_ADDR, 0x00, AT24C02_ALEN, &fdplc_eeprom, sizeof(fdplc_eeprom));
	if((fdplc_eeprom.magic != FDPLC_NVM_MAGIC) || (!fdplc_model_is_valid(fdplc_eeprom.model))) {
		fdplc_on_event(FDPLC_EVENT_FAIL_NVM);
		sys_error("iox eeprom!!!");
	}

	int model = (fdplc_eeprom.magic == FDPLC_NVM_MAGIC) ? fdplc_eeprom.model : -1;
	const char *customer = "invalid";
	const char *product = "invalid";
	if(fdplc_model_is_valid(model)) {
		customer = fdmdl_list[model].customer;
		product = fdmdl_list[model].product;
	}

	printf("fdplc_eeprom.magic = 0x%04x\n", fdplc_eeprom.magic);
	printf("fdplc_eeprom.pushed = %d\n", fdplc_eeprom.pushed);
	printf("fdplc_eeprom.model = %d\n", model);
	printf("fdplc_eeprom.model.customer = %s\n", customer);
	printf("fdplc_eeprom.model.product = %s\n", product);
	printf("fdplc_status.ecode = 0x%04x\n", fdplc_status.ecode);
}

/*
* LED_R CAN SEND FAIL
* LED_G FLASH: twinkling light
* LED_X FLASH: twinkling light
* EXT_R TEST FAIL(FLASH: ECODE)
* EXT_G TEST PASS/READY
* EXT_Y UNIT DIED(FLASH: TEST IS BUSY)
*/
void fdplc_status_update(void)
{
	if(fdplc_status.ecode & (1 << FDPLC_E_HOST)) {
		led_error(FDPLC_E_HOST);
	}
	else if(fdplc_status.ecode & (1 << FDPLC_E_SEND)) {
		led_error(FDPLC_E_SEND);
	}
	else if(fdplc_status.ecode & (1 << FDPLC_E_MCP)) {
		led_error(FDPLC_E_MCP);
	}
	else if(fdplc_status.ecode & (1 << FDPLC_E_NVM)) {
		led_error(FDPLC_E_NVM);
	}
	else if(fdplc_status.ecode & (1 << FDPLC_E_CAL)) {
		led_error(FDPLC_E_CAL);
	}
	else if(fdplc_status.ecode & (1 << FDPLC_E_WDT)) {
		led_y(LED_EXT_Y);
	}
	else {
		switch(fdplc_status.test_status) {
		case FDPLC_STATUS_PASS:
			led_y(LED_EXT_G);
			break;
		case FDPLC_STATUS_FAIL:
			led_y(LED_EXT_R);
			break;
		case FDPLC_STATUS_TEST:
			led_f((fdplc_status.test_offline) ? LED_EXT_G : LED_EXT_Y);
			break;
		}
	}
}

void fdplc_update(void)
{
	//test unit died detection
	if(fdplc_status.wdt_y) {
		if(time_left(fdplc_wdt) < 0) {
			fdplc_on_event(FDPLC_EVENT_WDT_TIMEOUT);
		}
	}

	//for mcp detection, PortB@MCP1 of iox board is unused
	unsigned char magic = 0;
	mcp23s17_ReadByte(&fdplc_mcp, ADDR_OLATB, &magic);
	if((magic & fdiox_mask) != fdiox_magic) {
		fdplc_on_event(FDPLC_EVENT_FAIL_MCP);
	}

	//report status to local front panel display
	fdplc_status_update();

	//wimg for valves
	if(fdplc_w_msk) {
		int msk, img;
		__disable_irq();
		msk = fdplc_w_msk;
		img = fdplc_w_img;
		fdplc_w_msk = 0;
		fdplc_w_img = 0;
		__enable_irq();
		gpio_wimg(img, msk);
	}

	//rimg for valves/sensors
	int sensors = fdplc_rimg();

	//report status to remote fdhost
	int delta = sensors ^ fdplc_sensors;
	if(delta) {
		fdplc_sensors = sensors;
		if(delta) {
			if(fdplc_status.assigned) {
				//report to vwhost
				__disable_irq();
				fdplc_txdat->src = fdplc_id_assign;
				fdplc_txdat->cmd = FDCMD_POLL;
				fdplc_txdat->model = fdplc_eeprom.model;
				fdplc_txdat->pushed = fdplc_eeprom.pushed;
				fdplc_txdat->sensors = fdplc_sensors;
				fdplc_can_send(&fdplc_txmsg);
				__enable_irq();
			}
		}
	}
}

//implemented by fdpwr.c
extern void fdpwr_init(void);
extern void fdpwr_update(void);

void main()
{
	sys_init();
	shell_mute(NULL);
	fdplc_init();
	fdpwr_init();
	printf("fdplc v1.x, build: %s %s\n\r", __DATE__, __TIME__);
	printf("press key 'alt'+'m' to begin ... ");
	while(1){
		sys_update();
		fdplc_update();
		fdpwr_update();
	}
}

void USB_LP_CAN1_RX0_IRQHandler(void)
{
	CAN_ClearITPendingBit(CAN1, CAN_IT_FMP0);
	while(!fdplc_can->recv(&fdplc_rxmsg)) {
		fdplc_can_handler();
	}
}

static int cmd_fdplc_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"FDPLC ID?			return test unit id\n"
		"FDPLC READY?		return -1 or mysql test record id\n"
		"FDPLC OFFLINE		change fdplc to offline mode\n"
		"FDPLC ONLINE		change fdplc to online mode\n"
		"FDPLC PASS\n"
		"FDPLC FAIL\n"
		"FDPLC RIMG?\n"
		"FDPLC WIMG MSK HEX\n"
	};

	if(argc < 2) {
		printf("%s", usage);
		return 0;
	}

	if(!strcmp(argv[1], "ID?")) {
		if(argc > 2) {
			sscanf(argv[2], "%d", &fdplc_id_assign);
			fdplc_txmsg.id = CAN_ID_UNIT(fdplc_id_assign);
			fdplc_txmsg.dlc = 8;
		}
		printf("<%+d\n", fdplc_id_assign);
	}
	else if(!strcmp(argv[1], "READY?")) {
		int sql_id = (fdplc_status.test_end) ? -1 : fdplc_id_sql;
		printf("<%+d\n", sql_id);
	}
	else if(!strcmp(argv[1], "OFFLINE")) {
		fdplc_on_event(FDPLC_EVENT_OFFLINE);
		printf("<%+d, OK\n", 0);
	}
	else if(!strcmp(argv[1], "ONLINE")) {
		fdplc_on_event(FDPLC_EVENT_ONLINE);
		printf("<%+d, OK\n", 0);
	}
	else if(!strcmp(argv[1], "PASS")) {
		fdplc_on_event(FDPLC_EVENT_PASS);
		printf("<%+d, OK\n", 0);
	}
	else if(!strcmp(argv[1], "FAIL")) {
		fdplc_on_event(FDPLC_EVENT_FAIL);
		printf("<%+d, OK\n", 0);
	}
	else if(!strcmp(argv[1], "RIMG?")) {
		printf("<%+d, OK\n", fdplc_sensors);
	}
	else if(!strcmp(argv[1], "WIMG")) {
		int msk = 0;
		int img = 0;
		if(argc > 3) {
			int n = 0;
			if((argv[2][1] == 'x') || (argv[2][1] == 'X')) n += sscanf(argv[2], "%x", &msk);
			else n += sscanf(argv[2], "%d", &msk);
			if((argv[3][1] == 'x') || (argv[3][1] == 'X')) n += sscanf(argv[3], "%x", &img);
			else n += sscanf(argv[3], "%d", &img);
			if(n == 2) {
				fdplc_wimg(img, msk);
				printf("<%+d, OK\n", 0);
			}
		}
	}

	return 0;
}

cmd_t cmd_fdplc = {"FDPLC", cmd_fdplc_func, "fdplc i/f commands"};
DECLARE_SHELL_CMD(cmd_fdplc)

static int cmd_fixture_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"FIXTURE LIST		list all supported fixtures\n"
		"FIXTURE TYPE?		query current fixture type id\n"
		"FIXTURE C P		configure fixture model with customer and product name\n"
		"FIXTURE PUSHED?		query fixture pushed counter\n"
		"FIXTURE PUSHED RESET	reset fixture pushed counter\n"
	};

	if(argc < 2) {
		printf("%s", usage);
		return 0;
	}

	if(!strcmp(argv[1], "LIST")) {
		printf("<%+02d, fixture_list_size = %d\n", fdmdl_list_size, fdmdl_list_size);
		for(int i = 0; i < fdmdl_list_size; i ++) {
			printf("<%+02d, %s \"%s\"\n", i, fdmdl_list[i].customer, fdmdl_list[i].product);
		}
		return 0;
	}
	else if(!strcmp(argv[1], "TYPE?")) {
		int model = (fdplc_eeprom.magic == FDPLC_NVM_MAGIC) ? fdplc_eeprom.model : -1;
		const char *customer = "invalid";
		const char *product = "invalid";
		if(fdplc_model_is_valid(model)) {
			customer = fdmdl_list[model].customer;
			product = fdmdl_list[model].product;
		}

		printf("<%+d, %s %s\n", model, customer, product);
		return 0;
	}
	else if(!strcmp(argv[1], "PUSHED?")) {
		int pushed = (fdplc_eeprom.magic == FDPLC_NVM_MAGIC) ? fdplc_eeprom.pushed : -1;
		printf("<%+d, pushed counter\n", pushed);
		return 0;
	}
	else if(!strcmp(argv[1], "PUSHED")) {
		if((argc == 3) && (!strcmp(argv[2], "RESET"))) {
			fdplc_eeprom.pushed = 0;
			int ecode = fdplc_eeprom_save(sizeof(fdplc_eeprom.pushed));
			printf("<%+d, pushed counter reset\n", ecode);
			return 0;
		}
	}
	else if(argc == 3) {
		int model = -1;
		for(int i = 0; i < fdmdl_list_size; i ++) {
			int mismatch = strcmp(argv[1], fdmdl_list[i].customer);
			if(!mismatch) mismatch = strcmp(argv[2], fdmdl_list[i].product);
			if(!mismatch) {
				model = i;
				break;
			}
		}

		if(model >= 0) {
			fdplc_eeprom.model = model;
			fdplc_eeprom.pushed = 0;
			fdplc_eeprom.magic = FDPLC_NVM_MAGIC;
			int ecode = fdplc_eeprom_save(sizeof(fdplc_eeprom));
			if(ecode) {
				printf("<%+d, model configuration save to eeprom fail\n", ecode);
				return 0;
			}
			else fdplc_on_event(FDPLC_EVENT_GOOD_NVM);
		}

		printf("<%+d, model configuration\n", model);
		return 0;
	}

	printf("<%+d, unsupported cmd or cmd para invalid\n", -1);
	return 0;
}

cmd_t cmd_fixture = {"FIXTURE", cmd_fixture_func, "fixture i/f commands"};
DECLARE_SHELL_CMD(cmd_fixture)

int cmd_xxx_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"*IDN?		to read identification string\n"
		"*RST		instrument reset\n"
		"*UUID?		query the stm32 uuid\n"
		"*WDT?		update wdt and return system ecode\n"
	};

	if(!strcmp(argv[0], "*IDN?")) {
		printf("<0,Ulicar Technology, FDPLC V1.x,%s,%s\n\r", __DATE__, __TIME__);
		return 0;
	}
	else if(!strcmp(argv[0], "*RST")) {
		printf("<+0, No Error\n\r");
		mdelay(50);
		NVIC_SystemReset();
	}
	else if(!strcmp(argv[0], "*UUID?")) {
		unsigned uuid = *(unsigned *)(0X1FFFF7E8);
		printf("<%+d\n", uuid);
		return 0;
	}
	else if(!strcmp(argv[0], "*WDT?")) {
		int ms = fdplc_status.wdt_y ? time_left(fdplc_wdt) : 0;
		fdplc_wdt = time_get(FDPLC_WDT_MS);
		fdplc_on_event(FDPLC_EVENT_WDT_OK);
		printf("<%+d, time left %d mS\n", fdplc_status.ecode, ms);
		return 0;
	}
	else if(!strcmp(argv[0], "*?")) {
		printf("%s", usage);
		return 0;
	}
	else {
		printf("<-1, Unknown Command\n\r");
		return 0;
	}
	return 0;
}
