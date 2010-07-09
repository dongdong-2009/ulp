/*
 * 	miaofng@2010 initial version
 *		This file is used to provide data link level i/f for ISO14230/keyword2000 protocol
 */
#ifndef __KWP_H_
#define __KWP_H_

#define KWP_BAUD 10400

/*kwp frame
byte format: header field(1-4 bytes) + data field(1~255 bytes) + cksum field(1 byte, simply 8bit sum excluding itself)
	header := fmt(1byte) + tgt(1byte, opt) + src(1byte, opt) + len(1byte, opt,)
	fmt := An(1..0, 2bit, form of the header) + Ln(5..0, 6bit, length of the data field, if 0, len bye exist
	An(1..0) := 00, no addr info 01, exception mode(CARB) 10, with addr info, phy addressing 11, with addr info, function addressing
*/

/* fast initialization
*/
#define KWP_FAST_INIT_LOW_MS 25

/*keybytes
bit format:  AL0 AL1 HB0 HB1 TP0 TP1 1 P, 11110001
	AL0: len info in fmt byte is supported
	AL1: addtional len byte is supported
	HB0: 1 byte header is supported
	HB1: tgt/src addr byte in header is suported
	TP0..1: 01, normal timing 10, extented timing
*/

/* kwp timing
*/
typedef struct {
	short P1; /*inter byte time for ECU response*/
	short P2; /*time between tester req and ECU response, or two ECU response*/
	short P3; /*time between end of ECU response and start of new tester req*/
	short P4; /*inter byte time for tester req*/
} kwp_timing_t;

/*service id*/
enum {
	SCR = 0X81, /*start communication request*/
	ECR = 0X82, /*end communication request*/
	ACP = 0X83, /*access communication parameter*/
	SDR = 0X10, /*start diag request*/
	RST = 0X11, /*ECU reset*/
	CDI = 0X14, /*clear diag info*/
	RDTCBS = 0X18, /*read DTC by status*/
	RID = 0X1A, /*read ECU ID*/
	EDR = 0X20, /*stop diag req*/
	RDATBLID = 0X21, /*read data by local id*/
	RMEMBADR = 0X23, /*read mem by address*/
	SAR = 0X27, /*security access req*/
	DDLID = 0X2C, /*dynamic define local id*/
	IOCBLID = 0X30, /*io ctrl by local id*/
	SRBLID = 0X31, /*start routine by local id*/
	RDN = 0X34, /*request download*/
	TDR = 0X36, /*transfer data req*/
	RTE = 0X37, /*request transfer exit*/
	SRBADR = 0X38, /*start routine by addr*/
	WDATBLID = 0X3B, /*write data by local id*/
	WMEMBADR = 0X3D, /*write mem by addr*/
	TP = 0X3E, /*tester present*/
};

/*error codes*/
enum {
	GR = 0X10, /*general reject*/
	SNS = 0X11, /*service not supported*/
	SFNS_IF = 0X12, /*sub function not supported - invalid format*/
	B_RR = 0X21, /*busy - repeat request*/
	CNCORSE = 0X22, /*condition not correct or request sequency error*/
	RNC = 0X23, /*routine not complete*/
	ROOR = 0X31, /*request out of range*/
	SAD_SAR = 0X33, /*security access denied - security access requested*/
	SAA = 0X34, /*security access allowed*/
	IK = 0X35, /*invalid key*/
	ENOA = 0X36, /*exceed nr of request*/
	RTDNE = 0X37, /*required time delay not expired*/
	DNA = 0X40, /*download not accepted*/
	IDT = 0X41, /*improper download type*/
	CNDTSA = 0X42, /*cann't download to specified addr*/
	CNDNOBR = 0X43, /*can not download nr of bytes requested*/
	UNA = 0X50, /*upload not accepted*/
	IUT = 0X51, /*improper upload type*/
	CNUFSA = 0X52, /*can not upload from specified addr*/
	CNUNOBR = 0X53, /*can not upload nr of bytes requested*/
	TS = 0X71, /*transfer suspended*/
	TA = 0X72, /*transfer aborted*/
	IAIBT = 0X74, /*illegal addr in block transfer*/
	IBCIBT = 0X75, /*illegal byte count in block transfer*/
	IBTT = 0X76, /*illegal block transfer type*/
	BTDCE = 0X77, /*block transfer data cksum error*/
	RCR_RP = 0X78, /*req correctly received - response pending*/
	IBCDBT = 0X79, /*incorrect byte count during block transfer*/
	SNSIADM = 0X80, /*service not supported in active diagnostic mode*/
};

enum {
	KWP_STM_IDLE,
	KWP_STM_WAKE_LO,
	KWP_STM_WAKE_HI,
	KWP_STM_START_COMM,
	KWP_STM_READ_KBYTES,
	KWP_STM_READY,
	KWP_STM_TXRX,
	KWP_STM_ERR,
};

typedef struct {
	char fmt;
	char tgt;
	char src;
	char len;
} kwp_head_t;

void kwp_init(void);
void kwp_update(void);
int kwp_wake(void);
int kwp_isready(void);

/* allocate a new kwp frame with maxium n bytes of data
return the pointer to data buf
*/
void *kwp_malloc(size_t n);
void kwp_free(void *p);

/* send n bytes of data in buf with kwp protocol
then receive the response data and store it to the buf
note: buf size info is recorded in kwp_malloc
return:
	bytes of data received, or error code
*/
int kwp_transfer(char *buf, size_t n);

/* return how many response data has been received yet
*/
int kwp_poll(void);

#endif /*__KWP_H_*/
