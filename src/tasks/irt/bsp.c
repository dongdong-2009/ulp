/*
*
*  miaofng@2014-6-6   separate driver from irc main routine
*  miaofng@2014-9-13 remove irt hw v1.x related codes
*
*/
#include "ulp/sys.h"
#include "irc.h"
#include "err.h"
#include "stm32f10x.h"
#include "can.h"
#include "bsp.h"
#include "mbi5025.h"
#include "shell/cmd.h"

static volatile int irc_vmcomp_pulsed;
static const mbi5025_t irc_mbi = {.bus = &spi2, .load_pin = SPI_2_NSS, .oe_pin = SPI_CS_PB12};

/*to conver relay name order(LSn) to mbi5024 control line nr(Km)*/
static int rly_map(int relays)
{
	int image = 0;
	const char map[] = {0,11,12,21,1,10,13,20,  2,9,14,19,5,6,24,3,  18,25,8,15,7,4,26,17,  16,23,22,27,};
	for(int i = 0; i < sizeof(map); i ++) {
		if(relays & (1 << i)) {
			image |= 1 << map[i];
		}
	}
	return image;
}

static void rly_init(void)
{
	mbi5025_DisableOE(&irc_mbi);
	mbi5025_Init(&irc_mbi);
	mbi5025_EnableOE(&irc_mbi);
}

/* warnning:
1, RPB: all slot's vline/iline must be shorten and PROBE line resistance will not be ignored!!
2, RMX: the same as rpb, but slot's vline/iline is shorten only when required
3, L4R = W4R, but IS is different
4, L2T could be used for diode test
5, maybe we should delete K4,K5,K6,K7,K20?
*/
void rly_set(int mode)
{
	#define LS(n) (1 << n)

	const int image[] = {
		LS(12)|LS(14), /*HVR*/
		LS(18)|LS(15)|LS(23)|LS(21), /*L4R, real 4 line resistor measurement*/
		LS(17)|LS(15)|LS(24)|LS(21), /*W4R, use Is!*/
		LS(18)|LS(15), /*L2T, dmm sense pin is open*/
		LS(18)|LS(23)|LS(0)|LS(1)|LS(2)|LS(3)|LS(8)|LS(9)|LS(10)|LS(11), /*RPB*/
		LS(18)|LS(23)|LS(0)|LS(1)|LS(2)|LS(3)|LS(8)|LS(9)|LS(10)|LS(11), /*RMX*/
		LS(25)|LS(16)|LS(18), /*VHV*/
		LS(26)|LS(16)|LS(18), /*VLV*/
		LS(17)|LS(16)|LS(13), /*IIS*/
		0x00,
	};

	sys_assert((mode >= IRC_MODE_HVR) && (mode <= IRC_MODE_OFF));
	int relays = image[mode];
	relays = rly_map(relays);
	mbi5025_write_and_latch(&irc_mbi, &relays, sizeof(relays));
}

static int cmd_relay_func(int argc, char *argv[])
{
	const char * usage = { \
		"usage:\n" \
		"relay 0-2,4	turn on relay k0,k1,k2,k4\n" \
	};

	if (argc >= 2) {
		int relays = cmd_pattern_get(argv[1]);
		relays = rly_map(relays);
		mbi5025_write_and_latch(&irc_mbi, &relays, sizeof(relays));
		return 0;
	}

	printf(usage);
	return 0;
}
const cmd_t cmd_relay = {"relay", cmd_relay_func, "route board relay set cmds"};
DECLARE_SHELL_CMD(cmd_relay)

/*
TRIG	PC2	OUT
VMCOMP	PC3	IN
LE_D	PC6	OUT
LE_R	PC7	IN
*/
static void gpio_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_7;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_6;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	//vmcomp exti input
	EXTI_InitTypeDef EXTI_InitStruct;
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource3);
	EXTI_InitStruct.EXTI_Line = EXTI_Line3;
	EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Rising;
	EXTI_InitStruct.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStruct);

	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = EXTI3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

void EXTI3_IRQHandler(void)
{
	irc_vmcomp_pulsed = 1;
	EXTI->PR = EXTI_Line3;
}

/*TRIG PC2 OUT*/
void trig_set(int high) {
	irc_vmcomp_pulsed = 0;
	if(high) {
		GPIOC->BSRR = GPIO_Pin_2;
	}
	else {
		GPIOC->BRR = GPIO_Pin_2;
	}
}

/*VMCOMP PC3 IN*/
int trig_get(void) {
	return irc_vmcomp_pulsed;
}

/*LE_txd PC7*/
void le_set(int high) {
	if(high) {
		GPIOC->BSRR = GPIO_Pin_7;
	}
	else {
		GPIOC->BRR = GPIO_Pin_7;
	}
}

/*LE_rxd	PC6*/
int le_get(void) {
	return (GPIOC->IDR & GPIO_Pin_6) ? 1 : 0;
}

void board_init(void)
{
	gpio_init();
	rly_init();
}

void board_reset(void)
{
	NVIC_SystemReset();
}
