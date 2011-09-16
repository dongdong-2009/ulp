/*
 * David@2011 init version
 */
#ifndef __C131_H_
#define __C131_H_
#include "osd/osd.h"

typedef enum {
	C131_MODE_NORMAL,
	C131_MODE_SIMULATOR,
} c131_mode_t;

typedef enum {
	C131_STAGE1_RELAY,
	C131_STAGE1_CANMSG,
	C131_STAGE2_RELAY,
	C131_STAGE2_CANMSG,
	C131_GET_DTCMSG,
}c131_stage_t;

typedef enum {
	DIAGNOSIS_NOTYET,
	DIAGNOSIS_OVER,
}c131_diagnosis_t;

typedef enum {
	SDM_UNEXIST,
	SDM_EXIST,
}c131_exist_t;

typedef enum {
	C131_TEST_NOTYET,
	C131_TEST_ONGOING,
	C131_TEST_FAIL,
	C131_TEST_SUCCESSFUL,
}c131_test_t;

typedef struct c131_load_s {
	int load_bExist;
	int load_option;
	char load_name[16];
	unsigned char load_ram[8];
} c131_load_t;

extern osd_dialog_t c131_dlg;

int c131_GetLoad(c131_load_t ** pload, int index_load);
int c131_AddLoad(c131_load_t * pload);
int c131_ConfirmLoad(int index_load);
int c131_GetCurrentLoadIndex(void);

//for sdm working mode
int c131_SetMode(int workmode);
int c131_GetMode(void);

//for dlg function define
int c131_GetSDMType(int index_load, char *pname);
int c131_GetLinkInfo(void);
int c131_GetTestStatus(void);
int c131_SetTeststatus(int status);

#endif /*__C131_H_*/
