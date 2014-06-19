/*
*
*  miaofng@2014-6-6   separate driver from irc main routine
*
*/
#include "ulp/sys.h"
#include "irc.h"
#include "err.h"
#include "stm32f10x.h"
#include "can.h"
#include "bsp.h"
#include "mbi5025.h"

static volatile int irc_vmcomp_pulsed;
const mbi5025_t irc_mbi = {.bus = &spi2, .load_pin = SPI_2_NSS, .oe_pin = SPI_CS_PB12};

#define K0 (1 << 0)
#define K1 (1 << 1)
#define K2 (1 << 2)
#define K3 (1 << 3)
#define K4 (1 << 4)
#define K5 (1 << 5)
#define K6 (1 << 6)
#define K7 (1 << 7)
#define K8 (1 << 8)
#define K9 (1 << 9)
#define K10 (1 << 10)
#define K11 (1 << 11)
#define K12 (1 << 12)
#define K13 (1 << 13)
#define K14 (1 << 14)
#define K15 (1 << 15)

static void _rly_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);

	/*PE0~PE7*/
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | \
		GPIO_Pin_3 |GPIO_Pin_4 |GPIO_Pin_5 |GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_Init(GPIOE, &GPIO_InitStructure);
}

void _rly_set(int mode)
{
	/*
	enum {
		IRC_MODE_HVR = 0x7f | IRC_MASK_IBUS | IRC_MASK_ELNE | IRC_MASK_ELV,
		IRC_MODE_L4R = 0x48 | IRC_MASK_IBUS | IRC_MASK_ELNE | IRC_MASK_EIS,
		IRC_MODE_W4R = 0x08 | IRC_MASK_IBUS | IRC_MASK_ELNE | IRC_MASK_ELV | IRC_MASK_EIS,
		IRC_MODE_L2R = 0x00 | IRC_MASK_IBUS | IRC_MASK_ELNE | IRC_MASK_ELV,
		IRC_MODE_L2T = IRC_MODE_L2R,
		IRC_MODE_RMX = 0x1c | IRC_MASK_IBUS | IRC_MASK_ILNE,
		IRC_MODE_VHV = 0x02 | IRC_MASK_EBUS | IRC_MASK_ILNE,
		IRC_MODE_VLV = 0x80 | IRC_MASK_IBUS | IRC_MASK_ELNE | IRC_MASK_ADD, //!!!ISSUE!!!
		IRC_MODE_IIS = 0x21 | IRC_MASK_IBUS | IRC_MASK_ELNE,
	};
	*/
	const char image[] = {0x7f, 0x48, 0x08, 0x00, 0x1c/*probe mode*/, 0x1c, 0x02, 0x80, 0x21, 0x00};
	sys_assert((mode >= IRC_MODE_HVR) && (mode <= IRC_MODE_OFF));

	int y = GPIOE->ODR;
	y &= 0xff00;
	y |= image[mode];
	GPIOE->ODR = y;
}

static void rly_init(void)
{
	_rly_init();
	mbi5025_DisableOE(&irc_mbi);
	mbi5025_Init(&irc_mbi);
}

void rly_set(int mode)
{
	_rly_set(mode);
	const short image[] = {
		/*HVR*/ K0|K2,
		/*L4R*/ K7|K4,
		/*W4R*/ K3|K7,
		/*L2T*/ K3|K10,
		/*RPB*/ K5|K8,
		/*RMX*/ K5|K8,
		/*VHV*/ K6|K9,
		/*VLV*/ K6|K11,
		/*IIS*/ K1|K13,
		0x00,
	};

	sys_assert((mode >= IRC_MODE_HVR) && (mode <= IRC_MODE_OFF));
	short relays = image[mode];
	mbi5025_write_and_latch(&irc_mbi, &relays, sizeof(relays));
	mbi5025_EnableOE(&irc_mbi);
}

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
