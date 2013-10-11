/*
 * miaofng@2013-9-30 initial version
 */

#include "config.h"
#include "common/mb.h"
#include "ulp/sys.h"
#include <string.h>

static uart_bus_t *mb_bus;
static char mb_1p5T_ms; /*inside frame delay must be less than 1.5T*/
static char mb_3p5T_ms; /*interframe delay must be bigger than 3.5T*/
static unsigned char mb_device;

static time_t mb_timer;
static char mb_frame[MB_FRAME_BYTES];
static int mb_index;

/*refer to mbcrc.c*/
extern unsigned short usMBCRC16(char *pucFrame, int usLen);

#define ntohsf htonsf
static void htonsf(char *p, int nregs)
{
	nregs <<= 1;
	for(int i = 0; i < nregs; i += 2) {
		char c = p[i];
		p[i] = p[i + 1];
		p[i + 1] = c;
	}
}

#define ntohs htons
static unsigned short htons(unsigned short v)
{
	htonsf((char *)&v, 1);
	return v;
}

static int mb_pack_and_send(char *frame, int bytes)
{
	unsigned short v = usMBCRC16(frame, bytes);
	memcpy(frame + bytes, &v, 2);
	bytes += 2;

	int ms = time_left(mb_timer);
	ms += (mb_3p5T_ms - mb_1p5T_ms);
	sys_mdelay(ms);
	for(int i = 0; i < bytes; i ++) mb_bus->putchar(frame[i]);
	mb_timer = time_get(mb_1p5T_ms);
        return 0;
}

int mb_init(uart_bus_t *bus, mb_mode_t mode, int addr, int baud)
{
	uart_cfg_t cfg = {.baud = baud};
	mb_bus = bus;
	int ecode = mb_bus->init(&cfg);
	mb_device = (char) addr;

	mb_1p5T_ms = (char) ((1.5 * 10 * 1000) / baud);
	mb_1p5T_ms += 3; //at least 3mS
	mb_3p5T_ms = (char)(mb_1p5T_ms * (3.5/1.5));
	mb_timer = time_get(0);

#if 0
	mb_frame[0] = 0;
	mb_frame[1] = 1;
	mb_frame[2] = 2;
	mb_frame[3] = 0;
	mb_frame[4] = 0;
	unsigned short v = usMBCRC16(mb_frame, 3);
	memcpy(mb_frame + 3, &v, 2);
	v = usMBCRC16(mb_frame, 5);
	memcpy(mb_frame + 5, &v, 2);
#endif
	return ecode;
}

void mb_process(char *frame, int n)
{
	if(usMBCRC16(frame, n) != 0) { //it's not a modbus frame
		mb_eframe_cb(frame, n);
		return;
	}

	struct mb_rsp_s *rsp = (struct mb_rsp_s *) frame;
	if(rsp->device != mb_device) {
		return;
	}

	int ecode = 0;
	char *payload;

	struct mb_req_s req;
	memcpy(&req, frame, sizeof(req));
	req.addr = ntohs(req.addr);
	req.nregs = ntohs(req.nregs);

	switch(req.func) {
	case MB_FUNC_READ_HOLDING_REGISTER: /*0x03*/
		payload = frame + sizeof(struct mb_rsp_s);
		ecode = mb_hreg_cb(req.addr, payload, req.nregs, 1);
		if(!ecode) {
			htonsf(payload, req.nregs);
			rsp->bytes = req.nregs << 1;
			mb_pack_and_send(frame, sizeof(struct mb_rsp_s) + rsp->bytes);
		}
		break;

	case MB_FUNC_WRITE_REGISTER: /*0x06*/
		ecode = mb_hreg_cb(req.addr, (char *)&req.value, 1, 0);
		if(!ecode) {
			//echo back the same packet head
			mb_pack_and_send(frame, sizeof(struct mb_req_s));
		}
		break;

	case MB_FUNC_WRITE_MULTIPLE_REGISTERS: /*0x10*/
		payload = frame + sizeof(struct mb_req_s) + 1; /*redundant??? nregs & length byte*/
		ntohsf(payload, req.nregs);
		ecode = mb_hreg_cb(req.addr, payload, req.nregs, 0);
		if(!ecode) {
			//echo back the same packet head
			mb_pack_and_send(frame, sizeof(struct mb_req_s));
		}
		break;

	default: //unsupported function???
		ecode = MB_E_ILLEGAL_FUNCTION;
		break;
	}

	if(ecode) {
		rsp->func += 0x80;
		rsp->ecode = ecode;
		mb_pack_and_send(frame, sizeof(struct mb_rsp_s));
	}
}

int mb_update(void)
{
	while(mb_bus->poll()) {
		if(mb_index < MB_FRAME_BYTES) {
			mb_frame[mb_index] = mb_bus->getchar();
			mb_index ++;
		}
		mb_timer = time_get(mb_1p5T_ms);
	}

	if(time_left(mb_timer) > 0)
		return 0;

	if(mb_index > 0) {
		//time is over ... process the frame now
		mb_process(mb_frame, mb_index);
		mb_index = 0;
	}
	return 0;
}
