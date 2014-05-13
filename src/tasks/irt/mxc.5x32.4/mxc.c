/*
*
*  miaofng@2014-5-10   initial version
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

static const spi_bus_t *mxc_spi = &spi1;
static const can_bus_t *mxc_can = &can1;
const mbi5025_t mxc_mbi = {.bus = &spi1, .idx = 0, .load_pin = SPI_CS_PA4, .oe_pin = SPI_CS_PC4};
static char mxc_image[20];
static char mxc_image_static[20]; //hold static open/closed relays
static int mxc_addr = 0;

static void mxc_can_handler(can_msg_t *msg);

#if 1
#include "stm32f10x.h"

static void _mxc_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

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

void _can_isr_enable(void)
{
	NVIC_InitTypeDef  NVIC_InitStructure;
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
#ifndef STM32F10X_CL
	NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
#else
	NVIC_InitStructure.NVIC_IRQChannel = CAN1_RX0_IRQn;
#endif
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0;
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
#endif

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
	mbi5025_write_and_latch(&mxc_mbi, mxc_image, sizeof(mxc_image));
	if(save) {
		memcpy(mxc_image_static, mxc_image, sizeof(mxc_image));
	}
	else { //this is maybe an scan operation, ignore the all modifications before
		memcpy(mxc_image, mxc_image_static, sizeof(mxc_image));
	}
	return 0;
}

void mxc_init(void)
{
	_mxc_init();
	mxc_addr = _mxc_addr_get();
	mbi5025_Init(&mxc_mbi);
	memset(mxc_image, 0x00, sizeof(mxc_image));
	mxc_execute(1);

	//communication init
	const can_cfg_t cfg = {.baud = CAN_BAUD, .silent = 0};
	mxc_can->init(&cfg);
	_can_isr_enable();
}

/* limitations:
1, seq operation must be inside one can frame
2, command frame must contain at least one relay operation
*/
static void mxc_can_switch(can_msg_t *msg)
{
	opcode_t *opcode_seq, *opcode = (opcode_t *) msg->data;
	for(int i = 0; i < msg->dlc; opcode ++, i += sizeof(opcode_t)) {
		opcode_seq = opcode;
		if(opcode->special.type == VM_OPCODE_SEQ) {
			opcode ++;
		}

		//seq contains special type & line_start
		for(int line = opcode_seq->line; line <= opcode->line; line ++) {
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
	irc_cfg_msg_t *cfg = (irc_cfg_msg_t *) msg->data;
	int slot = mxc_addr, bus, line, off = 1;

	switch(cfg->cmd) {
	case IRC_CFG_NORMAL:
		off = 0;
		slot = (cfg->slot == 0xff) ? mxc_addr : slot;
		bus = cfg->bus;
		line = cfg->line;
		break;
	case IRC_CFG_ALLOFF_IBUS_ELNE:
		bus = 0; //hw default = ibus
		line = -1; //hw default = iline
		break;
	case IRC_CFG_ALLOFF_EBUS_ILNE:
		bus = 0xff;
		line = 0;
		break;
	case IRC_CFG_ALLOFF_EBUS_ELNE:
		bus = -1;
		line = -1;
		break;
	default:
		break;
	}

	if(slot == mxc_addr) {
		if(off) {
			mxc_relay_clr_all();
		}
		mxc_line_set(line);
		mxc_execute(1);
		mxc_bus_set(bus);
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
		if(!mxc_verify_sequence(cnt)) {
			msg->id = id;
			mxc_can_switch(msg);
		}
		break;
	case CAN_ID_CFG:
		mxc_can_cfg(msg);
		break;
	default:
		break;
	}
}

void main()
{
	sys_init();
	mxc_init();
	printf("mxc v1.0, SW: %s %s\n\r", __DATE__, __TIME__);
	while(1){
		sys_update();
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
