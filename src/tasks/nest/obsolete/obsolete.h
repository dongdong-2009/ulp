#ifndef __OBSOLETE_H_
#define __OBSOLETE_H_

#include "ErrorCodes.h"
#include "TypesPlus.h"
#include "com_mcamOS.h"

#include "time.h"

/*time*/
#define NOW() nest_time_get(0)
#define SEC(time) (time/1000)
#define MIN(time) ((time)/60000)
#define Delay100us(n) nest_mdelay((n + 9) / 10)
#define Delay10ms(n) nest_mdelay(n * 10)
#define DelayS(n) nest_mdelay(n * 1000)

/*cncb i/f*/
enum {
	SIG1_LO = 0,
	SIG2_LO = 0,
	SIG3_LO = 0,
	SIG4_LO = 0,
	SIG5_LO = 0,
	SIG6_LO = 0,

	SIG1_HI = 1,
	SIG2_HI = 1,
	SIG3_HI = 1,
	SIG4_HI = 1,
	SIG5_HI = 1,
	SIG6_HI = 1,
};

#define InitCNCB cncb_init
#define PartNotPresent cncb_detect
#define PartPresent(debounce) (cncb_detect(debounce) == 0)

#define SigControl cncb_signal
#define WrBAT_ON(state) cncb_signal(BAT, state)
#define WrIGN_ON(state) cncb_signal(IGN, state)
#define WrLSD1_PG1(state) cncb_signal(LSD, state)
#define ToggleRun() nest_light(RUNNING_TOGGLE)

#define ICEnblDsbl(ch, state)
#define VerifyDUTSignal1()
#define VerifyDUTSignal2()

/*err handling*/
#define pass nest_pass
#define fail nest_fail
#define InitError nest_error_init
#define ERROR nest_error_set
#define ERROR_RETURN(TYPE, INFO) do \
{ \
	nest_error_set(TYPE, INFO); \
	return; \
} while(0)

/*light ctrl*/
#define NestLightControl nest_light
#define Flash_Err_Code nest_light_flash

/*debug message*/
#define message_clear nest_message_init
#define message nest_message

/*can bus*/
#define DW_CAN1 DW_CAN
#define DW_CAN2 DW_CAN

/*misc*/
#define xmem2root memcpy

typedef struct
{
	BOOL bUseFIS;
	//FIS_MSG_T FISMsg;

	//private data, buffer for FISMsg use
	char __type[8];  //BREQ/BCMP
	char __status[8]; //PASS/FAIL
	char __result[128];  //test result
	char __id[32]; //serial number or part ID.
	char __software[32];  //software ID.
	char __fixture[8];   //fixture ID.
} fis_data_t;
#endif