/*
*
*  miaofng@2014-5-10   initial version
*	opcode < opgrp < opq
*
*/

#ifndef __IRTVM_H__
#define __IRTVM_H__

#include "config.h"

/*vp230 could drive 120nodes: 32 * 120 = 3840 lines at most*/
#define NR_OF_SLOT_MAX 0064
#define NR_OF_LINE_MAX 2048
#define NR_OF_BUS_MAX 0004

/*scan queue over group boundary support
to fixed ulp shell cmdline short issue, such as in irt high
voltage test, scan_arm could be 20+ and one cmd cann't hold
the whole point list, then it should be seprate into several
ROUTE ARM 20
ROUTE SCAN XXXXXXXXX
ROUTE SCAN XXXXXXXXX
*/
#define VM_SCAN_OVER_GROUP 1

enum {
	VM_OPCODE_OPEN,
	VM_OPCODE_CLOS,
	VM_OPCODE_SCAN, /*SINGLE BUS SCAN*/
	VM_OPCODE_FSCN, /*FULL SCAN, for matrix relay self-diagnose */
	VM_OPCODE_SEQU,
	VM_OPCODE_GRUP,
	VM_OPCODE_NULL,
	VM_OPCODE_WAIT, /*Wn GFT command*/
};

typedef union opcode_u {
	struct {
		unsigned short bus : 2;
		unsigned short line : 11;
		unsigned short type : 3;
	};

	struct {
		unsigned short fscn : 13;
		unsigned short type : 3;
	} fscn;

	struct {
		unsigned short ms : 13; //max 8192
		unsigned short type : 3;
	} wait;

	unsigned short value;
} opcode_t;

/*delay before send dmm_trig signal
set by command ROUTE DELAY
to be used by mxc if mxc dmm trig function is supported
*/
int vm_get_measure_delay(void);

/*for VM_SCAN_OVER_GROUP support
return 0 in case scan list is over
*/
int vm_get_scan_cnt(void);

void vm_init(void);
void vm_update(void);

int vm_is_busy(void);
void vm_abort(void);
int vm_mode(int mode);

/*to be realized by matrix driver*/
extern void vm_execute(opcode_t opcode, opcode_t seq);
extern void vm_wait(int ms);

//vm debug support
void vm_opcode_print(opcode_t opcode);
void vm_dump(void);

#endif
