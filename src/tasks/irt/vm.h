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

enum {
	VM_OPCODE_OPEN,
	VM_OPCODE_CLOS,
	VM_OPCODE_SCAN, /*SINGLE BUS SCAN*/
	VM_OPCODE_FSCN, /*FULL SCAN, for matrix relay self-diagnose */
	VM_OPCODE_SEQU,
	VM_OPCODE_GRUP,
	VM_OPCODE_NULL,
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

	unsigned short value;
} opcode_t;

void vm_init(void);
void vm_update(void);

/*return nr of uncompleted events or 0 if all operation is completed*/
int vm_is_opc(void);
void vm_abort(void);
int vm_mode(int mode);

#endif
