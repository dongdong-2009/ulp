/*
 * 	miaofng@2013-2-16 initial version
 */

#ifndef __OID_H__
#define __OID_H__

#include "oid_dmm.h"

#define oid_rcal_mohm  10000
#define oid_short_threshold 1000
#define oid_hot_timeout_ms 90000
#define oid_hot_mv_th_ident 100
#define oid_hot_mv_th_diag 900

enum oid_error_type {
	OID_E_OK,

	OID_E_SYS_CAL,
	OID_E_SYS_DMM,
	OID_E_SYS_DMM_DATA,

	OID_E_GROUNDED_PIN_TOO_MUCH,
	OID_E_GROUNDED_PIN_LOST,
	OID_E_HEATWIRE_LOST,
	OID_E_PIN_SHORT_TO_HEATWIRE,

	OID_E_O2S_VOLTAGE_LOST,

};

enum {
	PIN_0,
	PIN_SHELL = PIN_0,

	PIN_1,
	PIN_GRAY = PIN_1,

	PIN_2,
	PIN_BLACK = PIN_2,

	PIN_3,
	PIN_WHITE0 = PIN_3,

	PIN_4,
	PIN_WHITE1 = PIN_4,

	NR_OF_PINS,
};

enum {
	FUNC_SHELL,
	FUNC_NONE = FUNC_SHELL,
	FUNC_GRAY,
	FUNC_BLACK,
	FUNC_WHITE0,
	FUNC_WHITE1,
};

struct o2s_s {
	int mohm;
	int min; //mohm_min
	int max; //mohm_max
	int grounded : 8;
	int lines : 8;
	int tcode;
};

struct oid_config_s {
	char lines; /*1..4*/
	char mode; /*'d' => diag mode, 'i' => ident mode*/
	char grounded; /*gray line is grounded? 'Y' or 'N' or '?'*/
};

/* note:
diag mode: kcode = lines(msb) + gnd + mohm(35 = 3500mohm)
idet mode: kcode = pin1->function(msb) + pin2->function + pin3->function + pin4->function
*/
struct oid_result_s {
	int kcode; /*0x000000 indicates unknown*/
	int tcode; /*0x000000 indicates unknown*/

	/*will be managed by oid_error()*/
	int ecode[3]; /*0x000000 indicates no error*/
};

/*
when idle: start a new test
when busy: halt test
*/
void oid_start(const struct oid_config_s *cfg);
void oid_get_result(struct oid_result_s *result); /*get test result*/

void oid_hot_set_ms(int ms);
int oid_hot_get_ms(void); /*for hot test time left display*/
int oid_hot_get_mv(void); /*for hot test get current o2s voltage output*/

#endif
