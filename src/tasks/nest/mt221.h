/*
 * 	miaofng@2010 initial version
 */

#ifndef __MT221_H__
#define __MT221_H__

// Defines (Verify DUT Signal 1) used in Cnd_Input_Capture.LIB.  11.49us/count
// Signal Monitored TACH, 50Hz, 50% duty cycle, 10mS
#define VDS1_DEF
#define VDS1_INTERVAL		(300)  // Frequency in milliseconds to run func.
#define VDS1_FAIL_CNT_LIM	(0) // Fail count limit., we do not want to use capture lib fail count function
#define VDS1_COUNT_LO_LIM	(784) // Pulse width count low limit. 9mS -> 784
#define VDS1_COUNT_HI_LIM	(957) // Pulse width count high limit. 11mS -> 957

// Defines (Verify DUT Signal 2) used in Cnd_Input_Capture.LIB. Signal Monitored IAC = 75Hz, 50% duty cycle. Applicable only for SGM
#define VDS2_DEF //pulse width of 75hz 50% is 6.6ms, 573 or 0x23D count.
#define VDS2_INTERVAL		(100) // Frequency in milliseconds to run fun.
#define VDS2_FAIL_CNT_LIM	(3)	// Fail count limit.
#define VDS2_COUNT_LO_LIM	(0x206)  // Pulse width count low limit. 0x206 is 6.0ms
#define VDS2_COUNT_HI_LIM	(0x27A)  // Pulse width cound high limit. 0x27A is 7.4ms

// Defines for CAN routines.
#define CAN_TIMEOUT		(100)	// milli-seconds.
#define CAN_MSG_DLY		(10)	  // 10 * 100us = 1ms intermessage delay.
#define CAN_KBAUD		(500)

//mt22.1 mcamos communication interface
#define INBOX_ADDR		(model.inbox_addr) //Memory Transfer Address
#define INBOX_SIZE		31
#define OUTBOX_ADDR		(model.outbox_addr)
#define OUTBOX_SIZE		64

//test IDs
#define FACTORY_TEST_MODE_TESTID	((UINT8)0x00)
#define RAM_TEST_TESTID			((UINT8)0x04)
#define MEMORY_READ_TESTID		((UINT8)0x05)
#define MEMORY_WRITE_TESTID		((UINT8)0x06)
#define EEPROM_WRITE_TESTID		((UINT8)0x07)
#define L9958_FAULTTEST_TESTID		((UINT8)0X0A)
#define FLASH_WRITE_TESTID		((UINT8)0x0B)
#define VSEP_FAULTTEST_TESTID		((UINT8)0X14)
#define LCM555_FREQTEST_TESTID		((UINT8)0X1A)

#define DFT_OP_TIMEOUT			((UINT32)100)//unit ms
#define CND_PERIOD			((UINT32)15*60*1000)//unit ms
#define RUN_FAULT_BYTE_TEST_DUTY	((UINT32)(10000))
#define FAULT_BYTES_THRESTHOLD		1
#define TACH_PULSE_THRESTHOLD		2

//IGBT on(1)/off(0)
#define RELAY_IGBT_SET(ON)	do \
{ \
	if(ON) SigControl(SIG1, SIG1_HI); \
	else SigControl(SIG1, SIG1_LO); \
	Delay10ms(5); /*wait 50ms for relay settle */ \
} while(0)

//ETC on(1)/off(0)
#define RELAY_ETC_SET(ON)	do \
{ \
	if(ON) SigControl(SIG2, SIG2_HI); \
	else SigControl(SIG2, SIG2_LO); /*IF SIG_2 = LO/DEF, THEN IAC = ON*/ \
	Delay10ms(5); /*wait 50ms for relay settle */ \
} while(0)

//MT22.1&60.1 mode on(1)/off(0)
#define RELAY_MT221_MT601_SET(ON)	do \
{ \
	if(ON) SigControl(SIG3, SIG3_HI); /*MT22.1&MT60.1*/\
	else SigControl(SIG3, SIG3_LO); /*MT18.1*/ \
	Delay10ms(5); /*wait 50ms for relay settle */ \
} while(0)

//LOCK on(1)/off(0)
#define RELAY_LOCK_SET(ON)	do \
{ \
	if(ON) SigControl(SIG4, SIG4_HI); \
	else SigControl(SIG4, SIG4_LO);  \
	Delay10ms(5); /*wait 50ms for relay settle */ \
} while(0)

//MT60.1 mode on(1)/off(0)
#define RELAY_MT601_SET(ON)	do \
{ \
	if(ON) SigControl(SIG5, SIG5_HI);/*MT60.1*/ \
	else SigControl(SIG5, SIG5_LO); /*MT22.1&MT18.1*/\
	Delay10ms(5); /*wait 50ms for relay settle */ \
} while(0)

//MT22.1 mode on(1)/off(0))
#define RELAY_MT221_SET(ON)	do \
{ \
	if(ON) SigControl(SIG6, SIG6_HI);/*MT22.1*/ \
	else SigControl(SIG6, SIG6_LO); /*MT18.1&MT60.1*/\
	Delay10ms(5); /*wait 50ms for relay settle */ \
} while(0)

#define RELAY_BAT_SET(ON) do \
{ \
	if(ON) WrBAT_ON(0);  /*inverted on circuit*/ \
	else WrBAT_ON(1); \
} while(0)

#define RELAY_IGN_SET(ON)  do \
{ \
	if(ON) WrIGN_ON(0); /*inverted on circuit*/ \
	else WrIGN_ON(1); \
} while(0)

#define RELAY_ETCBAT_SET(ON) do\
{ \
	if(ON) WrLSD1_PG1(0); /*inverted on circuit*/ \
	else WrLSD1_PG1(1); \
} while(0)

//data type definition
typedef struct {
	UINT32 inbox_addr;
	UINT32 outbox_addr;

	//test start/stop
	UINT32 pcb_seq_addr;
	UINT32 psv_amb_addr;
	UINT32 psv_cnd_addr;
	UINT32 mtype_addr;
	UINT32 fault_byte_addr;
	UINT32 error_code_addr;

	//ram test
	UINT32 ramtest_start_addr;
	UINT32 ramtest_end_addr;

	//cycling test
	const char *ramdnld;
	const char **ch_info;
} model_t;

#define MODEL_MT221	0
#define MODEL_MT181	1

typedef struct
{
	UINT8 igbt; // 1-> igbt, 0-> est
	UINT8 iac; // 1-> iac, 0-> etc
	UINT8 par; /* mt22.1: 1-> paried(2est/igbt), 0->sequencial(4est/igbt)
				 mt18.1: 2-> 2est/igbt, 3->3est/igbt, 4-> 4est/igbt */
	UINT8 mt601;
	UINT8 model;
} ModelType;

#define PRINT_MODEL_TYPE() do{ \
	if(mtype.par < 2) { if(mtype.par == 1) message("2"); else message("4"); } else { message("%d", mtype.par); } \
	if(mtype.igbt == 1) message("IGBT"); else message("EST"); \
	if(mtype.iac == 1) message(" + IAC"); else message(" + ETC"); \
	if(mtype.mt601== 1) message(" + MT60.1"); else message(" + NOTMT601"); \
	message("\n"); \
} while(0)

typedef struct {
	UINT8 flag;
	time_t dwFaultByteTimer;
	union {
		UINT8 buf[12];
		struct {
			UINT8 l9958;
			UINT8 vsep[8];
			UINT8 lcm555[3];
		} mt221;
		struct {
			UINT8 psvi[7];
			UINT8 up;
			UINT8 igbt;
		} mt181;
	} data;
} fault_bytes_t;

#define fault_bytes_init(fb) do{ \
	fb.flag = 0; \
} while(0)

#endif