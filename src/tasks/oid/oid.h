/*
 * 	miaofng@2013-2-16 initial version
 */

#ifndef __OID_H__
#define __OID_H__

#include "oid_dmm.h"

#define oid_rcal_mohm		ocfg.rcal
#define oid_rcal_mohm_delta_max ocfg.rcal_delta
#define oid_short_threshold	ocfg.short_max
#define oid_hot_timeout_ms	ocfg.hot_time
#define oid_hot_mv_th_ident	ocfg.hot_ident
#define oid_hot_mv_th_diag	ocfg.hot_diag

enum oid_error_type {
	OID_E_OK,

	/*Test Device Error*/
	OID_E_SYS_CAL = 0x090001,
	OID_E_SYS_DMM_COMM,
	OID_E_SYS_DMM_DATA,
	OID_E_SYS_CFG_ERROR,

	OID_E_GROUNDED_PIN_TOO_MUCH = 0x020001,
	OID_E_GROUNDED_PIN_LOST,
	OID_E_HEATWIRE_LOST,
	OID_E_HEATWIRE_MANY,

	OID_E_O2S_VOLTAGE_LOST = 0x030001,
	OID_E_O2S_VOLTAGE_POLAR = 0x030002, /*black&gray pin exchange?*/
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
	PIN_5 = 5,
	PIN_CAL = PIN_5,
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
	char grounded;
	char lines : 4;
	char deleted : 1;
	char newbie : 1;
	unsigned short tcode;
};

struct oid_config_s {
	char cksum;
	char nr_of_sensors;
	int rcal;
	int rcal_delta;
	int short_max;
	int hot_time;
	int hot_ident;
	int hot_diag;
	struct o2s_s o2s_list[32];
};

struct o2s_config_s {
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
void oid_start(void);
int oid_is_busy(void);
void oid_set_config(const struct o2s_config_s *cfg);
void oid_get_result(struct oid_result_s *result); /*get test result*/

int oid_is_hot(void); //warming up?
void oid_hot_set_ms(int ms);
int oid_hot_get_ms(void); /*for hot test time left display*/
int oid_hot_get_mv(void); /*for hot test get current o2s voltage output*/

//oid_gui
void oid_gui_init(void);
void oid_gui_update(void);

#endif
