/*
*
*  miaofng@2014-5-10   initial version
*  miaofng@2014-6-7	slot board v2.2, line switch default connect to external
*
*/
#include "ulp/sys.h"
#include "../irc.h"
#include "../vm.h"
#include "led.h"
#include <string.h>
#include "shell/shell.h"
#include "can.h"
#include "../err.h"
#include "mbi5025.h"
#include "common/bitops.h"
#include "../mxc.h"

static const spi_bus_t *mxc_spi = &spi1;
static const can_bus_t *mxc_can = &can1;
const mbi5025_t mxc_mbi = {.bus = &spi1, .idx = 0};
static char mxc_image[20];
static char mxc_image_static[20]; //hold static open/closed relays
static int mxc_addr = 0;
static int mxc_line_min;
static int mxc_line_max;
static opcode_t mxc_opcode_alone; //added to solve seq opcode be located two can frame issue
static volatile int mxc_le_pulsed;
static int mxc_ecode;
static int mxc_le_timeout;
static time_t mxc_timer_poll;
static enum mxc_status_e {
	MXC_STATUS_INIT,
	MXC_STATUS_READY,
	MXC_STATUS_ERROR,
	MXC_STATUS_PING,
} mxc_status;

static void mxc_can_handler(can_msg_t *msg);

#if 1
#include "stm32f10x.h"

static void _mxc_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

	/*PA0(= BUS0)..PA3*/
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/*ADDR PINS PC8(=A0)..PC15*/
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | \
		GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	/*mbi OE PC4 */
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	//hardware bug, set mbi LE PA4 to input to avoid conflict
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	/*irc central control, init LE_txd, LE_rxd,
	LE_txd	PC7
	LE_rxd	PC6
	*/
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	//LE_rxd	PC6
	EXTI_InitTypeDef EXTI_InitStruct;
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource6);
	EXTI_InitStruct.EXTI_Line = EXTI_Line6;
	EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Rising;
	EXTI_InitStruct.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStruct);

	/*!!! to usePreemptionPriority, group must be config first
	systick priority !must! use  NVIC_SetPriority() to set
	*/
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	NVIC_SetPriority(SysTick_IRQn, 0);

	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

void _can_isr_enable(void)
{
	NVIC_InitTypeDef  NVIC_InitStructure;
#ifndef STM32F10X_CL
	NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
#else
	NVIC_InitStructure.NVIC_IRQChannel = CAN1_RX0_IRQn;
#endif
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	CAN_ClearITPendingBit(CAN1, CAN_IT_FMP0);
	CAN_ITConfig(CAN1, CAN_IT_FMP0, ENABLE);
}

void USB_LP_CAN1_RX0_IRQHandler(void)
{
	CAN_ClearITPendingBit(CAN1, CAN_IT_FMP0);
	can_msg_t msg;
	while(!mxc_can->recv(&msg)) {
		mxc_can_handler(&msg);
	}
}

/*mbi OE PC4 */
static inline void _oe_set(int high) {
	if(high) {
		GPIOC->BSRR = GPIO_Pin_4;
	}
	else {
		GPIOC->BRR = GPIO_Pin_4;
	}
}

/*LE_txd PC7*/
static inline void _le_set(int high) {
	mxc_le_pulsed = 0;
	if(high) {
		GPIOC->BSRR = GPIO_Pin_7;
	}
	else {
		GPIOC->BRR = GPIO_Pin_7;
	}
}

/*LE_rxd	PC6*/
void EXTI9_5_IRQHandler(void)
{
	mxc_le_pulsed = 1;
	EXTI->PR = EXTI_Line6;
}

static inline int _le_get(void) {
	return (GPIOC->IDR & GPIO_Pin_6) ? 1 : 0;
}

void mxc_bus_set(unsigned bus_mask)
{
	int v = GPIOA->ODR ; /*PA0(= BUS0)..PA3*/
	v &= 0xfff0;
	v |= bus_mask;
	GPIOA->ODR = v;
}

int _mxc_addr_get(void)
{
	int v = GPIOC->IDR;
	v >>= 8;
	v = ~v;
	v &= 0xff;
	return v;
}
#endif

static int mxc_status_change(enum mxc_status_e new_status)
{
	/*default set le = 0 to disable any relay matrix latch operation
	response to can_id_dat/cmd only when status = ready
	*/
	switch(new_status) {
	case MXC_STATUS_INIT:
		led_flash(LED_YELLOW);
		_le_set(0);
		mxc_status = new_status;
		break;
	case MXC_STATUS_READY:
		mxc_timer_poll = time_get(IRC_POL_MS * 2);
		led_off(LED_YELLOW);
		led_flash(LED_GREEN);
		mxc_status = new_status;
		break;
	case MXC_STATUS_ERROR:
		led_off(LED_YELLOW);
		led_error(mxc_ecode);
		_le_set(0);
		mxc_status = new_status;
		break;
	case MXC_STATUS_PING:
		if(mxc_status == MXC_STATUS_READY) {
			if(mxc_timer_poll == 0) {
				//restore led display
				led_off(LED_YELLOW);
				led_flash(LED_GREEN);
			}
			mxc_timer_poll = time_get(IRC_POL_MS * 2);
		}
		break;
	default:
		mxc_status = MXC_STATUS_ERROR;
		break;
	}
	return 0;
}

static void mxc_relay_clr_all(void)
{
	//32lines * 4buses / 8 = 16Bytes
	memset(mxc_image, 0x00, 16);
}

/* opcode only support: VM_OPCODE_OPEN, VM_OPCODE_CLOSE
*/
static int mxc_relay_set(int line, int bus, int opcode)
{
	int n = (line << 2) + bus;
	if((opcode == VM_OPCODE_CLOSE) || (opcode == VM_OPCODE_SCAN)) {
		bit_set(n, mxc_image);
	}
	else if( opcode == VM_OPCODE_OPEN )  {
		bit_clr(n, mxc_image);
	}
	else {
		return -1;
	}
	return 0;
}

static int mxc_line_set(unsigned line_mask)
{
	//line0..line31(32*4bit/8)
	memcpy(mxc_image + 16, &line_mask, sizeof(line_mask));
	return 0;
}

int mxc_execute(int save)
{
	//shift out .. refer to mbi5025_write_and_latch
	for(int i = sizeof(mxc_image) - 1; i >= 0; i --) {
		char byte = mxc_image[i];
		mxc_spi->wreg(0, byte);
	}

	//led_on(LED_RED);
	//load pin ctrl, to sync every slots
	time_t deadline = time_get(mxc_le_timeout);
	time_t suspend = time_get(IRC_UPD_MS);

	/*in case of mxc error, i'll hold LE bus low
	to tell irc, pls take care of me :(
	pc could use mode command to unlock
	*/
	_le_set(1);
	while(!mxc_le_pulsed) {
		if(time_left(suspend) < 0) {
			sys_update();
		}
		if(mxc_le_timeout != 0) {
			if(time_left(deadline) < 0) { //host error???
				mxc_ecode = -IRT_E_SLOT;
				break;
			}
		}
	}

	//wait for irc clear le
	while(_le_get() == 1) {
		if(time_left(suspend) < 0) {
			sys_update();
		}
		if(mxc_le_timeout != 0) {
			if(time_left(deadline) < 0) { //host error???
				mxc_ecode = -IRT_E_SLOT;
				break;
			}
		}
	}

	//operation finish
	_le_set(0);
	//led_off(LED_RED);
	if(mxc_ecode) {
		mxc_status_change(MXC_STATUS_ERROR);
	}

	if(save) {
		memcpy(mxc_image_static, mxc_image, sizeof(mxc_image));
	}
	else { //this is maybe an scan operation, ignore the all modifications before
		memcpy(mxc_image, mxc_image_static, sizeof(mxc_image));
	}
	return 0;
}

/* limitations:
1, seq operation must be inside one can frame ----- supported now!
2, command frame must contain at least one relay operation
*/
static void mxc_can_switch(can_msg_t *msg)
{
	opcode_t *opcode_seq, *opcode;
	opcode_t opcode_alone;

	int n = msg->dlc / sizeof(opcode_t);
	for(int i = 0; i < n; ) {
		//lonely opcode exist?
		if(mxc_opcode_alone.special.type != VM_OPCODE_NUL) {
			//use it ...
			opcode_alone.value = mxc_opcode_alone.value;
			opcode = &opcode_alone;
			mxc_opcode_alone.special.type = VM_OPCODE_NUL;
		}
		else {
			opcode = (opcode_t *) msg->data + i;
			i ++;
		}

		opcode_seq = opcode;
		if(opcode->special.type == VM_OPCODE_SEQ) {
			if(i < n) {
				opcode = (opcode_t *) msg->data + i;
				i ++;
			}
			else { //save to alone opcode, this can id mustn't be a command frame
				mxc_opcode_alone.value = opcode->value;
				break;
			}
		}

		//seq contains special type & line_start
		int min = opcode_seq->line;
		int max = opcode->line;
		min = (min < mxc_line_min) ? mxc_line_min : min;
		max = (max > mxc_line_max) ? mxc_line_max : max;
		for(int line = min - mxc_line_min; line <= max - mxc_line_min; line ++) {
			switch(opcode->type) {
			case VM_OPCODE_SCAN:
			case VM_OPCODE_OPEN:
			case VM_OPCODE_CLOSE:
				mxc_relay_set(line, opcode->bus, opcode->type);
				break;
			default:
				sys_error("invalid opcode");
				break;
			}
		}
	}

	if(msg->id == CAN_ID_CMD) { //command frame, execute it ...
		mxc_execute(opcode->type != VM_OPCODE_SCAN);
	}
}

static void mxc_can_cfg(can_msg_t *msg)
{
	mxc_cfg_msg_t *cfg = (mxc_cfg_msg_t *) msg->data;
	mxc_echo_msg_t *echo = (mxc_echo_msg_t *) msg->data;
	int slot = cfg->slot, bus, line;

	switch(cfg->cmd) {
	case MXC_CMD_RST:
		mxc_addr = _mxc_addr_get();
		slot = (cfg->slot == 0xff) ? mxc_addr : slot;
		if(slot == mxc_addr) {
			mxc_ecode = IRT_E_OK; //clear last error
			mxc_status_change(MXC_STATUS_READY);
			mxc_can->flush();
			mxc_opcode_alone.special.type = VM_OPCODE_NUL;
			mxc_line_min = mxc_addr * 32;
			mxc_line_max = mxc_line_min + 31;
		}
		break;
	case MXC_CMD_CFG:
		slot = (cfg->slot == 0xff) ? mxc_addr : slot;
		bus = cfg->bus;
		line = ~cfg->line;
		if(slot == mxc_addr) {
			mxc_le_timeout = cfg->ms;
			mxc_relay_clr_all();
			mxc_line_set(line);
			mxc_execute(1);
			mxc_bus_set(bus);
			_oe_set(0);
		}
		break;
	case MXC_CMD_PING:
		if(slot == mxc_addr) {
			echo->cmd = MXC_CMD_ECHO;
			echo->slot = (unsigned char) mxc_addr;
			echo->ecode = mxc_ecode;
			mxc_can->send(msg);
		}
		mxc_status_change(MXC_STATUS_PING);
		break;
	default:
		break;
	}


}

int mxc_verify_sequence(int cnt)
{
	return 0;
}

void mxc_can_handler(can_msg_t *msg)
{
	int id = msg->id;
	int cnt = id & 0x0f;
	id &= 0xfff0;

	switch(id) {
	case CAN_ID_DAT:
	case CAN_ID_CMD:
		if(mxc_status == MXC_STATUS_READY) {
			if(!mxc_verify_sequence(cnt)) {
				msg->id = id;
				mxc_can_switch(msg);
			}
		}
		break;
	case CAN_ID_CFG:
		mxc_can_cfg(msg);
		break;
	default:
		break;
	}
}

int mxc_init(void)
{
	_mxc_init();
	mbi5025_Init(&mxc_mbi);
	_oe_set(1);
	_le_set(0);

	//state matchine
	mxc_status_change(MXC_STATUS_INIT);

	//communication init
	const can_cfg_t cfg = {.baud = CAN_BAUD, .silent = 0};
	mxc_can->init(&cfg);
	_can_isr_enable();
        return 0;
}

void main()
{
	sys_init();
	mxc_init();
	printf("mxc v1.1, SW: %s %s\n\r", __DATE__, __TIME__);
	while(1){
		sys_update();
		if(mxc_status == MXC_STATUS_READY) {
			if(mxc_timer_poll != 0) {
				if(time_left(mxc_timer_poll) < 0) {
					led_flash(LED_YELLOW);
					mxc_timer_poll = 0;
				}
			}
		}
	}
}

static int cmd_mxc_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"mxc [set|get] <line> <bus>	turn on line:bus\n"
		"mxc line <line>		turn on line sw\n"
		"mxc bus <bus>			turn on bus sw\n"
	};

	if(!strcmp(argv[1], "set")) {
		int line = (argc > 2) ? atoi(argv[2]) : -1;
		int bus = (argc > 3) ? atoi(argv[3]) : -1;
		if(line < 0) {
			//turn on all relays
		}
		else if(line < 32) {
			if(bus < 0) {
				//turn on all line relays
			}
			else if(bus < 4) {
				//turn on bus:line
				memset(mxc_image, 0x00, sizeof(mxc_image));
				mxc_relay_set(line, bus, VM_OPCODE_CLOSE);
				mxc_execute(1);
				return 0;
			}
		}
	}
	else if(!strcmp(argv[1], "clr")) {
		int line = (argc > 2) ? atoi(argv[2]) : -1;
		int bus = (argc > 3) ? atoi(argv[3]) : -1;
		if(line < 0) {
			//turn off all relays
			mxc_execute(1);
			return 0;
		}
		else if(line < 32) {
			if(bus < 0) {
				//turn on all line relays
			}
			else if(bus < 4) {
				//turn on bus:line
				mxc_relay_set(line, bus, VM_OPCODE_OPEN);
				mxc_execute(1);
				return 0;
			}
		}
	}
	else if(!strcmp(argv[1], "line")) {
		int line = (argc > 2) ? atoi(argv[2]) : 0;
		mxc_line_set(1 << line);
		mxc_execute(1);
		return 0;
	}
	else if(!strcmp(argv[1], "bus")) {
		int bus = (argc > 2) ? atoi(argv[2]) : 0;
		mxc_bus_set(1 << bus);
		return 0;
	}
	else if(!strcmp(argv[1], "scan")) {
		int on = 1;
		int ms = atoi(argv[2]);
		while(1) {
			for(int bus = 0; bus < 4; bus ++) {
				for(int line = 0; line < 32; line ++) {
					mxc_relay_set(line, bus, (on) ? VM_OPCODE_CLOSE : VM_OPCODE_OPEN);
					mxc_execute(1);
					sys_mdelay(ms);
				}
			}
			on = !on;
		}
	}

	printf("%s", usage);
	return 0;
}
const cmd_t cmd_mxc = {"mxc", cmd_mxc_func, "relay debug commands"};
DECLARE_SHELL_CMD(cmd_mxc)
