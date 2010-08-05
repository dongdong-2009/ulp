/*
 * 	miaofng@2010 initial version
 */
#ifndef __UTIL_H_
#define __UTIL_H_

#define UTIL_PACKET_SZ 64

enum {
	UTIL_E_MEM = 1,
	UTIL_E_OPEN,
	UTIL_E_RDHEAD,
	UTIL_E_RDINST,
	UTIL_E_SEEK,
	UTIL_E_RDROUTINE,
	UTIL_E_UNDEF,
};

//interpreter type def
enum {
	UTIL_ITYPE_UART = 0,
	UTIL_ITYPE_CLASS2,
	UTIL_ITYPE_KWP,
	UTIL_ITYPE_GMLAN,
};

//addressing type def
enum {
	UTIL_ATYPE_2BYTE = 2,
	UTIL_ATYPE_3BYTE,
	UTIL_ATYPE_4BYTE,
};

typedef __packed struct {
	short cksum;
	short id; //module id, should be 0x0000
	int sn; //part nr, reserved
	short suffix; //design level suffix, reserved
	short htype; //header type, 0x0000
	short itype; //interpreter type
	short offset; //routine section offset
	short atype; //addressing type
	int addr; //data addr info
	short maxlen; //n data bytes in a message 
} util_head_t;

//utility instruction def, 16bytes
typedef __packed struct {
	char step;
	char sid;
	char ac[4]; //action field, used as para or indicating exception
	struct {
		char code; //0xfd, no comm; 0xff any; ...
		char step;
	} jt[5]; //goto fields, jump table
} util_inst_t;

int util_init(const char *util, const char *prog);
int util_interpret(void); //download file through util algo
void util_close(void);
#endif

