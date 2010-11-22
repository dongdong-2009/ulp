/*
 * 	miaofng@2010 initial version
 */

#ifndef __MT92_H__
#define __MT92_H__

// Defines (Verify DUT Signal 1) used in Cnd_Input_Capture.LIB.  11.49us/count
// Signal Monitored TACH, 50Hz, 50% duty cycle, 10mS
#define VDS1_DEF
#define VDS1_INTERVAL		(300)  // Frequency in milliseconds to run func.
#define VDS1_FAIL_CNT_LIM	(0) // Fail count limit., we do not want to use capture lib fail count function
#define VDS1_COUNT_LO_LIM	(0x82) // Pulse width count low limit. 1.5mS
#define VDS1_COUNT_HI_LIM	(0xD9) // Pulse width count high limit. 2.5mS

// Defines (Verify DUT Signal 2) used in Cnd_Input_Capture.LIB. Signal Monitored IAC = 75Hz, 50% duty cycle. Applicable only for SGM
#define VDS2_DEF //pulse width of 75hz 50% is 6.6ms, 573 or 0x23D count.
#define VDS2_INTERVAL		(100) // Frequency in milliseconds to run fun.
#define VDS2_FAIL_CNT_LIM	(3)	// Fail count limit.
#define VDS2_COUNT_LO_LIM	(0x206)  // Pulse width count low limit. 0x206 is 6.0ms
#define VDS2_COUNT_HI_LIM	(0x27A)  // Pulse width cound high limit. 0x27A is 7.4ms

// Defines for CAN routines.
#define CAN_TIMEOUT		(100)	// milli-seconds.
#define CAN_MSG_DLY		(10)	  // 10 * 100us = 1ms intermessage delay.
#define CAN_KBAUD			(500)

#define D4_ADDR ((UINT32)0XD0008700)
#define A4_ADDR ((UINT32)0XD0008710)
#define D2_ADDR ((UINT32)0XD0008800)

#define PSV_CND_ADDR         ((UINT32)(0x80008015)) // Conditioning Flag Address
#define PSV_AMB_ADDR           ((UINT32)(0x80008017)) // Set the conditioning flag as failed at start of the testing
#define PCB_SEQ_ADDR       ((UINT32)(0x80008004)) // Sequence number address, applicable to PML SuZhou only.
#define BASE_MODEL_TYPE_ADDR    ((UINT32)(0x80008009)) // Base model address, applicable to PML SuZhou only.
#define FB_ADDR              ((UINT32)(0X80008020))
#define SEQ_NUM_SIZE    (4)
#define BASE_MODEL_TYPE_SIZE (8)

#define DFT_OP_TIMEOUT				((UINT32)100)//unit ms
#define CND_PERIOD					((UINT32)15*60*1000)//unit ms
#define RUN_FAULT_BYTE_TEST_DUTY	((UINT32)(63000))
#define FAULT_BYTES_THRESTHOLD	10
#define TACH_PULSE_THRESTHOLD		2

//IGBT on(1)/off(0)
#define RELAY_SIG1_SET(ON)	do \
{ \
	if(ON) SigControl(SIG1, SIG1_HI); \
	else SigControl(SIG1, SIG1_LO); \
	Delay10ms(5); /*wait 50ms for relay settle */ \
} while(0)

//ETC on(1)/off(0)
#define RELAY_SIG2_SET(ON)	do \
{ \
	if(ON) SigControl(SIG2, SIG2_HI); \
	else SigControl(SIG2, SIG2_LO); /*IF SIG_2 = LO/DEF, THEN IAC = ON*/ \
	Delay10ms(5); /*wait 50ms for relay settle */ \
} while(0)

//MT22.1 mode on(1)/off(0)
#define RELAY_SIG3_SET(ON)	do \
{ \
	if(ON) SigControl(SIG3, SIG3_HI); /*MT22.1*/\
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

#endif