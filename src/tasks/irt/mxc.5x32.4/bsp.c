/*
*
*  miaofng@2014-9-23   separate driver from mxc main routine
*
*/
#include "ulp/sys.h"
#include "led.h"
#include <string.h>
#include "shell/shell.h"
#include "can.h"
#include "mbi5025.h"
#include "common/bitops.h"
#include "stm32f10x.h"
#include "mxc.h"

static const can_bus_t *mxc_can = &can1;
static const spi_bus_t *mxc_spi = &spi1;
static const mbi5025_t mxc_mbi = {.bus = &spi1, .idx = 0};
static volatile int mxc_le_pulsed;
static int mxc_le_locked;
static int mxc_flag_diag; //set by gpio_init
static can_msg_t mxc_msg;

//relay image memory
static char mxc_image_write_static;
static char mxc_image[20];
static char mxc_image_static[20]; //hold static open/closed relays
static char mxc_sense; //0..3=>vbus0..3 ctrl, msb=>vline ctrl

static void mxc_gpio_init(void)
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

	/*ADDR PINS PC8(=A0)..PC15, PC3 = DET*/
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | \
		GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

#if 1
	/*PC2(LINE_CTRL) MMUN2211 10K+10K*/
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	/*high level means no MMUN2211 => slot v2.2, has self diag function*/
	mxc_flag_diag = (GPIOC->IDR & GPIO_Pin_2) ? 1 : 0;
#endif

	/*mbi OE PC4, PC2 = VLINE */
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_2;
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
}

/*mbi OE PC4 */
void oe_set(int high)
{
	if(high) {
		GPIOC->BSRR = GPIO_Pin_4;
	}
	else {
		GPIOC->BRR = GPIO_Pin_4;
	}
}

/*LE_txd PC7*/
void le_set(int high)
{
	if(mxc_le_locked) {
		high = 0;
	}

	mxc_le_pulsed = 0;
	if(high) {
		GPIOC->BSRR = GPIO_Pin_7;
	}
	else {
		GPIOC->BRR = GPIO_Pin_7;
	}
}

int le_get(void)
{
	return (GPIOC->IDR & GPIO_Pin_6) ? 1 : 0;
}

/*LE_rxd	PC6*/
void EXTI9_5_IRQHandler(void)
{
	mxc_le_pulsed = 1;
	EXTI->PR = EXTI_Line6;
}

int le_is_pulsed(void)
{
	return mxc_le_pulsed;
}

void le_lock(void)
{
	mxc_le_locked = 1;
	le_set(0);
}

void le_unlock(void)
{
	mxc_le_locked = 0;
	le_set(0);
}

void sense_set(unsigned char mask)
{
	//vbus 3..0
	int v = GPIOA->ODR ; /*PA0(= BUS0)..PA3*/
	v &= 0xfff0;
	v |= (mask & 0x0f);
	GPIOA->ODR = v;

	//vline
	if(mask & 0x80) {
		GPIOC->BSRR = GPIO_Pin_2;
	}
	else {
		GPIOC->BRR = GPIO_Pin_2;
	}
}

int mxc_addr_get(void)
{
	int v = GPIOC->IDR;
	v >>= 8;
	v = ~v;
	v &= 0xff;
	return v;
}

int mxc_has_diag(void)
{
	return mxc_flag_diag;
}

int mxc_has_dcfm(void)
{
	/*PC3(DET), if DCFM(din cal fixture mounted), level should be 0*/
	int mounted = (GPIOC->IDR & GPIO_Pin_3) ? 0 : 1;
	return mounted;
}

void mxc_relay_clr_all(void)
{
	//32lines * 4buses / 8 = 16Bytes
	memset(mxc_image, 0x00, 16);
}

void mxc_relay_set(int line, int bus, int on)
{
	int n = (line << 2) + bus;
	if(on) {
		bit_set(n, mxc_image);
	}
	else {
		bit_clr(n, mxc_image);
	}
}

void mxc_vsense_set(int mask)
{
	mxc_sense |= mask;
}

void mxc_linesw_set(unsigned line_mask)
{
	//line0..line31(32*4bit/8)
	memcpy(mxc_image + 16, &line_mask, sizeof(line_mask));
}

void mxc_image_store(void)
{
	memcpy(mxc_image_static, mxc_image, sizeof(mxc_image));
}

void mxc_image_restore(void)
{
	memcpy(mxc_image, mxc_image_static, sizeof(mxc_image));
}

void mxc_image_select_static(void)
{
	mxc_image_write_static = 1;
}

void mxc_image_write(void)
{
	char *image = mxc_image;
	if(mxc_image_write_static) {
		mxc_image_write_static = 0;

		sense_set(0); //sense relay ctrl
		image = mxc_image_static;
	}
	else {
		sense_set(mxc_sense); //sense relay ctrl
		mxc_sense = 0;
	}

	//shift out .. refer to mbi5025_write_and_latch
	for(int i = sizeof(mxc_image) - 1; i >= 0; i --) {
		char byte = image[i];
		mxc_spi->wreg(0, byte);
	}
}

void board_init(void)
{
	mxc_gpio_init();
	const can_cfg_t cfg = {.baud = CAN_BAUD, .silent = 0};
	mxc_can->init(&cfg);
	mbi5025_Init(&mxc_mbi);
	mxc_image_write_static = 0;

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

	NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	CAN_ClearITPendingBit(CAN1, CAN_IT_FMP0);
	CAN_ITConfig(CAN1, CAN_IT_FMP0, ENABLE);
}

void board_reset(void)
{
	NVIC_SystemReset();
}

void USB_LP_CAN1_RX0_IRQHandler(void)
{
	CAN_ClearITPendingBit(CAN1, CAN_IT_FMP0);
	while(!mxc_can->recv(&mxc_msg)) {
		mxc_can_handler(&mxc_msg);
	}
}

void mxc_can_echo(int slot, int flag, int ecode)
{
	can_msg_t msg;
	mxc_echo_t *echo = (mxc_echo_t *) msg.data;
	echo->ecode = ecode;
	echo->flag = flag;

	msg.dlc = sizeof(mxc_echo_t);
	msg.id = CAN_ID_MXC | MXC_NODE(slot);

	time_t deadline = time_get(IRC_CAN_MS);
	mxc_can->flush_tx();
	if(!mxc_can->send(&msg)) { //wait until message are sent
		while(time_left(deadline) > 0) {
			if(mxc_can->poll(CAN_POLL_TBUF) == 0) {
				break;
			}
		}
	}
}

static int cmd_mxc_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"mxc [set|clr] <line> <bus>	turn on/off line:bus\n"
		"mxc line <line>		turn on line sw\n"
		"mxc bus <bus>			turn on bus sw\n"
		"mxc scan <ms>			scan all relay\n"
	};

	if(argc == 4) {
		int on = (argv[1][0] == 's') ? 1 : 0;
		int line = atoi(argv[2]);
		int bus = atoi(argv[3]);

		line = (line > 31) ? 0 : line;
		bus = (bus > 3) ? 0 : bus;

		memset(mxc_image, 0x00, sizeof(mxc_image));
		mxc_relay_set(line, bus, on);
		mxc_execute();
		return 0;
	}
	else if(argc == 3) {
		if(!strcmp(argv[1], "line")) {
			int line = atoi(argv[2]);
			mxc_linesw_set(1 << line);
			mxc_execute();
			return 0;
		}
		else if(!strcmp(argv[1], "bus")) {
			int bus = atoi(argv[2]);
			sense_set(1 << bus);
			mxc_execute();
			return 0;
		}
		else if(!strcmp(argv[1], "scan")) {
			int on = 1;
			int ms = atoi(argv[2]);
			while(1) {
				for(int bus = 0; bus < 4; bus ++) {
					for(int line = 0; line < 32; line ++) {
						mxc_relay_set(line, bus, on);
						mxc_execute();
						sys_mdelay(ms);
					}
				}
				on = !on;
			}
		}
	}

	printf("%s", usage);
	return 0;
}
const cmd_t cmd_mxc = {"mxc", cmd_mxc_func, "relay debug commands"};
DECLARE_SHELL_CMD(cmd_mxc)
