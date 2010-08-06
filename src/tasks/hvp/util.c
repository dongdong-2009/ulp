/*
 * 	miaofng@2010 initial version
 *		this is service programming system interpreter !
 */

#include "config.h"
#include "util.h"
#include "common/kwp.h"
#include "common/ptp.h"
#include "common/print.h"
#include <stdio.h>
#include <stdlib.h>
#include "FreeRTOS.h"

#include "ff.h"

#define __DEBUG
//#define __DISABLE_PROG

static FIL util_file;
static ptp_t util_ptp;
static FIL prog_file;
static ptp_t prog_ptp;
static util_head_t util_head;
static util_inst_t *util_inst;
static int util_inst_nr;
static int util_routine_addr;
static short util_routine_size;
//var for interpreter parser
static int util_global_addr;
static int util_global_size;

//big endian to little endian
static int ntohl(int vb)
{
	int vl = 0;
	vl |= ((vb >> 0) & 0xff) << 24;
	vl |= ((vb >> 8) & 0xff) << 16;
	vl |= ((vb >> 16) & 0xff) << 8;
	vl |= ((vb >> 24) & 0xff) << 0;
	return vl;
}

static int ntohs(int vb)
{
	int vl = 0;
	vl |= ((vb >> 0) & 0xff) << 8;
	vl |= ((vb >> 8) & 0xff) << 0;
	return vl;
}

/*parse the utility bin file to a struct, ready for usage*/
int util_init(const char *util, const char *prog)
{
	int br, btr;
	int ret = 0;
	FRESULT res;

	res = f_open(&util_file, util, FA_OPEN_EXISTING | FA_READ);
	if(res != FR_OK) {
		return - UTIL_E_OPEN;
	}

	res = f_open(&prog_file, prog, FA_OPEN_EXISTING | FA_READ);
	if(res != FR_OK) {
		return - UTIL_E_OPEN;
	}
	
	util_ptp.fp = &util_file;
	util_ptp.read = (int (*)(void *, void *, int, int *)) f_read;
	util_ptp.seek = (int (*)(void *, int)) f_lseek;
	prog_ptp.fp = &prog_file;
	prog_ptp.read = (int (*)(void *, void *, int, int *)) f_read;
	prog_ptp.seek = (int (*)(void *, int)) f_lseek;
	
	if(ptp_init(&prog_ptp)) {
		return -1;
	}
	
	if(!ptp_init(&util_ptp)) {
		//read util head
		btr = sizeof(util_head_t);
		ptp_read(&util_ptp, &util_head, btr, &br);
		if(br != btr) {
			return - UTIL_E_RDHEAD;
		}
		
		//head endian convert
		util_head.cksum = ntohs(util_head.cksum);
		util_head.id = ntohs(util_head.id);
		util_head.sn = ntohl(util_head.sn);
		util_head.suffix = ntohs(util_head.suffix);
		util_head.htype = ntohs(util_head.htype);
		util_head.itype = ntohs(util_head.itype);
		util_head.offset = ntohs(util_head.offset);
		util_head.atype = ntohs(util_head.atype);
		util_head.addr = ntohl(util_head.addr);
		util_head.maxlen = ntohs(util_head.maxlen);
		
		//read all util instructions
		btr = util_head.offset - sizeof(util_head_t);
		util_inst_nr = btr / sizeof(util_inst_t);
		util_inst = MALLOC(btr);
		if(!util_inst) {
			return - UTIL_E_MEM;
		}
		ptp_read(&util_ptp, util_inst, btr, &br);
		if(br != btr) {
			return - UTIL_E_RDINST;
		}
		
		//read routine info
		ptp_read(&util_ptp, &util_routine_addr, 4, &br);
		ptp_read(&util_ptp, &util_routine_size, 2, &br);
		util_routine_addr = ntohl(util_routine_addr);
		util_routine_size = ntohs(util_routine_size);
		util_head.offset += 6;
		
		//success
#ifdef __DEBUG
		print("util_init() success\n");
#endif
	}
		
	return ret;
}

/* note: ptp file pointer must be at routine start, or ptp_seek op is needed
option = 0x00 use addr
option = 0x01 do not use addr
option = 0x40 refer option 0, plus send ReqDnld and ReqTransExit each time
option = 0x41 refer option 1, plus send ReqDnld and ReqTransExit each time
*/
static int util_dnld(int id, int option)
{
	int err;
	int addr, alen;
	short size;
	int btr, br;
	char buf[UTIL_PACKET_SZ];
	
	addr = util_routine_addr;
	size = util_routine_size;

#ifdef __DEBUG
	print("util info: addr = 0x%06x, size = %06x\n", addr, size);
#endif
	
	br = btr = 0;
	alen = (option & 0x01) ? 0 : util_head.atype;
	while(size > 0) {
		btr = UTIL_PACKET_SZ;
		btr = (size < btr) ? size : btr;
		
		//ReqDnld
		if(option & 0xf0) {
			if(btr && br) {//&&br to ignore first time 
				err = kwp_RequestToDnload(0, addr, btr, 0);
				if(err)
					break;
			}
		}
				
		//download
		ptp_read(&util_ptp, buf, btr, &br);
		if(br == btr) {
			err = kwp_TransferData(addr, alen, br, buf);
			addr += br;
			size -= br;
			if(err)
				break;
		}
		else {
			//???file system err :(
		}
			
		//ReqTransExit
		if(option & 0xf0) {
			err = kwp_RequestTransferExit();
			if(err)
				break;
		}
	}
	
	return err;	
}

/* program product software to target
option = 0x00 use header addr, but keep it constant during transfer
option = 0x01 use global addr, but keep it constant during transfer
option = 0x02 not use addr
option = 0x03 not use addr,  but include data packet cksum(8bit sum of all data) and len byte in front

option = 0x80 use header addr and inc(4byte)
option = 0x81 use global addr and inc(4byte)

option = 0x40 refer option 0, plus send ReqDnld and ReqTransExit each time
option = 0x41 refer option 1, plus send ReqDnld and ReqTransExit each time
option = 0x42 refer option 2, plus send ReqDnld and ReqTransExit each time
option = 0x43 refer option 3, plus send ReqDnld and ReqTransExit each time

option = 0xC0 refer option 0x80, plus send ReqDnld and ReqTransExit each time
option = 0xC1 refer option 0x81, plus send ReqDnld and ReqTransExit each time
 
			addr type ( h-> header addr, g-> global addr, n -> not use addr, c -> cksum instead of addr, + -> addr inc)
0x00/0x40		h
0x01/0x41		g
0x02/0x42		n
0x03/0x43		nc
0x80/0xc0		h+
0x81/0xc1		g+
*/
static int util_prog(int id, int option)
{
	int err;
	int addr, alen;
	short size;
	int btr, br;
	char buf[UTIL_PACKET_SZ];
	
	addr = (option & 0x01) ? util_global_addr : util_head.addr;
	size = 0;
	
	alen = (option & 0x02) ? 0 : util_head.atype;
	while(1) {
		btr = UTIL_PACKET_SZ;
		ptp_read(&prog_ptp, buf, btr, &br);
		if(!br)
			break;
		
		size += br; //Statistics info
		
		//ReqDnld
		if(option & 0x40) {
			err = kwp_RequestToDnload(0, addr, br, 0);
			if(err)
				break;
		}
				
		//download
		err = kwp_TransferData(addr, alen, br, buf);
		if(err)
			break;
		
		//addr inc
		if(option & 0x80)
			addr += br;
		
		//ReqTransExit
		if(option & 0x40) {
			err = kwp_RequestTransferExit();
			if(err)
				break;
		}
	}
	
#ifdef __DEBUG
	print("prog info: addr = 0x%06x, size = %06x\n", addr, size);
#endif

	return err;	
}

/*
	code :=
		1, timeout -> 0xfd
		2, negative response -> response code, byte 3 in data field of rx frame
		3, positive response srv id

	sp := special mode, 1 indicates special handling
	
	return: 
		step next, or 0-> fail
	
	note: 
	1, if negative response code == positive response code + 0x40, special handling as following:
	-> look for second occurrence of response code in the goto field
	2, if not found, 0xff is used 
*/
static char util_jump(util_inst_t *p, char code, char sp)
{
	char next = 0;
	
	if(p->jt[0].code == code) {
		next = p->jt[0].step;
	}
	else if(p->jt[1].code == code) {
		next = p->jt[1].step;
	}	
	else if(p->jt[2].code == code) {
		next = p->jt[2].step;
	}
	else if(p->jt[3].code == code) {
		next = p->jt[3].step;
	}
	else if(p->jt[4].code == code) {
		next = p->jt[4].step;
	}
	
	if(next && sp) { //search 2nd time
		next = util_jump(p, code, 0);
	}
	
	if(!next && code != 0xff) { //not found -> 0xff
		next = util_jump(p, 0xff, 0);
	}
	
	return next;
}

/*return the step next*/
static int util_execute(util_inst_t *p)
{
	char code, sp, step;
	int v, err = 0;
	
	//success
	code = p->sid + 0x40;
	sp = 0;
	
	switch (p->sid) {
		case SID_01:
			kwp_SetAddr(p->ac[0], p->ac[1]);
			code = 0xff;
			break;
		case SID_11:
			err = kwp_EcuReset(p->ac[0]);
			break;
		case SID_81:
			err = kwp_EstablishComm();
			break;
		case SID_27:
			v = 1 + p->ac[3] + p->ac[3];
			err = kwp_SecurityAccessRequest((char) v, 0, &v);
			if(!err && v == 0) {
				code = 0x34;
				break;
			}
			//note: other situation are not supported yet!!!
			break;
		case SID_83:
			err = kwp_AccessCommPara();
			break;
		case SID_10:
			err = kwp_StartDiag(p->ac[0], p->ac[1]);
			break;
		case SID_34:
			if(p->ac[3] == 0x01) {
				err = kwp_RequestToDnload(p->ac[2], util_global_addr, util_global_size, 0);
			}
			else {
				err = kwp_RequestToDnload(0, 0, 0, 0);
			}
			break;
		case SID_90: //dnload routine
			err = util_dnld(p->ac[0], p->ac[3]);
			if(!err)
				code = SID_36 + 0x40;
			break;
		case SID_93: //dnload software
#ifndef __DISABLE_PROG
			err = util_prog(p->ac[0], p->ac[3]);
#endif
			if(!err)
				code = SID_36 + 0x40;			
			break;
		case SID_37:
			err = kwp_RequestTransferExit();
			break;
		case SID_38: //??? routine para are not supported yet
			v = (p->ac[3]) ? util_global_addr : util_routine_addr;
			err = kwp_StartRoutineByAddr(v);
			break;
		case SID_31:
#ifndef __DISABLE_PROG
			err = kwp_StartRoutineByLocalId(p->ac[0], 0, 0);
#endif
			break;
		case 0xf1: //set global mem addr for data download
		case 0xf2: //set global length for data download
			v = p->ac[3];
			v = (v << 8) + p->ac[0];
			v = (v << 8) + p->ac[1];
			v = (v << 8) + p->ac[2];
			util_global_addr = (p->sid == 0xf1) ? v : util_global_addr;
			util_global_size = (p->sid == 0xf2) ? v : util_global_size;
			code = 0; //???
			break;
		default: //not supported
			return -1;
	}
	
	if(err) {
		kwp_GetLastErr(0, 0, &code);
		sp = (code == p->sid + 0x40 + 0x40);
	}
	
	step = util_jump(p, code, sp);
	return step;
}

int util_interpret(void)
{
	int step = 1;
	util_inst_t *p;
	
#ifdef __DEBUG
	print("utility interpreter start\n");
	for(int i = 0; i < util_inst_nr; i ++) {
		p = util_inst + i;
		print("%02d: SR_%02X(%02x, %02x, %02x, %02x), [%02x->%02d, %02x->%02d, %02x->%02d, %02x->%02d, %02x->%02d]\n", \
			p->step, p->sid, \
			p->ac[0], p->ac[1], p->ac[2], p->ac[3], \
			p->jt[0].code, p->jt[0].step, \
			p->jt[1].code, p->jt[1].step, \
			p->jt[2].code, p->jt[2].step, \
			p->jt[3].code, p->jt[3].step, \
			p->jt[4].code, p->jt[4].step \
		);
	}
#endif

	while(step > 0 && step < util_inst_nr) {
		//find step
		p = util_inst + step - 1;
		if(p->step != step) {
			//unfortunately, some steps are not in order :(
			for(int i = 0; i < util_inst_nr; i ++) {
				p = util_inst + i;
				if(p->step == step)
					break;
			}
		}
		
#ifdef __DEBUG
		print("%02d: SR_%02X(%02x, %02x, %02x, %02x), [%02x->%02d, %02x->%02d, %02x->%02d, %02x->%02d, %02x->%02d]\n", \
			p->step, p->sid, \
			p->ac[0], p->ac[1], p->ac[2], p->ac[3], \
			p->jt[0].code, p->jt[0].step, \
			p->jt[1].code, p->jt[1].step, \
			p->jt[2].code, p->jt[2].step, \
			p->jt[3].code, p->jt[3].step, \
			p->jt[4].code, p->jt[4].step \
		);
#endif
		step = util_execute(p);
	}

#ifdef __DEBUG
	print("utility interpreter end\n");
#endif

	return step;
}

void util_close(void)
{
	ptp_close(&util_ptp);
	f_close(util_ptp.fp);
	ptp_close(&prog_ptp);
	f_close(prog_ptp.fp);
	FREE(util_inst);
}

