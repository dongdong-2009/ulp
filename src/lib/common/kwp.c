/*
 * 	miaofng@2010 initial version
 *		This file is used to provide data link level i/f for ISO14230/keyword2000 protocol
 */

#include "config.h"
#include "common/kwp.h"
#include "kwd.h"
#include "time.h"
#include <stdlib.h>

#define __DEBUG
#ifdef __DEBUG
#include "shell/cmd.h"
#include <stdio.h>
#include <string.h>
#endif

static time_t kwp_timer;
static char kwp_step; //transfer step
static char kwp_wake; //wake step
static char kwp_frame[260];
static kwp_err_t kwp_err;
static char *pbuf;

int kwp_Init(void)
{
	kwp_wake = 0;
	kwp_step = 0;
	
	kwd_init();
	return 0;
}

int kwp_IsReady(void)
{
	int bytes, len;
	int ready = 1;
	
	if(kwp_wake)
		ready = time_left(kwp_timer) < 0;
	
	if(kwp_step) {
		ready = 0;
		bytes = kwd_poll(1);
#ifdef __DEBUG
		printf("\nkwp rx:");
		for(int i = 0; i < bytes; i ++) {
			printf(" %02x", kwp_frame[i]);
		}
#endif
		if(bytes > 1) {
			len = kwp_frame[0] & 0x3f;
			len += 3 + 1; /*head 3 bytes + cksum (1 byte)*/
			ready = (bytes >= len);
		}
	}
	
	return ready;
}

int kwp_EstablishComm(void)
{
	//fast init is the only wakeup protocol supported now
	if(kwp_wake == 0) {
		kwp_timer = time_get(KWP_FAST_INIT_MS);
		kwd_wake(0);
		kwp_wake = 1;
	}
	else if(kwp_wake == 1) {
		kwp_timer = time_get(KWP_FAST_INIT_MS);
		kwd_wake(1);
		kwp_wake = 2;
	}
	else {
		kwp_wake = 0;
	}
	
	return kwp_wake;
}

void *kwp_malloc(int n)
{
	kwp_frame[0] = 0x80;
	kwp_frame[1] = KWP_DEVICE_ID;
	kwp_frame[2] = KWP_TESTER_ID;
	return &kwp_frame[3];
}

void kwp_free(void *p)
{
	//nothing needs to be done here
}

static char kwp_cksum(const char *p, int n)
{
	int i;
	char cksum = 0;
	
	for(i = 0; i < n; i++ ) {
		cksum += *p;
		p ++;
	}

	return (cksum);
}

int kwp_transfer(char *tbuf, int tn, char *rbuf, int rn)
{
	int n = 3 + tn;
	kwp_frame[0] += tn;
	kwp_frame[n] = kwp_cksum(kwp_frame, n);
#ifdef __DEBUG
	printf("\nkwp tx:");
	for(int i = 0; i < n + 1; i ++) {
		printf(" %02x", kwp_frame[i]);
	}
#endif
	return kwd_transfer(kwp_frame, tn + 4, kwp_frame, rn + 4);
}

int kwp_check(void)
{
	int err;
	
	/*success?*/
	err = 0;
	kwp_err.rid = pbuf[0];
	kwp_err.sid = 0;
	kwp_err.code = 0;
	
	/*fail?*/
	if(pbuf[0] == SID_ERR) {
		err = -1;
		kwp_err.sid = pbuf[1];
		kwp_err.code = pbuf[2];
		free(pbuf);
		kwp_step = 0;
	}
	
	return err;
}

int kwp_GetLastErr(char *rid, char *sid, char *code)
{
	if(rid)
		*rid = kwp_err.rid;
	if(sid)
		*sid = kwp_err.sid;
	if(code)
		*code = kwp_err.code;
		
	return 0;
}

/*
The 81 Op-Code is used to start communications with an ECU.  Communications will remain established while 
activity exists on the link.  If the link is inactive for five seconds the ECU will revert back to a waiting for 
communications state.  There are no negative responses allowed for the Start Communication request.
*/
int kwp_StartComm(char *kb0, char *kb1)
{	
	if(kwp_step == 0) {
		pbuf = kwp_malloc(2);
		pbuf[0] = SID_81;
		
		kwp_transfer(pbuf, 1, pbuf, 3);
		kwp_step = 1;
	}
	else {
		if(kwp_check())
			return -1;

		if(kb0)
			*kb0 = pbuf[1];
		if(kb1)
			*kb1 = pbuf[2];

		kwp_step = 0;
		free(pbuf);
	}
	
	return kwp_step;
}

/*
The 82 Op-Code is used to stop communications with an ECU.  There is no negative response allowed for the Stop 
Communication request.
*/
int kwp_StopComm(void)
{
	if(kwp_step == 0) {
		pbuf = kwp_malloc(3);
		pbuf[0] = SID_82;

		kwp_transfer(pbuf, 1, pbuf, 3);
		kwp_step = 1;
	}
	else {
		if(kwp_check())
			return -1;
		
		kwp_free(pbuf);
		kwp_step = 0;
	}
	
	return kwp_step;
}

/*
The 83 Op-Code is used to access the communications parameters.  This Op-Code is issued after a security request (
Op-Code 27) has passed.  This Op-Code is performed in two steps first, the optimum timing parameters for use during 
a reprogramming event are read from the ECU.  Second,  the optimum timing parameters are put into a service request 
83 and sent to the ECU. 
*/
int kwp_AccessCommPara(void)
{
	if(kwp_step == 0) {
		pbuf = kwp_malloc(7);
		pbuf[0] = SID_83;
		pbuf[1] = 0;
	
		kwp_transfer(pbuf, 2, pbuf, 7);
		kwp_step = 1;
	}
	else if(kwp_step == 1) {
		if(kwp_check())
			return -1;

		pbuf[0] = SID_83;
		pbuf[1] = 0x03;
		/*pbuf[2..6] is the val received*/
		
		kwp_transfer(pbuf, 7, pbuf, 3);
		kwp_step = 2;
	}
	else if(kwp_step == 2) {
		if(kwp_check())
			return -1;
		
		free(pbuf);
		kwp_step = 0;
	}
	
	return kwp_step;
}

/*
The 10 Op-Code is used to inform devices on the serial data link that the tool is ready to start diagnostic procedures
 using the link and the devices on the link may need to alter their normal operation. 
 para baud: $00->invalid, $01->9600, $02->19200, $03->38400, $04->57600, $05->115200
*/
int kwp_StartDiag(char mode, char baud)
{
	int n;
	n = (baud > 0) ? 3 : 2;
	if(kwp_step == 0) {
		pbuf = kwp_malloc(3);
		pbuf[0] = SID_10;
		pbuf[1] = mode;
		pbuf[2] = baud;
		
		kwp_transfer(pbuf, n, pbuf, 3);
		kwp_step = 1;
	}
	else {
		if(kwp_check())
			return -1;
		
		free(pbuf);
		kwp_step = 0;
	}
	
	return kwp_step;
}

#ifdef __DEBUG
static char htoc(char *buf)
{
	char h, l;

	h = buf[0];
	h = (h >= 'a') ? (h - 'a' + 10) : h;
	h = (h >= 'A') ? (h - 'A' + 10) : h;
	h = (h >= '0') ? (h - '0') : h;

	l = buf[1];
	l = (l >= 'a') ? (l - 'a' + 10) : l;
	l = (l >= 'A') ? (l - 'A' + 10) : l;
	l = (l >= '0') ? (l - '0') : l;

	return (h << 4) | l;
}

/*
this routine is only for debug purpose
*/
int kwp_debug(int argc, char *argv[])
{
	int n, i;
	n = argc - 1;
	if(kwp_step == 0) {
		pbuf = kwp_malloc(n);
		for(i = 0; i < n; i ++)
			pbuf[i] = htoc(argv[i + 1]);
		
		kwp_transfer(pbuf, n, pbuf, 250);
		kwp_step = 1;
	}
	else {
		if(kwp_check())
			return -1;
		
		free(pbuf);
		kwp_step = 0;
	}
	
	return kwp_step;
}

enum {
	CMD_NONE,
	CMD_WAKE,
	CMD_START,
	CMD_TRANS,
};

static char cmd;
static int cmd_kwp_func(int argc, char *argv[])
{
	int repeat;
	const char usage[] = { \
		" usage:\n" \
		" kwp start\n"
		" kwp d0 d1 d2 ... dn	d0..n is only the data field\n" \
	};

	if(argc == 1) {
		printf(usage);
		return 0;
	}

	if(argc > 1) {
		if(!strcmp(argv[1], "start")) { //kwp start?
			kwp_Init();
			kwp_EstablishComm();
			cmd = CMD_WAKE;
			return 1;
		}
		else {
			kwp_debug(argc, argv);
			cmd = CMD_TRANS;
			return 1;
		}
	}

	//update
	if(kwp_IsReady()) {
		if(cmd == CMD_WAKE) {
			repeat = kwp_EstablishComm();
			if(repeat == 0) {
				printf("kwp bus has been waked up!!!\n");
				cmd = CMD_START;
				return 1;
			}
		}
		
		if(cmd == CMD_START) {
			repeat = kwp_StartComm(0, 0);
			if(repeat == 0) {
				printf("\nkwp start comm sucessfully!!!\n");
				cmd = CMD_NONE;
				return 0;
			}
		}
		
		if(cmd == CMD_TRANS) {
			repeat = kwp_debug(argc, argv);
			if(repeat == 0) {
				printf("\nkwp transfer finished!!!\n");
				cmd = CMD_NONE;
				return 0;
			}
		}
	}
	
	return 1;
}

const cmd_t cmd_kwp = {"kwp", cmd_kwp_func, "kwp tx/rx commands"};
DECLARE_SHELL_CMD(cmd_kwp)
#endif
