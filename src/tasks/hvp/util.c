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

static FIL util_file;
static ptp_t util_ptp;
static util_head_t util_head;
static util_inst_t *util_inst;
static int util_inst_nr;
static int util_seek;
static int util_routine_addr;
static short util_routine_size;

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
int util_init(const char *util, const char *ptp)
{
	int br, btr;
	int ret = 0;
	FRESULT res;
	
	util_seek = 0;
	
	res = f_open(&util_file, util, FA_OPEN_EXISTING | FA_READ);
	if(res != FR_OK) {
		return - UTIL_E_OPEN;
	}

	util_ptp.fp = &util_file;
	util_ptp.read = (int (*)(void *, void *, int, int *)) f_read;
	util_ptp.seek = (int (*)(void *, int)) f_lseek;
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
		util_seek = util_head.offset;
		
		//success
#ifdef __DEBUG
		print("util_init() success\n");
#endif
	}
		
	return ret;
}

int util_read(char *buf, int btr, int *br)
{
	int ret = 0;

	if(util_seek != util_head.offset) {
		util_seek = util_head.offset;
		if(ptp_seek(&util_ptp, util_seek)) {
			return - UTIL_E_SEEK;
		}
	}

	if(ptp_read(&util_ptp, buf, btr, br)) {
		return - UTIL_E_RDROUTINE;
	}
	
	return ret;
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
static char util_execute(util_inst_t *p)
{
	char code, sp, step;
	
	//success
	code = p->sid + 0x40;
	sp = 0;
	
	switch (p->sid) {
		case SID_01:
			kwp_SetAddr(p->ac[0], p->ac[1]);
			code = 0xff;
			break;
		case SID_81:
			if(kwp_EstablishComm()) {
				kwp_GetLastErr(0, 0, &code);
				sp = (code == p->sid + 0x40 + 0x40);
			}
			break;
		default: //not supported
			return 0xff;
	}
	
	step = util_jump(p, code, sp);
	return step;
}

int util_interpret(void)
{
	char step = 1;
	
#ifdef __DEBUG
	print("utility interpreter start\n");
#endif

	while(step > 0 && step < util_inst_nr) {
#ifdef __DEBUG
		util_inst_t *p = util_inst + step - 1;
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
	FREE(util_inst);
}

/* get routine size
*/
int util_size(void)
{
	return util_routine_size;
}

/* get routine addr
*/
int util_addr(void)
{
	return util_routine_addr;
}

