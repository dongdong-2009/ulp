/*
*
*  miaofng@2014-5-10   initial version
*	opcode < opgrp < opq
*
*/

#ifndef __IRTVM_H__
#define __IRTVM_H__

#include "config.h"

enum {
	VM_OPCODE_OPEN,
	VM_OPCODE_CLOSE,
	VM_OPCODE_SCAN,
	VM_OPCODE_SPECIAL,

	/*special opcode*/
	VM_OPCODE_SEQ = 12,
	VM_OPCODE_NUL = 15
};

typedef union opcode_u {
	struct {
		short line : 12;
		short bus : 2;
		short type : 2;
	};

	struct {
		short line : 12;
		short type : 4;
	} special;

	short value;
} opcode_t;

void vm_init(void);
int vm_fetch( //non-zero if error happened
	void *buf, //rx buffer,  normally = can_msg.data[8]
	int *bytes //input buf size, output bytes filled
);

#endif
