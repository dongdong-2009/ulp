/**
 *  \file fdsig.h
 *  \brief miaofng@2018-04-10 separate from fdsig.c
 */

#ifndef __FDSIG_H__
#define __FDSIG_H__

//enable on board tm7707 emulated DMM
#define CONFIG_DMM_INT 1

//dmm hardware driver
#define DMM_MCLK_HZ 4000000
#define DMM_ODAT_HZ 100
#define DMM_ISR_Y() do {EXTI->PR = EXTI_Line4; NVIC_EnableIRQ(EXTI4_IRQn);} while(0)
#define DMM_ISR_N() NVIC_DisableIRQ(EXTI4_IRQn)
#define DMM_DUMMY_N 50

#define ADC_REF 3.000 //unit: V
#define ACS_VDD 5.000 //unit: V
#define ACS_OFS ((ACS_VDD) / 2)
#define ACS_G05 0.185 //unit: V/A, +/-05A => +/-925mV
#define ACS_G20 0.096 //unit: V/A, +/-05A => +/-500mV
#define IBAT_RS 10.00 //unit: ohm

//0R->0.231V
//1K->1.294V - 1.0192V = 0.275
#define OHM_ROFS 0.230 //unit: ohm
#define OHM_IREF 0.001 //unit: A
#define DMM_VOFS (ADC_REF * 0.5 / (0.5 + 1.0)) //unit: V, R = Kohm
#define DMM_GAIN ((100.0 + 33.0) / 33.0) //R = Kohm

enum {
	E_OK,
	E_CAL_ZERO,
	E_CAL_BANK,
};

enum {
#if CONFIG_FDPLC_V1P0 == 1
	BANK_18V, //0~+/-18V
	BANK_06V, //0~+/-06V
	BANK_ACS, //-0V5~5V5
	BANK_1V5, //0~+/-1V5
#else
	BANK_ANY, //0~+/-6V
#endif
	NR_OF_BANK,
};

typedef struct { //y = g*x + d
	const char *name;
	const char *unit;
	float g;
	float d;
} dmm_cal_t;

typedef struct {
	const char *name;
	unsigned ainx	: 1; //always TM_AIN1(=0)
	unsigned gain	: 8; //2^(0~7)
	unsigned dg409	: 2; //0~3, only by fdsig_v1.0
	unsigned dg408	: 6; //0~7|NSS
	unsigned diagx	: 3; //0~7
	unsigned vcdpx	: 3; //0~6
} dmm_ch_t;

extern int sys_ecode;
extern const char *sys_emsg;

void dmm_hw_init(void);
int dmm_hw_config(int ainx, int gain);
int dmm_hw_poll(void);
int dmm_hw_read(float *v);

void dmm_init(void);
int dmm_read(float *v);
int dmm_flush(int n);
#define dmm_poll dmm_hw_poll
int dmm_switch_bank(int bank);
int dmm_switch_ch(const char *ch_name);
int dmm_cal_bank(int bank);

const dmm_ch_t *dmm_ch_search(const char *ch_name);
dmm_cal_t *dmm_cal_search(const char *ch_name);
const dmm_cal_t *dmm_cal_search_rom(const char *ch_name);

void sig_init(void);
void sig_update(void);

#endif
