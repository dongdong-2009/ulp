/*
 * David@2011 init version
 */
#ifndef __C131_H_
#define __C131_H_
#include "osd/osd.h"
#include "can.h"

typedef enum {
	APT_MODE_NORMAL,
	APT_MODE_SIMULATOR,
} apt_mode_t;

typedef struct apt_load_s {
	int load_bExist;
	int load_option;
	char load_name[16];
	unsigned char load_ram[8];
} apt_load_t;

typedef struct dtc_s {
	unsigned char dtc_hb;
	unsigned char dtc_mb;
	unsigned char dtc_lb;
	unsigned char dtc_status;
} dtc_t;

typedef struct c131_dtc_s {
	short dtc_bExist;
	short dtc_bPositive;
	int dtc_len;
	dtc_t *pdtc;
} c131_dtc_t;

extern osd_dialog_t apt_dlg;

//for cmd module function
int apt_GetMode(void);
int apt_SetAPTRAM(unsigned char *p);
int apt_AddLoad(apt_load_t * pload);

//for dlg function define
int apt_GetSDMTypeName(void);
int apt_GetSDMTypeSelect(void);
int apt_SelectSDMType(int keytype);

int apt_GetSDMPWRName(void);
int apt_GetSDMPWRIndicator(void);
int apt_GetLEDPWRName(void);
int apt_GetLEDPWRIndicator(void);
int apt_SelectPWR(int keytype);

int apt_GetLinkInfo(void);
int apt_GetTypeInfo(void);

int apt_GetDiagInfo(void);
int apt_SelectAPTDiag(int keytype);

int apt_GetTestInfo(void);
int apt_SelectSDMTest(int keytype);

int apt_GetDTCInfo(void);
int apt_SelectSDMDTC(int keytype);
int c131_GetCurrentLoadIndex(void);
int c131_GetLoad(apt_load_t ** pload, int index_load);
//for can send
int c131_ClearHistoryDTC(void);
int c131_GetDTC(c131_dtc_t *pc131_dtc);
int c131_GetDiagInfo(can_msg_t *pReq, can_msg_t *pRes, int * plen);
int c131_GetEEPROMInfo(can_msg_t *pReq, can_msg_t *pRes, char data_len, int *plen);
#endif /*__C131_H_*/
