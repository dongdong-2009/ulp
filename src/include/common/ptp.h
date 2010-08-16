/*
 * 	miaofng@2010 initial version
 *		This file is used to decode and convert motorolar S-record to binary
 */
#ifndef __PTP_H_
#define __PTP_H_

/* s record format:
	type(char[2]) count(char[2], bytes left until the tail, except 'LR') address(char[4,6 or 8]) data cksum(char [2]) 'LR'
	S0:= type(0X5330) + count + addr(0x0000) +  data(name(char[20] ) + ver(char[2]) + rev(char[2])  +  description(char[0-36])) + cksum
	S1:= type(0x5331) + count + addr(char[2]) + data + cksum
	S2:= type(0x5332) + count + addr(char[3]) + data + cksum
	S3:= type(0x5333) + count + addr(char[4]) + data + cksum
	
	S5:= type(0x5335) + count + addr(char[2], cnt of S1/S2/S3 previously transmitted) + data(none) + cksum
	
	S7:= type(0x5337) + count + addr(char[4], starting execution address) + data(none) + cksum
	S8:= type(0x5338) + count + addr(char[3], starting execution address) + data(none) + cksum
	S9:= type(0x5339) + count + addr(char[2], starting execution address) + data(none) + cksum
*/
#define PTP_LINE_BYTES_MAX	40

typedef struct {
	int addr;
	char len;
	char ofs; //current record 
	char bin[PTP_LINE_BYTES_MAX];
} ptp_record_t;

typedef struct {
	ptp_record_t s;
} ptp_priv_t;

typedef struct {
	/*filled by caller*/
	void *fp; //file pointer, return by fopen
	int (*read)(void *fp, void *buff, int btr, int *br);
	int (*seek)(void *fp, int ofs);
	
	/*private*/
	ptp_priv_t *priv;
} ptp_t;

int ptp_init(ptp_t *ptp); //init ptp_priv
int ptp_read(ptp_t *ptp, void *buff, int btr, int *br);
int ptp_seek(ptp_t *ptp, int ofs);
int ptp_close(ptp_t *ptp); //release mem space occupied by *priv

//misc op
int ptp_size(ptp_t *ptp); //get binary size


#endif /*__PTP_H_*/
