/*
*
*  miaofng@2014-9-23   separate driver from mxc main routine
*
*/
#include "ulp/sys.h"
#include "led.h"
#include <string.h>
#include "shell/shell.h"
#include "spi.h"
#include "mbi5025.h"
#include "common/bitops.h"
#include "stm32f10x.h"
#include "umx_bsp.h"
#include "vm.h"
#include "err.h"

static const spi_bus_t *mxc_spi = &spi1;
static const mbi5025_t mxc_mbi = {.bus = &spi1, .idx = 0};
static int vmcomp_pulsed = 0;

/* pinmap:

pc7		o	trig
pc6		i	vmcomp

pa4		o	le
pc4		o	oe
pc8		o	miso_sel
*/
static void mxc_gpio_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

	//to avoid relay action between gpio init
	oe_set(1);

	//GPO
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_7 | GPIO_Pin_8;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	//GPI
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	//VMCOMP
	EXTI_InitTypeDef EXTI_InitStruct;
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource6);
	EXTI_InitStruct.EXTI_Line = EXTI_Line6;
	EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Rising;
	EXTI_InitStruct.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStruct);
}

/*LE_rxd	PC6*/
void EXTI9_5_IRQHandler(void)
{
	vmcomp_pulsed = 1;
	EXTI->PR = EXTI_Line6;
}

void trig_set(int high) {
	vmcomp_pulsed = 0;
	if(high) {
		GPIOC->BSRR = GPIO_Pin_7;
	}
	else {
		GPIOC->BRR = GPIO_Pin_7;
	}
}

int trig_get(void) {
	return vmcomp_pulsed;
}

void oe_set(int high)
{
	if(high) {
		GPIOC->BSRR = GPIO_Pin_4;
	}
	else {
		GPIOC->BRR = GPIO_Pin_4;
	}
}

void le_set(int high)
{
	if(high) {
		GPIOA->BSRR = GPIO_Pin_4;
	}
	else {
		GPIOA->BRR = GPIO_Pin_4;
	}
}

void miso_sel(int board)
{
	unsigned short odr = GPIOC->ODR;
	odr &= ~GPIO_Pin_8;
	odr |= (board & 0x01) ? GPIO_Pin_8 : 0;
	GPIOC->ODR = odr;
}

void board_init(void)
{
	mxc_gpio_init();
	mbi5025_Init(&mxc_mbi);

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

void board_reset(void)
{
	NVIC_SystemReset();
}

//relay image memory
static char mxc_image[20];
static char mxc_image_static[20]; //hold static open/closed relays

void mxc_relay_clr_all(void)
{
	//32lines * 4buses / 8 = 16Bytes
	memset(mxc_image, 0x00, 16);
}

//bus 0..3	v+/v-/i+/i-
void mxc_relay_set(int line, int bus, int on)
{
	int n = (line << 2) + bus;

#if 1 //sjc's hardware bug
	if(n & 0x10) {
		int nshift = n & 0x0f;
		n = (n & 0xfff0) + (15 - nshift);
	}
#endif

	if(on) {
		bit_set(n, mxc_image);
	}
	else {
		bit_clr(n, mxc_image);
	}
}

void mxc_init(void)
{
	mxc_relay_clr_all();
	mxc_execute();
	oe_set(0);
}

int mxc_switch(int line, int bus, int opcode)
{
	switch(opcode) {
	case VM_OPCODE_CLOS:
	case VM_OPCODE_SCAN:
	case VM_OPCODE_FSCN:
		mxc_relay_set(line, bus, 1);
		break;
	case VM_OPCODE_OPEN:
		mxc_relay_set(line, bus, 0);
		break;
	default:
		irc_error(-IRT_E_OPCODE);
	}
	return 0;
}

void mxc_image_store(void)
{
	memcpy(mxc_image_static, mxc_image, sizeof(mxc_image));
}

void mxc_image_restore(void)
{
	memcpy(mxc_image, mxc_image_static, sizeof(mxc_image));
}

void mxc_image_write(void)
{
	char *image = mxc_image;

	//shift out .. refer to mbi5025_write_and_latch
	for(int i = sizeof(mxc_image) - 1; i >= 0; i --) {
		char byte = image[i];
		mxc_spi->wreg(0, byte);
	}
}

void mxc_execute(void)
{
	mxc_image_write();

	le_set(1);
	le_set(0);
}

static int cmd_mxc_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"mxc [set|clr] <line> <bus>	turn on/off line:bus\n"
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
		if(!strcmp(argv[1], "scan")) {
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
