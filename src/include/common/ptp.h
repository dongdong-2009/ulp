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

#endif /*__PTP_H_*/
