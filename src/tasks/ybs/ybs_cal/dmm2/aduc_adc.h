/*
 * 	miaofng@2013 initial version
 */

#ifndef __ADUC_ADC_H__
#define __ADUC_ADC_H__

#define ADUC_ADC0	0x00
#define ADUC_ADC1	0x01

#define ADUC_MUX0_DCH01	0x00
#define ADUC_MUX0_SCH05	0x01
#define ADUC_MUX0_SCH15	0x02
#define ADUC_MUX0_DCH23	0x05
#define ADUC_MUX0_SCH25	0x06
#define ADUC_MUX0_SCH35	0x07
#define ADUC_MUX0_VREF  0x03

#define ADUC_MUX1_DCH23	0x00
#define ADUC_MUX1_DCH45 0x01
#define ADUC_MUX1_DCH67	0x02
#define ADUC_MUX1_DCH89 0x03
#define ADUC_MUX1_SCH25 0x04
#define ADUC_MUX1_SCH35 0x05
#define ADUC_MUX1_SCH45 0x06
#define ADUC_MUX1_SCH65 0x07
#define ADUC_MUX1_SCH75 0x08
#define ADUC_MUX1_SCH86 0x09
#define ADUC_MUX1_SCH95 0x0A
#define ADUC_MUX1_SCHTS	0x0B
#define ADUC_MUX1_VREF	0x0C

#define ADUC_GAIN_PGA(pga) (1 << (pga))
#define ADUC_IEXC_UA(cfg) (cfg->iexc * 200UL + cfg->ua10 * 10UL)

#define ADUC_ADC_MODE_PWRD	0x00
#define ADUC_ADC_MODE_AUTO	0x01
#define ADUC_ADC_MODE_TRIG	0x02
#define ADUC_ADC_MODE_IDLE	0x03
#define ADUC_ADC_MODE_ZCAL_SELF	0x04 /*result -> ADCxOF*/
#define ADUC_ADC_MODE_GCAL_SELF	0x05 /*result -> ADCxGN, ADC0 & G=1 only*/
#define ADUC_ADC_MODE_ZCAL_SYS	0x06
#define ADUC_ADC_MODE_GCAL_SYS	0x07

typedef union {
	struct {
		unsigned ADC0RDY : 1;
		unsigned ADC1RDY : 1;
		unsigned DUMMY0 : 1;
		unsigned ADC0OVR : 1;
		unsigned ADC0THEX : 1;
		unsigned DUMMY1 : 1;
		unsigned ADC0ATHEX : 1;
		unsigned DUMMY2 : 5;
		unsigned ADC0CERR : 1; /*primary adc conv err*/
		unsigned ADC1CERR : 1; /*auxiliary adc conv err*/
		unsigned DUMMY3 : 1;
		unsigned ADCCALSTA : 1; /*CAL FINISH*/
	};
	unsigned value;
} aduc_adc_status_t;

typedef union {
	struct {
		unsigned adc0: 1; //adc0 enable/disable
		unsigned adc1: 1; //adc1 enable/disable
		unsigned iexc: 3; //200uA*N, N=0..5
		unsigned ua10: 1; //10uA on/off
		unsigned pga0: 4; //2^N, N=0..9
		unsigned pga1: 2; //2^N, N=0..3
		unsigned mux0: 4; //ADC0/ADC1@0, ADC2/ADC3@5
		unsigned mux1: 4; //ADC2/ADC3@0, TSENSOR@B
		unsigned vofs: 12; //offset common mode voltage, unit: mv, 0-2500mV
	};
	unsigned value;
} aduc_adc_cfg_t;

enum {
	ADUC_CAL_NONE,
	ADUC_CAL_FULL,
	ADUC_CAL_ZERO,
	ADUC_CAL_GAIN,
};

int aduc_adc_init(const aduc_adc_cfg_t *cfg, int cal);
/*!!! adc0 must be read before adc1 if it's useful*/
int aduc_adc_is_ready(int adc); //to poll adc convert status, 1->ready, 0->busy
int aduc_adc_get(int *adc0, int *adc1); //blocked adc conversion until ready, 0->ok, <0 ->overrange

#endif
