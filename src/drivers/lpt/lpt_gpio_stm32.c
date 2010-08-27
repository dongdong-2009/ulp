/*
*	This bus driver is used to emulate a virtual parallel port to connect lcd or some other device
*	note:
*		this parallel bus must be pulled up externally
*
*	miaofng@2010 initial version
*/

#include "config.h"
#include "stm32f10x.h"
#include "lpt.h"

#define t0	((CONFIG_LPT_T - CONFIG_LPT_TP) / 2)
#define t1	CONFIG_LPT_TP

//active level define
#define H Bit_SET
#define L Bit_RESET

#ifdef CONFIG_LPT_PINMAP_DEF8
/* pinmap:
	data bus		PC0~7
	addr bus		PC9
	we/wr		PC8
	oe			PC13
	cs/enable		PC11
*/
#define CONFIG_LPT_WIDTH_8BIT
#define db_pins ( \
	GPIO_Pin_0 | GPIO_Pin_1| GPIO_Pin_2| GPIO_Pin_3 | \
	GPIO_Pin_4 | GPIO_Pin_5| GPIO_Pin_6| GPIO_Pin_7 \
)
#define db_bank GPIOC
#define cs_set(level) GPIO_WriteBit(GPIOC, GPIO_Pin_11, level)
#define we_set(level) GPIO_WriteBit(GPIOC, GPIO_Pin_8, level)
#define oe_set(level) GPIO_WriteBit(GPIOC, GPIO_Pin_13, level)
#endif

#ifdef CONFIG_LPT_PINMAP_ZF32
/* pinmap:
	data bus		PC0~7
	addr bus		PC10
	we/wr		PC9
	oe			PC8
	cs/enable		PC12
	rst			PC11
*/
#define CONFIG_LPT_WIDTH_8BIT
#define db_pins ( \
	GPIO_Pin_0 | GPIO_Pin_1| GPIO_Pin_2| GPIO_Pin_3 | \
	GPIO_Pin_4 | GPIO_Pin_5| GPIO_Pin_6| GPIO_Pin_7 \
)
#define db_bank GPIOC
#define cs_set(level) GPIO_WriteBit(GPIOC, GPIO_Pin_12, level)
#define we_set(level) GPIO_WriteBit(GPIOC, GPIO_Pin_9, level)
#define oe_set(level) GPIO_WriteBit(GPIOC, GPIO_Pin_8, level)
#define rst_set(level) GPIO_WriteBit(GPIOC, GPIO_Pin_11, level)
#endif 

#define ndelay(ns) do { \
	for(int i = ((ns) >> 3); i > 0 ; i -- ); \
} while(0)

static int lpt_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

	//config data bus as open drain output
	GPIO_InitStructure.GPIO_Pin = db_pins;
#ifdef CONFIG_LPT_DB_OD
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
#endif
#ifdef CONFIG_LPT_DB_PP
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
#endif
	GPIO_Init(db_bank, &GPIO_InitStructure);

#ifdef CONFIG_LPT_PINMAP_DEF8
	//config addr bus as pushpull out
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	//config ctrl pins as pushpull out
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Pin |= GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Pin |= GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
#endif

#ifdef CONFIG_LPT_PINMAP_ZF32
	//config addr bus as pushpull out
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	//config ctrl pins as pushpull out
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Pin |= GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Pin |= GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Pin |= GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
#endif

#ifdef CONFIG_LPT_MODE_1602
	cs_set(L);
#else
	cs_set(H);
	we_set(H);
	oe_set(H);

	//bus reset
	#ifdef rst_set
	rst_set(L);
	ndelay(10000);
	rst_set(H);
	#endif
#endif

	return 0;
}

static int lpt_setaddr(int addr)
{
#ifdef CONFIG_LPT_PINMAP_DEF8
	GPIO_WriteBit(GPIOC, GPIO_Pin_9, (BitAction) (addr & 1));
#endif
#ifdef CONFIG_LPT_PINMAP_ZF32
	GPIO_WriteBit(GPIOC, GPIO_Pin_10, (BitAction) (addr & 1));
#endif
	return 0;
}

static int lpt_setdata(int data)
{
#ifdef CONFIG_LPT_WIDTH_8BIT
	GPIO_SetBits(db_bank, 0x00ff & data);
	GPIO_ResetBits(db_bank, 0x00ff & (~ data));
#else
	GPIO_Write(db_bank, data);
#endif
	return 0;
}

static int lpt_getdata(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	int value;

#ifdef CONFIG_LPT_DB_PP
	//gpio output -> input
	GPIO_InitStructure.GPIO_Pin = db_pins;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(db_bank, &GPIO_InitStructure);
#endif
	
	value = GPIO_ReadInputData(db_bank);
	
#ifdef CONFIG_LPT_WIDTH_8BIT
	value &= 0x00ff;
#else
	value &= 0xffff;
#endif

#ifdef CONFIG_LPT_DB_PP
	//gpio input -> output
	GPIO_InitStructure.GPIO_Pin = db_pins;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(db_bank, &GPIO_InitStructure);
#endif

	return value;
}

static int lpt_write(int addr, int data)
{
#ifdef CONFIG_LPT_MODE_1602
	lpt_setaddr(addr);
	lpt_setdata(data);
	we_set(L);
	ndelay(t0);
	cs_set(H); //enable
	ndelay(t1);
	cs_set(L);
	ndelay(t0);
#else
	lpt_setaddr(addr);
	lpt_setdata(data);
	cs_set(L);
	ndelay(t0);
	we_set(L);
	ndelay(t1);
	we_set(H);
	ndelay(t0);
	cs_set(H);
#endif

	return 0;
}

static int lpt_read(int addr)
{
	int data;

#ifdef CONFIG_LPT_MODE_1602
	lpt_setaddr(addr);
	lpt_setdata(0xffff); //open drain bus, to avoid pull down
	we_set(H);
	ndelay(t0);
	cs_set(H); //enable
	ndelay(t1);
	data = lpt_getdata();
	cs_set(L);
	ndelay(t0);
#else
	lpt_setaddr(addr);
	lpt_setdata(0xffff); //open drain bus, to avoid pull down
	cs_set(L);
	ndelay(t0);
	oe_set(L); //read op
	ndelay(t1);
	data = lpt_getdata();
	oe_set(H);
	ndelay(t0);
	cs_set(H);
#endif

	return data;
}

lpt_bus_t lpt = {
	.init = lpt_init,
	.write = lpt_write,
	.read = lpt_read,
};
