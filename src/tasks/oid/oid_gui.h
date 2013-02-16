/*
 * 	miaofng@2013-2-16 initial version
 */

#ifndef __OID_GUI_H__
#define __OID_GUI_H__

#include "gui/gui.h"

/*oid usr config*/
struct oid_config_s {
	unsigned start : 1;
	unsigned pause : 1;
	unsigned lines : 3;
	unsigned seconds : 9;
	unsigned mode : 8; //'i', 'd'
	const char *ground; //"N/Y/?"
};

/*oid state machine*/
enum {
	IDLE,
	BUSY,
	CAL,
	COLD,
	WAIT,
	HOT,
	FINISH,
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
	E_UNDEF = 0x1000000,
	E_SYS_ERR,
	E_DMM_INIT,
	E_MATRIX_INIT,
	E_RES_CAL,
	E_VOL_CAL,
};

void oid_error_init(void);
void oid_error(int ecode);
void oid_error_flash(void);

#endif
