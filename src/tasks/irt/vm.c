/*
*
*  miaofng@2014-5-10   initial version
*  1, only matrix relay operation will be buffered in opq(bus&line sw is configured by command 'mode')
*  2, ARM value could be changed only when vm_opq_is_empty
*/

#include "ulp/sys.h"
#include "vm.h"
#include <string.h>
#include "common/circbuf.h"
#include "err.h"
#include <ctype.h>

static circbuf_t vm_opq; /*virtual machine task queue*/
static opcode_t vm_opcode; //current opcode
static opcode_t vm_opcode_nxt;
static opcode_t vm_opcode_seq; //contain line_stop of seq type
static opcode_t vm_opcode_scan; //contain line_stop of seq+scan type, for odata_seq use
static opcode_t *vm_odata;
static opcode_t *vm_odata_seq; //contain line_stop for seq type
static int vm_scan_arm;
static int vm_scan_cnt;

static struct {
	int grp_over : 1;
} vm_status;

void vm_init(void)
{
}

int vm_opgrp_is_over(void)
{
	int grp_is_over = 0;
	if(vm_status.grp_over) {
		if((vm_odata == NULL) && (vm_odata_seq == NULL)) {
			//odata has been fetched, over
			grp_is_over = 1;
		}
	}
	return grp_is_over;
}

static int vm_opq_is_not_empty(void)
{
	return buf_left(&vm_opq);
}

/*
1, vm_opq shoud be empty
2, opcode_nxt should be null
3, opgrp should be over
*/
int vm_is_opc(void)
{
	if(vm_opq_is_not_empty()) {
		return 0;
	}

	if(vm_opcode_nxt.special.type != VM_OPCODE_NUL) {
		return 0;
	}

	if(!vm_opgrp_is_over()) {
		return 0;
	}

	return 1;
}

int vm_prefetch(void)
{
	int ecode = 0;
	vm_opcode.value = vm_opcode_nxt.value;
	vm_opcode_nxt.special.type = VM_OPCODE_NUL;
	if(vm_opq_is_not_empty()) {
		buf_pop(&vm_opq, &vm_opcode_nxt.value, sizeof(vm_opcode_nxt));
	}

	if(vm_opcode_nxt.special.type == VM_OPCODE_SEQ) {
		/*exchange & fill opcode_seq, note:
		before: opcode = scan/open/clos + bus + linestart, opcode_nxt = seq + linestop
		after:   opcode = seq + linestart, opcode_seq = scan/open/clos + bus + linestop
		*/
		vm_opcode_seq.value = vm_opcode.value;
		vm_opcode_seq.line = vm_opcode_nxt.special.line;
		vm_opcode_nxt.line = vm_opcode.line;
		vm_opcode.value = vm_opcode_nxt.value;
	}

	switch(vm_opcode.special.type) {
	case VM_OPCODE_GRP:
	case VM_OPCODE_NUL:
		if(vm_opq_is_not_empty()) {
			vm_prefetch();
		}
		break;
	case VM_OPCODE_SEQ:
		vm_opcode_nxt.special.type = VM_OPCODE_NUL;
		if(vm_opq_is_not_empty()) {
			buf_pop(&vm_opq, &vm_opcode_nxt.value, sizeof(vm_opcode_nxt));
		}
		break;
	default:
		//serious error!!!!
		break;
	}
	return ecode;
}

/*
 1, normal open/close instruction, just let it pass through
 2, grp tag
 3, seq(loop) instruction
*/
int vm_execute(void)
{
	int prefetch = 1;

	vm_odata = NULL;
	vm_odata_seq = NULL;

	/*prefetch is needed? */
	if(vm_opcode.type != VM_OPCODE_SPECIAL) {
		vm_odata = &vm_opcode;
	}

	/*
	vm_opcode		seq type + line_start
	vm_opcode_seq	scan/open/clos type + bus + line_stop
	*/
	if(vm_opcode.special.type == VM_OPCODE_SEQ) {
		if(vm_opcode.line < vm_opcode_seq.line) { //update line_start
			if(vm_opcode_seq.type != VM_OPCODE_SCAN) {
				//give out two opcode, over
				vm_odata = &vm_opcode;
				vm_odata_seq = &vm_opcode_seq;
			}
			else {
				vm_opcode_scan.value = vm_opcode_seq.value;
				vm_opcode_scan.line = vm_opcode.line + vm_scan_arm - 1;

				//give out two opcode
				vm_odata = &vm_opcode;
				vm_odata_seq = &vm_opcode_scan;

				if(vm_opcode_scan.line < vm_opcode_seq.line) {
					prefetch = 0;
				}

				if(vm_opcode_scan.line > vm_opcode_seq.line) {
					//Serious error!!!
				}
			}
		}
	}

	/*group over?*/
	int grp_is_over = 0;
	if(vm_odata != NULL) {
		//identify current opcode type
		int type = vm_odata->type;
		if(vm_odata->special.type == VM_OPCODE_SEQ) {
			if(vm_odata_seq != NULL) {
				type = vm_odata_seq->type;
			}
			else {
				//Serious error!!!
			}
		}

		//calculate according to the different opcode type
		if((type == VM_OPCODE_OPEN) || (type == VM_OPCODE_CLOSE)) {
			if(vm_scan_cnt != 0) {
				//Serious error!!!
			}

			grp_is_over = (vm_opcode_nxt.special.type == VM_OPCODE_GRP) ? 1 : 0;
			grp_is_over = (vm_opcode_nxt.special.type == VM_OPCODE_NUL) ? 1 : grp_is_over;
		}
		else if(type == VM_OPCODE_SCAN) {
			//vm_scan_cnt process
			if(vm_odata->special.type == VM_OPCODE_SEQ) {
				vm_scan_cnt += vm_odata_seq->line - vm_odata->line + 1;
			}
			else {
				vm_scan_cnt ++;
			}

			//grp_is_over is determined by arm value & scan counter
			if(vm_scan_cnt == vm_scan_arm) {
				grp_is_over = 1;
				vm_scan_cnt = 0;
			}
		}
	}
	vm_status.grp_over = grp_is_over;

	if(prefetch) {
		vm_prefetch();
	}
	return 0;
}

int vm_fetch( //non-zero if error happened
	void *buf, //rx buffer,  normally = can_msg.data[8]
	int *bytes //input buf size, output bytes filled
)
{
	int n = 0;
	short *p = (short *) buf;
	for(n = 0; n < *bytes;) {
		if(!vm_opq_is_not_empty()){
			break;
		}

		if(vm_odata != NULL) {
			*p ++ = vm_odata->value;
			n += sizeof(short);
			vm_odata = NULL;
			continue;
		}
		if(vm_odata_seq != NULL) {
			*p ++ = vm_odata_seq->value;
			n += sizeof(short);
			vm_odata_seq = NULL;
			continue;
		}
		vm_execute();
	}

	*bytes = n;
	return 0;
}



int __vm_opq_add(int tcode, int bus, int line)
{
	opcode_t opcode;
	opcode.value = 0;
	switch(tcode) {
	case VM_OPCODE_GRP:
		opcode.special.type = tcode;
		break;
	case VM_OPCODE_SEQ:
		opcode.special.type = tcode;
		opcode.line = line;
		break;
	default:
		opcode.type = tcode;
		opcode.bus = bus;
		opcode.line = line;
	}

	int ecode = - IRT_E_OPQ_FULL;
	if(buf_left(&vm_opq) > sizeof(opcode)) {
		buf_push(&vm_opq, &opcode.value, sizeof(opcode));
		ecode = 0;
	}
	return ecode;
}

int _vm_opq_add(int tcode, const char *relay)
{
	int ecode = 0;

	//verify all 6 char followed are digit
	for(int i = 1; i < 7; i ++) {
		if(!isdigit(relay[i])) {
			ecode = - IRT_E_CMD_FORMAT;
			return ecode;
		}
	}

	//str to int
	int bus, line;
	sscanf(relay + 1, "%d", &bus);
	line = bus % 10000;
	bus /= 10000;

	switch(relay[0]) {
	case '@': //first one
		__vm_opq_add(VM_OPCODE_GRP, 0, 0);
		ecode = __vm_opq_add(tcode, bus, line);
		break;

	case ',': //list
		ecode = __vm_opq_add(tcode, bus, line);
		break;
	case ':': //seq
		ecode = __vm_opq_add(VM_OPCODE_SEQ, bus, line);
		break;
	default:
		ecode = - IRT_E_CMD_FORMAT;
	};
	return ecode;
}

//<relay_list> = (@bbllll,bbllll:bbllll,bbllll,bbllll:bbllll,...)
int vm_opq_add(int tcode, const char *relay_list)
{
	int ecode = 0;
	int n = strlen(relay_list);
	if((n < 2) || (relay_list[0] != '(') || (relay_list[n] != ')')) {
		ecode = - IRT_E_CMD_FORMAT;
		return ecode;
	}

	for(int i = 1; i + 7 < n; i += 7) {
		ecode = _vm_opq_add(tcode, relay_list + i);
		if(ecode) break;
	}
	return 0;
}
