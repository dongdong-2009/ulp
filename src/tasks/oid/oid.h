/*
 * 	miaofng@2013-2-16 initial version
 */

#ifndef __OID_H__
#define __OID_H__

#define oid_sec_threshold 90
#define oid_mv_threshold 750
#define mohm_open_threshold _kohm(10)
#define mohm_short_threshold _ohm(1)

enum {
	PIN_SHELL,
	PIN_GRAY,
	PIN_BLACK,
	PIN_WHITE0,
	PIN_WHITE1,
	NR_OF_PINS,
};

struct oid_s {
	char seconds;
	char o2s[NR_OF_PINS]; /*o2 sensor pinmap*/
	int mohms[NR_OF_PINS][NR_OF_PINS];
	int mv; /*current o2 output voltage*/
	int mohm; /*heat wire*/
	int mohm_real;
	int tcode;
	int kcode;
	int ecode[3];
	unsigned start : 1;
	unsigned lock : 1; /*config operation is locked*/
	unsigned lines : 3;
	unsigned scnt : 8; /*seconds counter*/
	unsigned mode : 8; //'i', 'd'
	unsigned gnd : 8; //0x00->ground, 0x01->unground, '?'->unknown
};

struct oid_gui_s {
	time_t timer;
	int ecode[3];
	int tcode;
	int kcode;
	int mv;
	unsigned lock : 1;
	unsigned pbar : 3; /*progress bar*/
	unsigned scnt : 8;
	unsigned gnd : 8;
	
};

/*progress bar operation*/
enum {
	PROGRESS_START,
	PROGRESS_STOP,
	PROGRESS_BUSY,
};

extern int oid_stm;
extern struct oid_config_s oid_config;
extern void oid_mdelay(int ms);

void oid_gui_init(void);
void oid_show_result(int tcode, int kcode);
void oid_show_progress(int value);

/*error handling*/
enum oid_error_type {
	E_OK = 0,
	E_SYSTEM = 0x000100, /*oid system self check error*/
	E_STRANGE_RESISTOR = 0x01000000, /*strange resistance is found*/
	E_SHORT_OTHER = 0x000300, /*some pins are shorten except shell pin*/
	E_SHORT_SHELL_MORE = 0x000400, /*more than one pins are shorten to shell*/
	E_WIRE_MORE = 0x000500, /*more than one heating wire is found*/
	
	/*trans provided error codes*/
	E_SHORT_SHELL_GRAY = 0x020202,
	E_SHORT_SHELL_BLACK = 0x010101,
	E_SHORT_SHELL_WHITE = 0x030103,
	
	E_SHORT_GRAY_BLACK = 0x020101,
	E_SHORT_GRAY_WHITE = 0x000, //MF ADDED
	
	E_SHORT_BLACK_WHITE = 0x030105, //MF ADDED
	E_SHORT_WHITE_WHITE = 0x030106, //MF ADDED
	E_STRANGE_WHITE_WHITE = 0x030107, //MF ADDED
	
	E_OPEN_SHELL_GRAY = 0x020102,
	E_OPEN_WHITE_WHITE = 0x030102,
	
	E_LOSE_HIGH_VOLTAGE = 0x030104,
	E_UNDEF,
};

extern struct oid_s oid;
extern struct oid_gui_s gui;
void gui_error_flash(void);

#endif
