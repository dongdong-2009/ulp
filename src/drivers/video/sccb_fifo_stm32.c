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
#include <string.h>

struct lpt_cfg_s lpt_cfg = LPT_CFG_DEF;

#define t0	((lpt_cfg.t - lpt_cfg.tp) >> 1)
#define t1	(lpt_cfg.tp)

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

#ifdef CONFIG_LPT_PINMAP_DEF16
/* pinmap:
	data bus		PC0~15
	addr bus		PB5
	we/wr		PB1
	oe		PB0
	cs/enable		PB10
*/
#define CONFIG_LPT_WIDTH_16BIT
#define db_pins ( \
	GPIO_Pin_0 | GPIO_Pin_1| GPIO_Pin_2| GPIO_Pin_3 | \
	GPIO_Pin_4 | GPIO_Pin_5| GPIO_Pin_6| GPIO_Pin_7 | \
	GPIO_Pin_8 | GPIO_Pin_9| GPIO_Pin_10| GPIO_Pin_11 | \
	GPIO_Pin_12 | GPIO_Pin_13| GPIO_Pin_14| GPIO_Pin_15 \
)
#define db_bank GPIOC
#define cs_set(level) GPIO_WriteBit(GPIOB, GPIO_Pin_10, level)
#define we_set(level) GPIO_WriteBit(GPIOB, GPIO_Pin_1, level)
#define oe_set(level) GPIO_WriteBit(GPIOB, GPIO_Pin_0, level)
//fast access mode
#define cs_set_H() GPIOB->BSRR = GPIO_Pin_10;
#define cs_set_L() GPIOB->BRR = GPIO_Pin_10;
#define we_set_H() GPIOB->BSRR = GPIO_Pin_1;
#define we_set_L() GPIOB->BRR = GPIO_Pin_1;
#define oe_set_H() GPIOB->BSRR = GPIO_Pin_0;
#define oe_set_L() GPIOB->BRR = GPIO_Pin_0;
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

#ifdef CONFIG_LPT_PINMAP_APT
/* pinmap:
	data bus		PD0~7
	addr bus		PD8
	we/wr			PD9
	oe				PD11
	cs/enable		PD10
*/
#define CONFIG_LPT_WIDTH_8BIT
#define db_pins ( \
	GPIO_Pin_0 | GPIO_Pin_1| GPIO_Pin_2| GPIO_Pin_3 | \
	GPIO_Pin_4 | GPIO_Pin_5| GPIO_Pin_6| GPIO_Pin_7 \
)
#define db_bank GPIOD
#define cs_set(level) GPIO_WriteBit(GPIOD, GPIO_Pin_10, level)
#define we_set(level) GPIO_WriteBit(GPIOD, GPIO_Pin_9, level)
#define oe_set(level) GPIO_WriteBit(GPIOD, GPIO_Pin_11, level)
//fast access mode
#define cs_set_H() GPIOD->BSRR = GPIO_Pin_10;
#define cs_set_L() GPIOD->BRR = GPIO_Pin_10;
#define we_set_H() GPIOD->BSRR = GPIO_Pin_9;
#define we_set_L() GPIOD->BRR = GPIO_Pin_9;
#define oe_set_H() GPIOD->BSRR = GPIO_Pin_11;
#define oe_set_L() GPIOD->BRR = GPIO_Pin_11;
#endif

#define ndelay(ns) do { \
	for(int i = ((ns) >> 3); i > 0 ; i -- ); \
} while(0)

static int lpt_init(const struct lpt_cfg_s *cfg)
{
	//default config
	if(cfg != NULL) {
		memcpy(&lpt_cfg, cfg, sizeof(struct lpt_cfg_s));
	}
	
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
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

#ifdef CONFIG_LPT_PINMAP_DEF16
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	//config ctrl pins as pushpull out
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0; //RD
	GPIO_InitStructure.GPIO_Pin |= GPIO_Pin_1; //WR
	GPIO_InitStructure.GPIO_Pin |= GPIO_Pin_5; //A0
	GPIO_InitStructure.GPIO_Pin |= GPIO_Pin_10; //CS
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
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

#ifdef CONFIG_LPT_PINMAP_APT
	//config addr bus as pushpull out
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOD, &GPIO_InitStructure);

	//config ctrl pins as pushpull out
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Pin |= GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Pin |= GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
#endif

	if(lpt_cfg.mode == LPT_MODE_M68) {
		cs_set(L);
	}
	else {
		cs_set(L); //cs_set(H);
		we_set(H);
		oe_set(H);

		//bus reset
#ifdef rst_set
		rst_set(L);
		ndelay(10000);
		rst_set(H);
#endif
	}

	return 0;
}

static inline int lpt_setaddr(int addr)
{
#ifdef CONFIG_LPT_PINMAP_DEF8
	GPIO_WriteBit(GPIOC, GPIO_Pin_9, (BitAction) (addr & 1));
#endif
#ifdef CONFIG_LPT_PINMAP_DEF16
	//GPIO_WriteBit(GPIOB, GPIO_Pin_5, (BitAction) (addr & 1));
	if(addr)
		GPIOB->BSRR = GPIO_Pin_5;
	else
		GPIOB->BRR = GPIO_Pin_5;
#endif
#ifdef CONFIG_LPT_PINMAP_ZF32
	GPIO_WriteBit(GPIOC, GPIO_Pin_10, (BitAction) (addr & 1));
#endif
#ifdef CONFIG_LPT_PINMAP_APT
	GPIO_WriteBit(GPIOD, GPIO_Pin_8, (BitAction) (addr & 1));
#endif
	return 0;
}

static inline int lpt_setdata(int data)
{
#ifdef CONFIG_LPT_WIDTH_8BIT
	GPIO_SetBits(db_bank, 0x00ff & data);
	GPIO_ResetBits(db_bank, 0x00ff & (~ data));
#else
	//GPIO_Write(db_bank, data);
	db_bank -> ODR = data;
#endif
	return 0;
}

static int lpt_getdata(void)
{
	int value = GPIO_ReadInputData(db_bank);
	
#ifdef CONFIG_LPT_WIDTH_8BIT
	value &= 0x00ff;
#else
	value &= 0xffff;
#endif
	return value;
}

static int lpt_write(int addr, int data)
{
#if CONFIG_LPT_MODE_M68 == 1
	lpt_setaddr(addr);
	lpt_setdata(data);
	we_set(L);
	ndelay(t0);
	cs_set(H); //enable
	ndelay(t1);
	cs_set(L);
	ndelay(t0);
#elif CONFIG_LPT_MODE_I80 == 1
	lpt_setaddr(addr);
	lpt_setdata(data);
	//cs_set(L);
	//ndelay(t0);
	we_set_L();
	ndelay(t1);
	we_set_H();
	//ndelay(t0);
	//cs_set(H);
#else
#error "bus timing mode not defined in .config file!!!"
#endif
	return 0;
}

static int lpt_writen(int addr, int data, int n)
{
	lpt_setaddr(addr);
	lpt_setdata(data);
	for(; n > 0; n--) {
#if CONFIG_LPT_MODE_M68 == 1
		we_set(L);
		ndelay(t0);
		cs_set(H); //enable
		ndelay(t1);
		cs_set(L);
		ndelay(t0);
#elif CONFIG_LPT_MODE_I80 == 1
		//cs_set(L);
		//ndelay(t0);
		we_set_L();
		ndelay(t1);
		we_set_H();
		//ndelay(t0);
		//cs_set(H);
#else
#error "bus timing mode not defined in .config file!!!"
#endif
	}
	return 0;
}

static int lpt_writeb(int addr, const void *buf, int n)
{
	short *data = (short *) buf;
	lpt_setaddr(addr);
	for(;n > 0; n --) {
#if CONFIG_LPT_MODE_M68 == 1
		lpt_setdata(*data ++);
		we_set(L);
		ndelay(t0);
		cs_set(H); //enable
		ndelay(t1);
		cs_set(L);
		ndelay(t0);
#elif CONFIG_LPT_MODE_I80 == 1
		lpt_setdata(*data ++);
		//cs_set(L);
		//ndelay(t0);
		we_set_L();
		ndelay(t1);
		we_set_H();
		//ndelay(t0);
		//cs_set(H);
#else
#error "bus timing mode not defined in .config file!!!"
#endif
	}
	return 0;
}

static int lpt_read(int addr)
{
	int data;

	//gpio output -> input
#ifdef CONFIG_LPT_DB_PP
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = db_pins;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(db_bank, &GPIO_InitStructure);
#else /*CONFIG_LPT_DB_OD*/
	lpt_setdata(0xffff); //open drain bus, to avoid pull down
#endif

#if CONFIG_LPT_MODE_M68 == 1
	lpt_setaddr(addr);
	we_set(H);
	ndelay(t0);
	cs_set(H); //enable
	ndelay(t1);
	data = lpt_getdata();
	cs_set(L);
	ndelay(t0);
#elif CONFIG_LPT_MODE_I80 == 1
	lpt_setaddr(addr);
	//cs_set(L);
	//ndelay(t0);
	oe_set_L(); //read op
	ndelay(t1);
	data = lpt_getdata();
	oe_set_H();
	//ndelay(t0);
	//cs_set(H);
#else
	#error "bus timing mode not defined in .config file!!!"
#endif

	//gpio input -> output
#ifdef CONFIG_LPT_DB_PP
	GPIO_InitStructure.GPIO_Pin = db_pins;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(db_bank, &GPIO_InitStructure);
#endif

	return data;
}

const lpt_bus_t lpt = {
	.init = lpt_init,
	.write = lpt_write,
	.read = lpt_read,
	.writeb = lpt_writeb,
	.writen = lpt_writen,
};
