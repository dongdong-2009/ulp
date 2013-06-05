#ifndef __MT621_H__
#define __MT621_H__

//Defines for CAN routines.
#define CAN_TIMEOUT		(100)	// milli-seconds.
#define CAN_MSG_DLY		(10)	// 10 * 100us = 1ms intermessage delay.
#define CAN_KBAUD		(500)	//Kbit per second

#define DFT_OP_TIMEOUT			((UINT32)100)//unit ms
#define RW_OP_TIMEOUT			((UINT32)1000)//uint ms
#define RUN_FAULT_BYTE_TEST_DUTY	((UINT32)(10000))

typedef struct
{
	UINT8 igbt; // 1-> igbt, 0-> est
	UINT8 iac; // 1-> iac, 0-> etc
	UINT8 par; /* mt22.1 & mt62.1 : 1-> paried(2est/igbt), 0->sequencial(4est/igbt)
			 mt18.1: 2-> 2est/igbt, 3->3est/igbt, 4-> 4est/igbt */
	UINT8 model;
} ModelType;

#define PRINT_MODEL_TYPE() do{ \
	if(mtype.par < 2) { if(mtype.par == 1) message("2"); else message("4"); } else { message("%d", mtype.par); } \
	if(mtype.igbt == 1) message("IGBT"); else message("EST"); \
	if(mtype.iac == 1) message(" + IAC"); else message(" + ETC"); \
	message("\n"); \
} while(0)

#endif