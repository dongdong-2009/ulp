/*
 * 	miaofng@2010 initial version
 *		This file is used to provide data link level/app level i/f for ISO14230/keyword2000 protocol
 */
#ifndef __KWP_H_
#define __KWP_H_


/* fast initialization
*/
#define KWP_FAST_INIT_MS 25
#define KWP_RECV_TIMEOUT_MS 500

/*keybytes
bit format:  AL0 AL1 HB0 HB1 TP0 TP1 1 P, 11110001
	AL0: len info in fmt byte is supported
	AL1: addtional len byte is supported
	HB0: 1 byte header is supported
	HB1: tgt/src addr byte in header is suported
	TP0..1: 01, normal timing 10, extented timing
*/
#define KWP_KB_AL0 (1 << 0)
#define KWP_KB_AL1 (1 << 1)
#define KWP_KB_HB0 (1 << 2)
#define KWP_KB_HB1 (1 << 3)

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
	/*standard service*/
	SID_10 = 0X10, /*!start diag request*/
	SID_11 = 0X11, /*!ECU reset*/
	SID_14 = 0X14, /*!clear diag info*/
	SID_18 = 0X18, /*read DTC by status*/
	SID_1A = 0X1A, /*read ECU ID*/
	SID_20 = 0X20, /*!stop diag req*/
	SID_21 = 0X21, /*read data by local id*/
	SID_23 = 0X23, /*!read mem by address*/
	SID_27 = 0X27, /*!security access req*/
	SID_2C = 0X2C, /*dynamic define local id*/
	SID_30 = 0X30, /*!io ctrl by local id*/
	SID_31 = 0X31, /*!start routine by local id*/
	SID_32 = 0X32, /*!stop routine by local id*/
	SID_33 = 0X33, /*!request routine result by local id*/
	SID_34 = 0X34, /*!request download*/
	SID_36 = 0X36, /*transfer data req*/
	SID_37 = 0X37, /*!request transfer exit*/
	SID_38 = 0X38, /*!start routine by addr*/
	SID_39 = 0X39, /*!stop routine by addr*/
	SID_3A = 0X3A, /*!request routine result by addr*/
	SID_3B = 0X3B, /*!write data by local id*/
	SID_3D = 0X3D, /*!write mem by addr*/
	SID_3E = 0X3E, /*tester present*/
	SID_81 = 0X81, /*!start communication request*/
	SID_82 = 0X82, /*!end communication request*/
	SID_83 = 0X83, /*!access communication parameter*/
	
	SID_ERR = 0X7F, /*negative response*/
	
	/*special for utility only*/
	SID_01 = 0X01, /*!setup kwp2000 programming*/
	SID_02 = 0X02, /*!usr defined SR(development only)*/
	SID_03 = 0X03, /*!Set nr of Repetitions*/
	SID_71 = 0X71, /*!SID_31 + */
	SID_78 = 0X78, /*!SID_38 + */
	SID_84 = 0X84, /*!SID_83 + */
	SID_90 = 0X90, /*!SID_36 + */
	SID_93 = 0X93, /*!SID_36 ++ */
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

/*kwp frame
byte format: header field(1-4 bytes) + data field(1~255 bytes) + cksum field(1 byte, simply 8bit sum excluding itself)
	header := fmt(1byte) + tgt(1byte, opt) + src(1byte, opt) + len(1byte, opt, length of data field)
	fmt := An(1..0, 2bit, form of the header) + Ln(5..0, 6bit, length of the data field, if 0, len bye exist
	An(1..0) := 00, no addr info 01, exception mode(CARB) 10, with addr info, phy addressing 11, with addr info, function addressing
*/
#define KWP_FMT(fmt) (fmt & (0x03 << 6))
enum {
	KWP_FMT_ADR_NON = 0x00 << 6,
	KWP_FMT_INVALID = 0x01 << 6,
	KBP_FMT_ADR_PHY = 0x02 << 6,
	KBP_FMT_ADR_FUN = 0x03 << 6,
};

typedef struct {
	char fmt;
	char tgt; //A1..0 00 no addr info, 01 exception mode, 10 with addr
	char src;
	char len;
} kwp_head_t;

typedef struct {
	char rid; /*response id*/
	char sid;
	char code;
} kwp_err_t;

#define KWP_DEVICE_ID 0X11
#define KWP_TESTER_ID 0XF1

int kwp_Init(void);
int kwp_GetLastErr(char *rid, char *sid, char *code); /*0->no err*/
void kwp_SetAddr(char tar, char src);

/*service routines:
	= 0, routine finished sucessfully
	<0, error occurs
*/
int kwp_EstablishComm(void);
int kwp_StartComm(char *kb0, char *kb1);
int kwp_StopComm(void);
int kwp_AccessCommPara(void);
int kwp_StartDiag(char mode, char baud);
int kwp_SecurityAccessRequest(char mode, short key, int *seed);
int kwp_RequestToDnload(char fmt, int addr, int size, char *plen);
int kwp_TransferData(int addr, int size, char *data);
int kwp_RequestTransferExit(void);
int kwp_StartRoutineByAddr(int addr);

#endif /*__KWP_H_*/
