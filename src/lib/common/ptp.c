/*
 * 	miaofng@2010 initial version
 *		This file is used to decode and convert motorolar S-record to binary
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "FreeRTOS.h"
#include "common/print.h"
#include "common/ptp.h"

#ifndef __DEBUG
#define __DEBUG
#endif

//convert hex string to integer
static int htoi(char *buf)
{
	int i, v = 0;
	char c;

	for(i = 0; (c = buf[i]) != 0; i ++) {
		c = (c >= 'a') ? (c - 'a' + 10) : c;
		c = (c >= 'A') ? (c - 'A' + 10) : c;
		c = (c >= '0') ? (c - '0') : c;
		
		v <<= 4;
		v += c;
	}

	return v;
}

/*
para:
	op = 0, default, convert all infomation
	op = 1, convert size info only
note:
	file pointer inside of ptp, will point to next s-record
*/
static int ptp_parse(ptp_t *ptp, int op)
{
	ptp_record_t *s = &ptp->priv->s;
	int br, i, len, val;
	char buf[3];
	char t, cksum = 0;
	
	//init
	buf[2] = 0;
	s->addr = 0;
	s->len = 0;
	s->ofs = 0;
	
	//get 'S'
	ptp->read(ptp->fp, buf, 1, &br);
	if(br != 1 || buf[0] != 'S') //EOF?
		return -1;

	//get record type
	ptp->read(ptp->fp, &t, 1, &br);
	
	//get len
	ptp->read(ptp->fp, buf, 2, &br);
	val = htoi(buf); 
	s->len = val - 1; // - cksum byte
	cksum += val;
	
	//get addr
	switch (t) {
		case '0':
		case '5':
		case '9':
		case '1': //2 byte addr
			len = 2;
			break;
		case '8':
		case '2': //3 byte addr
			len = 3;
			break;
		case '7':
		case '3': //4 byte addr
			len = 4;
			break;
		default:
			s->len = 0;
			return 0;
	}
	s->len -= len;
	for(i = 0; i < len; i ++) {
		ptp->read(ptp->fp, buf, 2, &br);
		val = htoi(buf);
		s->addr <<= 8;
		s->addr += val;
		cksum += val;
	}
	
	//get bin
	for(i = 0; i < s->len; i ++) {
		ptp->read(ptp->fp, buf, 2, &br);
		if(op == 0) { //convert to bin
			val = htoi(buf);
			s->bin[i] = (char) val;
			cksum += val;
		}
	}
	
	//get cksum
	ptp->read(ptp->fp, buf, 2, &br);
	if(op == 0) { //verify cksum
		val = htoi(buf);
		cksum += val;
		cksum += 1; //ones complement of len + addr + data
		if(cksum & 0xff) {
#ifdef __DEBUG
			print("warning: cksum err!!!\n");
#else
			return -1;
#endif
		}
	}
	
	//'CR' 'LF'?
	ptp->read(ptp->fp, buf, 1, &br);
	if(buf[0] == '\r')
		ptp->read(ptp->fp, buf, 1, &br);
	
	//only s1 s2 s3 type record is supported
	if(t != '1' && t != '2' && t != '3')
		s->len = 0;

	return 0;
}

int ptp_init(ptp_t *ptp)
{
	ptp->priv = MALLOC(sizeof(ptp_priv_t));
	return 0;
}

int ptp_read(ptp_t *ptp, void *buff, int btr, int *br)
{
	ptp_record_t *s = &ptp->priv->s;
	int n;
	
	*br = 0;
	while(btr > 0) {
		//fetch bin from current record
		n = s->len - s->ofs;
		if(n > 0) {
			n = (n > btr) ? btr : n;
			memcpy((char*)buff + (*br), s->bin + s->ofs, n);
			s->ofs += n;
			btr -= n;
			*br += n;
		}
		else {
			//try to fetch a new record
			if(ptp_parse(ptp, 0))
				break;
		}
	}
	
	return 0;
}

int ptp_seek(ptp_t *ptp, int ofs)
{
	ptp_record_t *s = &ptp->priv->s;
	
	ptp->seek(ptp->fp, 0);
	while(ofs > 0) {
		if(ptp_parse(ptp, 1)) {
			//EOF?
			break;
		}
		
		if(ofs <= s->len) {
			//ok
			s->ofs = ofs;
			ofs = 0;
			break;
		}
		
		ofs -= s->len;
	}
	return (ofs == 0) ? 0 : -1;
}

int ptp_close(ptp_t *ptp)
{
	FREE(ptp->priv);
	return 0;
}

//misc op
int ptp_size(ptp_t *ptp)
{
	ptp_record_t *s = &ptp->priv->s;
	int len = 0;
	
	ptp->seek(ptp->fp, 0);
	while(!ptp_parse(ptp, 1)) {
		len += s->len;
	}
	
	ptp->seek(ptp->fp, 0);
	return len;
}
