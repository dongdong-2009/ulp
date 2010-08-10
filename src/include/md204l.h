/*
 * File Name : md204l.h
 * Author    : dusk
 */

#ifndef __MD204L_H
#define __MD204L_H

#include "stm32f10x.h"

#define CMD_MD204L_READ		0x52
#define CMD_MD204L_WRITE	0x57

#define MD204L_READ_ADDR	0x01
#define MD204L_READ_LEN		0x05

enum {
MD204L_OK = 0,
MD204L_ADDR_WRONG,
MD204L_LEG_WRONG,
MD204L_OUT_RANGE,
MD204L_CMD_WRONG
};

typedef struct {
unsigned char station;
unsigned char cmd;
unsigned char addr;
unsigned char len;
//short *data;
unsigned char cksum;
}md204l_req_t;

typedef struct {
unsigned char station;
unsigned char status;
unsigned char addr;
unsigned char len;
//short *data;
unsigned char cksum;
}md204l_ack_t;

extern short md204l_read[MD204L_READ_LEN];

void md204l_Init(void);
int md204l_Write(int addr, short *pbuf, int count);
int md204l_Read(int addr, short *pbuf, int count);
void md204l_Update(void);

#endif /* __MD204L_H */
