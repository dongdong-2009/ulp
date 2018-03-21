/**
 *  fdplc.h header for ford plc
 *  miaofng@2018-03-08
 */

#ifndef __FDPLC_H__
#define __FDPLC_H__

#define CAN_BAUD 500000
#define CAN_ID_SCAN 0x200 //!low priority
#define CAN_ID_HOST 0x100 //0x101..120 is test unit
#define CAN_ID_UNIT(N) (CAN_ID_HOST + (N))

#define FD_NPLC 12
#define FD_SCAN_MS 1200
#define FD_ALIVE_MS (FD_SCAN_MS << 1)

typedef enum {
	FDCMD_POLL, //host: cmd -> slave: response status
	FDCMD_SCAN, //host: cmd+rsel+csel

	FDCMD_MOVE, //host: cmd
	FDCMD_TEST, //host: cmd + mysql_record_id

	//only for debug purpose, do not use it normal testing!!!!
	FDCMD_PASS, //host: cmd, identical with cmd "TEST PASS"
	FDCMD_FAIL, //host: cmd, identical with cmd "TEST FAIL"
	FDCMD_ENDC, //host: cmd, clear test_end request, indicates all cyl has been released
	FDCMD_WDTY, //host: cmd, enable test unit wdt
	FDCMD_WDTN, //host: cmd, disable test unit wdt
	FDCMD_UUID, //host: cmd, query fdplc board uuid
} fdcmd_type_t;

typedef struct {
	unsigned dst : 4; //id@destination test unit
	unsigned cmd : 4;
	union {
		struct {
			unsigned img : 16;
			unsigned msk : 16;
		} move;

		struct {
			int sql_id;
		} test;
	};
} fdcmd_t;

typedef struct {
	unsigned src : 4; //id@source test unit
	unsigned cmd : 4;
	unsigned model: 8;  //fixture model type
	unsigned pushed : 16; //probe counter, low 16bit
	unsigned sensors; //bit31: end, bit30: ng, bit29
	#define FDRPT_MASK_UUTE (1 << 01) //!!! 'UE'
	#define FDRPT_MASK_SM_N (1 << 17) //!!! 'SM-'
	#define FDRPT_MASK_TEND (1 << 31)
	#define FDRPT_MASK_TNGD (1 << 30)
	#define FDRPT_MASK_TDBG (1 << 29) //teststand is in offline test mode
	#define FDRPT_MASK_TERR (1 << 28) //test unit has error
} fdrpt_t;

/*fdplc private*/
#define FDPLC_IOF_MS 50 //sensor gpi filter
#define FDPLC_WDT_MS 3000 //test unit alive watchdog
#define FDPLC_NVM_MAGIC 0x3ABC

typedef enum {
	FDPLC_STATUS_PASS,
	FDPLC_STATUS_FAIL,
	FDPLC_STATUS_TEST,

	FDPLC_EVENT_WDTY,
	FDPLC_EVENT_WDTN,
	FDPLC_EVENT_WDT_TIMEOUT,
	FDPLC_EVENT_WDT_OK,

	FDPLC_EVENT_HOST_LOST,
	FDPLC_EVENT_HOST_OK,

	FDPLC_EVENT_CAN_SEND_TIMEOUT,
	FDPLC_EVENT_CAN_SEND_OK,

	FDPLC_EVENT_OFFLINE, //indicates unit is in ts debug mode
	FDPLC_EVENT_ONLINE,

	FDPLC_EVENT_FAIL_CAL,
	FDPLC_EVENT_FAIL_MCP,
	FDPLC_EVENT_FAIL_NVM,
	FDPLC_EVENT_GOOD_NVM,

	FDPLC_EVENT_INIT, /*green on*/
	FDPLC_EVENT_ASSIGN,
	FDPLC_EVENT_PASS, /*green on*/
	FDPLC_EVENT_FAIL, /*red on*/
	FDPLC_EVENT_TEST, /*yellow flash*/
} fdplc_event_t;

typedef enum {
	FDPLC_E_OK,
	FDPLC_E_HOST,
	FDPLC_E_SEND,
	FDPLC_E_MCP,
	FDPLC_E_NVM,
	FDPLC_E_CAL,
	FDPLC_E_WDT, //ni teststand died
} fdplc_ecode_t;

typedef struct {
	unsigned assigned : 1;
	unsigned wdt_y : 1;
	unsigned test_end : 1;
	unsigned test_ng : 1;
	unsigned test_offline : 1; //teststand is in offline test mode
	unsigned test_status : 4; //save for status restore
	unsigned ecode : 16;
} fdplc_status_t;

typedef struct {
	unsigned pushed; //pushed counter
	unsigned model : 8; //fixture model type

	//to identify eeprom correct
	unsigned magic : 16;
} fdplc_eeprom_t;

void fdplc_on_event(fdplc_event_t event);
#endif
