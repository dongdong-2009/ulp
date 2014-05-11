/*
*
*  miaofng@2014-5-10   initial version
*	opcode < opgrp < opq
*
*/

#ifndef __IRTVM_H__
#define __IRTVM_H__

#include "config.h"

#define NR_OF_LINE_MAX 9999
#define NR_OF_BUS_MAX 0004

enum {
	VM_OPCODE_OPEN,
	VM_OPCODE_CLOSE,
	VM_OPCODE_SCAN,
	VM_OPCODE_SPECIAL,

	/*special opcode*/
	VM_OPCODE_SEQ = 12,
	VM_OPCODE_GRP = 13, /*TAG of a new opgrp start*/
	VM_OPCODE_NUL = 15
};

typedef union opcode_u {
	struct {
		unsigned short line : 12;
		unsigned short bus : 2;
		unsigned short type : 2;
	};

	struct {
		unsigned short line : 12;
		unsigned short type : 4;
	} special;

	struct {
		unsigned short arm : 12;

	} seq;

	unsigned short value;
} opcode_t;

void vm_init(void);

/* give me your can_msg.data[8], i'll filled it for you:)
what about can id i should use?  pls check vm_opgrp_is_over()
*/
int vm_fetch( //non-zero if error happened
	void *buf, //rx buffer,  normally = can_msg.data[8]
	int *bytes //input buf size, output bytes filled
);
int vm_opgrp_is_over(void); //group finish
int vm_is_opc(void); //operation complete

#endif
