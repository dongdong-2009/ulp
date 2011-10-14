/* START LIBRARY DESCRIPTION ***************************************************
MT221.c
*.
*.COPYRIGHT (c) 2006 Delphi Corporation, All Rights Reserved
*.
*.Security Classification: DELPHI CONFIDENTIAL
*.
*.Description: This file contains the MT80 SGM conditioning nest functions.
*.
*.Functions:
*.	 main
*.	  CndOps
*.		 InitCCP
*.
*.
*.
*.
*.
*.
*.Language: Z-World Dynamic C ( ANSII C with extensions and variations )
*.
*.Target Environment: Rabbit Semiconductor, Rabbit 3000 microprocessor used in
*.						  RCM3xxx core module.
*.
*.Notes:
*.> Revision records: The revision of this file reflects the revision of
*.the complete program. The constant "_PGM_Version_" should contain the latest
*.revision date for this file. If any file or function in this program is
*.changed the revision log of this file should be updated to indicate which
*.file or function was modified. The revision log in the file or function
*.modified should contain the details of the change.
*.
*.******************************************************************************
*.Revision:
*.
*.DATE	  USERID  DESCRIPTION
*.-------  ------  ------------------------------------------------------------
*.21June06  zzrq7q > Initial release using Libraries developed by Keller/Hurtley
*.04Oct06	terence> Rename working directory to C:\DCRABBIT_9.25\MT80
*.							Case 8, run VerifyDUTSignal1()-TACH instead of VerifyDUTSignal2()- IAC
*.							Case 8, commented out Collect_Fault() and Store_Fault()
*.							Cond. period changed from 15 min to 30 seconds.
*.							PSV flag temporarily check for 0x00 instead of 0x03.
*.							Case 9, default cond. flag verify to pass '0'.
*.							case 7, reduce IGN cycle to 2 "(IGN_Count<2)"
*.						 Comment all flash write command including:
*.							1)UINT16 CndOps(void), case 5,
*.							In routine ERROR_CODE Cyc_IGN(void) set IGN on last command.
* 16Oct06  Pradeep > Added Verify Faulta data functionality.
* 22Sep08  fzwf47  > Initial version for project mt22.1
* 19Nov10	fzwf47 > modify for arm platform
END DESCRIPTION ***************************************************************/

#include "lib/nest.h"
#include "obsolete/obsolete.h"
#include "mt221.h"
#include "ramdnld/mt18p1_cyc.h"
#include "ramdnld/mt22p1p1ss_cyc.h"

//mt22.1 xmem data
const char *xVsepChName[] = {
	"J1-04 ESTA/COILA",
	"J1-01 ESTB/COILB",
	"J1-62 ESTC/COILC",
	"J1-81 ESTD/COILD",
	"J1-44 MPR",
	"J1-09 FPR",
	"J1-10 AC CLUTCH",
	"J1-17 FAN2",

	"J1-06 INJA",
	"J1-07 INJB",
	"J1-25 INJC",
	"J1-08 INJD",
	"J1-18 TACH",
	"J1-46 SAIC",
	"J1-65 FAN1",
	"J1-64 CANISTER PURGE",

	"J1-43 VVT-1",
	"J1-22 VVT-2/LGER",
	"J1-16 IC FAN/VGIS1",
	"J1-45 TAPR/VGIS2",
	"J1-24 FRONT-O2",
	"J1-23 REAR-O2",
	"J1-63 TURBO RELIEF",
	"J1-13 TURBO AWG",

	"J1-19 MIL/CEL",
	"J1-12 SVS",
	"J1-61 COOLANT GAUGE/THERMAL CTRL",
	"J1-15 SAIC PWM",
	"J1-80 FUEL CONSUMP",
	"J1-14 CRUISE LIGHT"
};

#define VSEP_CH_NR			((UINT8)30)

const char *xPsviChName[] = {
	//LFP1~16
	"J1-06 INJA", //LFP1
	"J1-07 INJB", //LFP2
	"J1-25 INJC", //LFP3
	"J1-08 INJD/TURBO", //LFP4
	"J1-64 CANISTER PURGE", //LFP5
	"J1-65 FAN1", //LFP6
	"J1-24 FRONT-O2", //LFP7
	"J1-23 REAR-O2", //LFP8

	"J1-17 FAN2/ACFAN", //LFP9
	"J1-19 MIL/CEL", //LFP10
	"J1-04 ESTA/COILA", //LFP11
	"J1-01 ESTB/COILB", //LFP12
	"J1-62 ESTC/COILC", //LFP13
	"J1-81 ESTD/COILD?", //LFP14
	"J1-16 SPWM1", //LFP15
	"J1-43 SPWM2", //LFP16

	//LED1~6
	"J1-61 FAN3PWM", //LED1
	"J1-78 IMMOREQ", //LED2
	"J1-18 TACH", //LED3
	"J1-15 SPWM3", //LED4
	"J1-80 SPWM4", //LED5
	"J1-14 LED1", //LED6

	//UP1~4
	"J1-44 MPR",
	"J1-09 FPR",
	"J1-10 AC CLUTCH",
	"J1-12 SVS"
};

#define PSVI_CH_LFP_OFFSET 0
#define PSVI_CH_LFP_NR 16
#define PSVI_CH_LED_OFFSET (PSVI_CH_LFP_OFFSET+PSVI_CH_LFP_NR)
#define PSVI_CH_LED_NR 6
#define PSVI_CH_UP_OFFSET (PSVI_CH_LED_OFFSET+PSVI_CH_LED_NR)
#define PSVI_CH_UP_NR 4
#define PSVI_CH_NR (PSVI_CH_LFP_NR+PSVI_CH_LED_NR)

const model_t model_mt221 = {
	0X0F7400UL, //UINT32 inbox_addr;
	0X0F7420UL, //UINT32 outbox_addr;

	//test start/stop
	0X100009UL, //UINT32 pcb_seq_addr;
	0X100015UL, //UINT32 psv_amb_addr;
	0X100013UL, //UINT32 psv_cnd_addr;
	0x100018UL, //UINT32 mtype_addr;
	0x100020UL, //UINT32 fault_byte_addr;
	0x10001AUL, //UINT32 error_code_addr;

	//ram test
	0X0F9000UL, //UINT32 ramtest_start_addr;
	0X0FFFFFUL, //UINT32 ramtest_end_addr;

	//cycling test
	0UL, //long ramdnld;
	xVsepChName //long* ch_info;
};

//mt18.1 xmem data
const model_t model_mt181 = {
	0X0F1000UL, //UINT32 inbox_addr;
	0X0F1080UL, //UINT32 outbox_addr;

	//test start/stop
	0X100009UL, //UINT32 pcb_seq_addr;
	0X100015UL, //UINT32 psv_amb_addr;
	0X100013UL, //UINT32 psv_cnd_addr;
	0X100018UL, //UINT32 mtype_addr;
	0X100020UL, //UINT32 fault_byte_addr;
	0X10001AUL, //UINT32 error_code_addr;

	//ram test
	0X0F9000UL, //UINT32 ramtest_start_addr;
	0X0FFFFFUL, //UINT32 ramtest_end_addr;

	//cycling test
	ramdnld_mt18p1_cyc, //long ramdnld;
	xPsviChName //long* ch_info;
};

//mt22.1.1ss xmem data
const model_t model_mt2211ss = {
	0X0F1000UL, //UINT32 inbox_addr;
	0X0F1080UL, //UINT32 outbox_addr;

	//test start/stop
	0X100009UL, //UINT32 pcb_seq_addr;
	0X100015UL, //UINT32 psv_amb_addr;
	0X100013UL, //UINT32 psv_cnd_addr;
	0X100018UL, //UINT32 mtype_addr;
	0X100020UL, //UINT32 fault_byte_addr;
	0X10001AUL, //UINT32 error_code_addr;

	//ram test
	0X0F9000UL, //UINT32 ramtest_start_addr;
	0X0FFFFFUL, //UINT32 ramtest_end_addr;

	//cycling test
	ramdnld_mt22p1p1ss_cyc, //long ramdnld;
	xPsviChName //long* ch_info;
};

//global variables declaration
fis_data_t fis;
ModelType mtype;
model_t model;
fault_bytes_t fb;
UINT8 Base_Model_Nr [8];

void NestPowerOn(void)
{
	message("Nest Power On\n");
	// flash led for visual verify.
	NestLightControl(ALL_ON);
	Delay10ms(10);
	NestLightControl(ALL_OFF);

	//enable solenoid
	RELAY_LOCK_SET(1);

	// Set nest's LED indicators and apply power to target.
	NestLightControl(RUNNING_ON);
	RELAY_BAT_SET(1);
	RELAY_ETCBAT_SET(1);
	Delay10ms(10);
	RELAY_IGN_SET(1);
	Delay10ms(50);
}


void NestReboot(void)
{
	message("Nest Reboot\n");
	/*Do not switch on/off BAT relay!!! We need to lock the DUT on the fixture by that signal*/
	NestLightControl(RUNNING_OFF);
	RELAY_IGN_SET(0);
	Delay10ms(1);
	RELAY_ETCBAT_SET(0);
	RELAY_BAT_SET(0);
	Delay10ms(100);

	//DelayS(1); //wait for capacitor discharge
	NestLightControl(RUNNING_ON);
	RELAY_BAT_SET(1);
	RELAY_ETCBAT_SET(1);
	Delay10ms(10);
	RELAY_IGN_SET(1);
	Delay10ms(50);
}

void NestPowerOff(void)
{
	message("Nest PowerOff\n");
	NestLightControl(ALL_OFF);          // Turn all indicators off.

	//release solenoid
	RELAY_LOCK_SET(0);

	//BAT&IGN&ETCBAT
	RELAY_IGN_SET(0);
	Delay10ms(1);
	RELAY_BAT_SET(0);
	RELAY_ETCBAT_SET(0);
	Delay10ms(1);
}

ERROR_CODE wait_test_finish(UINT8 testID, UINT32 timeout)
{
	ERROR_CODE result ;
	UINT8 id;
	UINT8 poll_cnt;
	UINT32 poll_interval; //unit 100uS
	UINT8 poll_dlycnt;

	//timeout value is expect to be smaller than 16*op_time
	//and we'll poll 16 times at a normal op_time.
	//then poll_interval = op_time/16 = timeout /256;
	timeout = timeout * 10; //unit change: ms -> 100uS
	poll_cnt = 255; //max poll 255 times
	poll_interval = timeout >> 8; //unit 100uS
	if(poll_interval == 0) {
		poll_interval = 1; //at least 1
		poll_cnt = timeout - 1;
	}

	while(poll_cnt --  > 0 )
	{
		//inbox id clear ???
		result = mcamOSUpload(DW_CAN1, &id, INBOX_ADDR, 1, CAN_TIMEOUT);
		if(result != E_OK) return result;
		if (id != testID)
		{
			if(testID != 0)
			{
				//outbox id set ???
				result = mcamOSUpload(DW_CAN1, &id, OUTBOX_ADDR, 1, CAN_TIMEOUT);
				if(result != E_OK) return result;
 				if(id == testID) return E_OK;
			} else
			{ //testID == 0, factory mode test...we won't do outbox check
				return E_OK;
			}
		}

		//delay
		poll_dlycnt = poll_interval / 10000; //x(100uS) /10000 = x/10000 (S)
		poll_interval = poll_interval % 10000;
		DelayS(poll_dlycnt); //0~255 S

		poll_dlycnt = poll_interval / 100; //x(100uS) /100 = x/100 (10mS)
		poll_interval = poll_interval % 100;
		Delay10ms(poll_dlycnt); //0~100 (*10mS)

		poll_dlycnt = poll_interval; //x(100uS)
		Delay100us(poll_dlycnt); // 0~100 (*100uS)
	}

	return E_OOPS;
}

#ifdef MCAMOS_SELFTEST
#define MCAMOS_SELFTEST_PATTERN_LEN 8
#define MCAMOS_SELFTEST_PATTERN_POS (OUTBOX_ADDR)
ERROR_CODE mcamos_selftest(void) //UINT32 addr, UINT8* pdata, int len)
{
	ERROR_CODE result ;
	UINT8 buffer[MCAMOS_SELFTEST_PATTERN_LEN];
	UINT32 cnt;

	result  = E_OK;

	//generate test pattern
	for(cnt = 0; cnt < MCAMOS_SELFTEST_PATTERN_LEN; cnt ++) buffer[cnt] = (cnt<<4)|cnt;

	// write the pattern to the mailbox
	result = mcamOSDownload(DW_CAN1, MCAMOS_SELFTEST_PATTERN_POS, buffer, MCAMOS_SELFTEST_PATTERN_LEN, CAN_TIMEOUT);
	if (result != E_OK) return result;

	// clear the buffer
	memset(buffer, '\0', MCAMOS_SELFTEST_PATTERN_LEN);

	// read back the pattern from the mailbox
	result = mcamOSUpload(DW_CAN1, buffer, MCAMOS_SELFTEST_PATTERN_POS, MCAMOS_SELFTEST_PATTERN_LEN, CAN_TIMEOUT);
	if (result != E_OK) return result;

	//pattern ok?
	for(cnt = 0; cnt < MCAMOS_SELFTEST_PATTERN_LEN; cnt ++)
	{
		if(buffer[cnt] != (cnt<<4|cnt)) {
			result = E_ERR;
			break;
		}
	}

	return result;
}
#else
	#define mcamos_selftest() E_OK
#endif

#define FACTORY_TEST_MODE_TIMEOUT 100 //unit ms
#define PRINT_SOFTWARE_VERSION_MAGIC 0x55
ERROR_CODE PrintSoftwareVersion(void)
{
	ERROR_CODE result ;
	UINT8 buffer[8];
	UINT8 major, minor;

	//send factory test command to mcamos inbox
	buffer[0] = FACTORY_TEST_MODE_TESTID;
	buffer[1] = PRINT_SOFTWARE_VERSION_MAGIC;
	result = mcamOSDownload(DW_CAN1, INBOX_ADDR, buffer, 2, CAN_TIMEOUT);
	if(result != E_OK) return result;

	//wait
	result = wait_test_finish(FACTORY_TEST_MODE_TESTID, FACTORY_TEST_MODE_TIMEOUT);
	if (result != E_OK) return result;

	//print result
	result = mcamOSUpload(DW_CAN1, buffer, OUTBOX_ADDR, 4, CAN_TIMEOUT);
	if (result != E_OK) return result;

	//verify magic byte and get software version
	if(mtype.model == MODEL_MT221) {
		if(buffer[0] != PRINT_SOFTWARE_VERSION_MAGIC) return E_OOPS;
		major = buffer[1];
		minor = buffer[2];
		message("TESTABILITY S/W VERSION: mt22.1 or mt60.1 %c.%d\n", (major-10+'A'), minor);
	}
	else {
		if(buffer[1] != PRINT_SOFTWARE_VERSION_MAGIC) return E_OOPS;
		major = buffer[2];
		minor = buffer[3];
		message("TESTABILITY S/W VERSION: mt18.1 %c.%d\n", (major-10+'A'), minor);
	}

	return result;
}

#define MEMORY_READ_TIMEOUT		DFT_OP_TIMEOUT
ERROR_CODE Read_Memory(UINT32 addr, UINT8* pdata, int len)
{
	ERROR_CODE result ;
	UINT8 buffer[5];

	//send memory read command to mcamos inbox
	buffer[0] = MEMORY_READ_TESTID;
	buffer[1] = len&0xff;
	buffer[2] = (addr>>16)&0xff;
	buffer[3] = (addr>> 8)&0xff;
	buffer[4] = (addr>> 0)&0xff;
	result = mcamOSDownload(DW_CAN1, INBOX_ADDR, buffer, sizeof(buffer), CAN_TIMEOUT);
	if (result != E_OK) return result;

	//check op finish
	result = wait_test_finish(MEMORY_READ_TESTID, MEMORY_READ_TIMEOUT);
	if (result != E_OK) return result;

	//read data from mcamos outbox(note: byte 0 is TestID, should be ignored)
	result = mcamOSUpload(DW_CAN1, pdata, OUTBOX_ADDR+1, len, CAN_TIMEOUT);
	return result;
}

#define MEMORY_WRITE_TIMEOUT		DFT_OP_TIMEOUT //unit mS
#define EEPROM_WRITE_TIMEOUT		DFT_OP_TIMEOUT //uint mS
#define FLASH_WRITE_TIMEOUT		DFT_OP_TIMEOUT //uinit mS
ERROR_CODE Write_Memory(UINT32 addr, UINT8* pdata, int len)
{
	ERROR_CODE result ;
	UINT8 buffer[5];

	//fill up the frame header
	buffer[0] = EEPROM_WRITE_TESTID;
	buffer[1] = len&0xff;
	buffer[2] = (addr>>16)&0xff;
	buffer[3] = (addr>> 8)&0xff;
	buffer[4] = (addr>> 0)&0xff;

	//send data payload
	result = mcamOSDownload(DW_CAN1, INBOX_ADDR+5, pdata, len&0xff, CAN_TIMEOUT);
	if (result != E_OK) return result;

	//send header
	result = mcamOSDownload(DW_CAN1, INBOX_ADDR, buffer, sizeof(buffer), CAN_TIMEOUT);
	if (result != E_OK) return result;

	//wait
	result = wait_test_finish(EEPROM_WRITE_TESTID, EEPROM_WRITE_TIMEOUT);
	if (result != E_OK) return result;

	//read op result
	result = mcamOSUpload(DW_CAN1, buffer, OUTBOX_ADDR, 2, CAN_TIMEOUT);
	if (result != E_OK) return result;

	if(buffer[1] == 0x00) result = E_OK;
	else result = DUT_OOPS;

	return result;
}

ERROR_CODE Execute_RAMDnld(UINT32 addr, UINT8 *pfile, int len)
{
	ERROR_CODE result;

	//download to ram
	result = mcamOSDownload(DW_CAN1, addr, pfile, len, 10000); //10s timeout
	if( result != E_OK ) return result;

	//execute
	result = mcamOSExecute(DW_CAN1, addr, CAN_TIMEOUT);
	return result;
}

enum {
	L9958_FAULTTEST_MODE_OPEN_FAULT = 0,
	L9958_FAULTTEST_MODE_SHORT_FAULT,
	L9958_FAULTTEST_MODE_OVER_CURRENT
};

#define L9958_FAULTTEST_TIMEOUT		DFT_OP_TIMEOUT //unit mS
ERROR_CODE Execute_L9958FaultTest(char mode, UINT8* pdata)
{
	ERROR_CODE result ;
	UINT8 buffer[2];

	//send test command
	buffer[0] = L9958_FAULTTEST_TESTID;
	buffer[1] = (mode&0xff);
	result = mcamOSDownload(DW_CAN1, INBOX_ADDR, buffer, 2, CAN_TIMEOUT);
	if(result != E_OK) return result;

	//wait
	result = wait_test_finish(L9958_FAULTTEST_TESTID, L9958_FAULTTEST_TIMEOUT);
	if(result != E_OK) return result;

	//read result
	result = mcamOSUpload(DW_CAN1, pdata, OUTBOX_ADDR+1, 1, CAN_TIMEOUT);
	return result;
}

#define LCM555_FREQTEST_TIMEOUT		DFT_OP_TIMEOUT //unit mS
ERROR_CODE Execute_LCM555FreqTest(UINT8* pdata)
{
	ERROR_CODE result ;
	UINT8 buffer[2];

	//send ram test command to mcamos inbox
	buffer[0] = LCM555_FREQTEST_TESTID;
	result = mcamOSDownload(DW_CAN1, INBOX_ADDR, buffer, 1, CAN_TIMEOUT);
	if(result != E_OK) return result;

	//wait
	result = wait_test_finish(LCM555_FREQTEST_TESTID, LCM555_FREQTEST_TIMEOUT);
	if(result != E_OK) return result;

	//read result
	result = mcamOSUpload(DW_CAN1, pdata, OUTBOX_ADDR+1, 2, CAN_TIMEOUT);
	return result;
}

enum {
	VSEP_FAULTTEST_MODE_OPENSHORT_TO_GND = 0,
	VSEP_FAULTTEST_MODE_SHORT_TO_BAT
};

#define VSEP_FAULTTEST_TIMEOUT		DFT_OP_TIMEOUT //unit mS
ERROR_CODE Execute_VSEPFaultTest(char mode, UINT8* pdata)
{
	ERROR_CODE result ;
	UINT8 buffer[2];

	//send ram test command to mcamos inbox
	buffer[0] = VSEP_FAULTTEST_TESTID;
	buffer[1] = (mode&0xff);
	result = mcamOSDownload(DW_CAN1, INBOX_ADDR, buffer, 2, CAN_TIMEOUT);
	if(result != E_OK) return result;

	//wait
	result = wait_test_finish(VSEP_FAULTTEST_TESTID, VSEP_FAULTTEST_TIMEOUT);
	if(result != E_OK) return result;

	//read result
	result = mcamOSUpload(DW_CAN1, pdata, OUTBOX_ADDR+1, 8, CAN_TIMEOUT);
	return result;
}

#define RAM_TEST_TIMEOUT			((UINT32)10000)  //10s

ERROR_CODE Execute_RAMTest(UINT32 Start_Addr, UINT32 End_Addr)
{
	ERROR_CODE result ;
	UINT8 buffer[7];
	UINT8 len;

	len = 1;
	//send ram test command to mcamos inbox
	buffer[0] = RAM_TEST_TESTID;
	if(mtype.model == MODEL_MT221)
	{
		buffer[1] = (Start_Addr>>16)&0xff;
		buffer[2] = (Start_Addr>> 8)&0xff;
		buffer[3] = (Start_Addr>> 0)&0xff;
		buffer[4] = (End_Addr>>16)&0xff;
		buffer[5] = (End_Addr>> 8)&0xff;
		buffer[6] = (End_Addr>> 0)&0xff;
		len = 7;
	}
	result = mcamOSDownload(DW_CAN1, INBOX_ADDR, buffer, len, CAN_TIMEOUT);
	if(result != E_OK) return result;

	//wait
	if(mtype.model == MODEL_MT181 ) DelayS(2); //mt18.1 bug
	result = wait_test_finish(RAM_TEST_TESTID, RAM_TEST_TIMEOUT);
	if(result != E_OK) return result;

	//read op result
	result = mcamOSUpload(DW_CAN1, buffer, OUTBOX_ADDR, 2, CAN_TIMEOUT);
	if(result != E_OK) return result;

	//check result
	if(buffer[1] == 0) return E_OK;
	else return DUT_OOPS;
}
#define RAM_TEST_START_ADDR		(model.ramtest_start_addr)
#define RAM_TEST_END_ADDR			(model.ramtest_end_addr)

/* START FUNCTION DESCRIPTION *************************************************
RAMTest
*.
*.Syntax: void RAMTest(void)
*.
*.Description: To do the ECM ram test
*.
*.Parameters
*.	error		error code
*.External References:
*.Return Value:
*.
*.Notes:
*.
*.*****************************************************************************
*.Revision:
*.
*.DATE	  USERID  DESCRIPTION
*.-------  ------  ------------------------------------------------------------
*.18Nov08	 fzwf47	> Initial release.
*.
END DESCRIPTION ***************************************************************/
void RAMTest(void)
{
	ERROR_CODE result;

	if(fail()) return;
	message("#RAM Test Start ... ");

	result = Execute_RAMTest(RAM_TEST_START_ADDR, RAM_TEST_END_ADDR);
	if (result != E_OK) ERROR(RAM_FAIL, "RAM Test");
	else message("pass\n");

	//restart the dut
	NestReboot();
	return;
}

#define RAMDNLD_ADDR	((UINT32)0X0F2000)
void RAMDnld(void)
{
	ERROR_CODE result;
	int len;
	UINT8 *pfile;

	if(fail()) return;
	if(model.ramdnld == 0) return;

	pfile = (UINT8 *)model.ramdnld;
	len = *((int *) pfile);
	message("#RAM Download(addr=0x%08lx, size= %04ld) Start ... ", RAMDNLD_ADDR, len);
	pfile += 4;

	result = Execute_RAMDnld(RAMDNLD_ADDR, pfile, len);
	if (result == E_OK) {
		message("pass\n");
		message("RAMDNLD ");
		result = PrintSoftwareVersion();
	}
	if (result != E_OK) ERROR(MISC_FAIL, "RAM Download");

	return;
}

#define OUTPUT_CYCLING_TESTID		((UINT8)0x21)
ERROR_CODE Execute_Cycling(ModelType mtype)
{
	ERROR_CODE result ;
	UINT8 buffer[3];
	UINT32 cnt;
	UINT8 len;

	/* base model type option:
		0	cycle ETC, 4EST/IGBT in sequential mode and all other output
		1	cycle ETC, 2EST/IGBT in paired mode and all other output
		2	cycle IAC, 4EST/IGBT in sequential mode and all other output
		3	cycle IAC, 2EST/IGBT in paired mode and all other output.
	*/
	len = 2;

	//send output cycling command to mcamos inbox
	buffer[0] = OUTPUT_CYCLING_TESTID;
	buffer[1] = (mtype.iac<<1) + mtype.par;
	if(mtype.model == MODEL_MT181)
	{
	/* mt18.1 base model option:
		'0', 3EST/IGBT and all other output
		'1', 2EST/IGBT and all other output
	*/
		if(mtype.par == 3) buffer[1] = 0;
		else buffer[1] = 1;

		buffer[2] = SEC(RUN_FAULT_BYTE_TEST_DUTY);
		len =3;
	}
	result = mcamOSDownload(DW_CAN1, INBOX_ADDR, buffer, len, CAN_TIMEOUT);

	return result;
}

/*Testability I/F Functions -- end*/
const char * get_ch_name(UINT8 ch)
{
	if(model.ch_info) {
		return model.ch_info[ch];
	}
	else {
		return "undefined ch";
	}
}

enum {
	VSEP_CH_OK, //0X00
	VSEP_CH_OPEN, //0X01, open load
	VSEP_CH_GND, //0x10, short to ground
	VSEP_CH_BAT //0x11, short to battery
};

static const char * pStrChFailTypeName[] =
{
	"ok",
	"open load",
	"short to ground",
	"short to battery"
};

ERROR_CODE Verify_VSEPFaultTest(UINT8 * pdata)
{
	ERROR_CODE result;
	UINT8 ch;
	UINT8 val;

	result = E_OK;

	//compare each channel value
	for(ch = 0; ch < VSEP_CH_NR; ch ++)
	{
		//ch bypass??
		if( (ch == 2) || (ch == 3))
		{
			if (mtype.par == 1) continue;
		}

		if((ch == 18) || (ch == 19)) { //PCH19..PCH20 MT221SS BYPASS
			if(mtype.mt221ss)
				continue;
		}

		//get ch val
		val = pdata[(UINT8)(ch/4)];
		val = (val>>((ch%4)*2))&0x03;

		//ch val verify
		switch (val){
			case VSEP_CH_BAT:
			case VSEP_CH_GND:
			case VSEP_CH_OPEN:
				if(result == E_OK){
					if((mtype.mt601 == 1\
						&& (ch!=0) /*COILA BYPASS*/ \
						&& (ch!=1) /*COILB BYPASS*/ \
						&& (ch!=2) /*COILC BYPASS*/ \
						&& (ch!=3) /*COILD BYPASS*/ \
						&& (ch!=5)/*J1-09 FPR BYPASS*/\
						&& (ch!=6)/*J1-10 AC CLUTCH BYPASS*/\
						&& (ch!=23)/*J1-13 TURBO AWG BYPASS*/\
						&& (ch!=25)/*J1-12 SVS BYPASS*/\
						&& (ch!=26)/*J1-61 COOLANT GAUGE/THERMAL BYPASS*/\
						&& (ch!=28)/*J1-80 FUEL CONSUMP BYPASS*/\
						&& (Base_Model_Nr[4] == 0x37) && (Base_Model_Nr[5] == 0x37) && (Base_Model_Nr[6] == 0x35) && (Base_Model_Nr[7] == 0x31))\
						||(mtype.mt601 == 1\
						&& (ch!=0) /*COILA BYPASS*/ \
						&& (ch!=1) /*COILB BYPASS*/ \
						&& (ch!=2) /*COILC BYPASS*/ \
						&& (ch!=3) /*COILD BYPASS*/ \
						&& (ch!=5)/*J1-09 FPR BYPASS*/\
						&& (ch!=6)/*J1-10 AC CLUTCH BYPASS*/\
						&& (Base_Model_Nr[4] == 0x31) && (Base_Model_Nr[5] == 0x32) && (Base_Model_Nr[6] == 0x32) && (Base_Model_Nr[7] == 0x38))\
						||(mtype.mt601 != 1\
						&& (ch!=0) /*COILA BYPASS*/ \
						&& (ch!=1) /*COILB BYPASS*/ \
						&& (ch!=2) /*COILC BYPASS*/ \
						&& (ch!=3) /*COILD BYPASS*/ )
					) result = DUT_OOPS;
					message("Vsep Fault:\n");
				}
				message("PCH%02d = %02x: %s %s!!!\n", ch+1, val, get_ch_name(ch), pStrChFailTypeName[val]);
				break;
			default:
				break;
		};
	}

	return result;
}

enum {
	PSVI_OK, //0X00
	PSVI_LOW_DURING_OFF,
	PSVI_HIGH_DURING_ON,
	PSVI_INVERT //low_during_off||high_during_on
};

static const char * pStrPSVIFailType[] =
{
	"ok",
	"low_during_off",
	"high_during_on",
	"low(high)_during_off(on)"
};

ERROR_CODE Verify_PSVIFaultTest(UINT8 * pdata)
{
	ERROR_CODE result;
	UINT8 ch;
	UINT8 bit0, bit1,val;
	UINT8 index;

	result = E_OK;

	for(ch = 0; ch < PSVI_CH_NR; ch ++)
	{
		//ch bypass??
		switch(ch){
			case 10: //LPF11, ESTA/COILA
			case 11: //LPF12, ESTB/COILB
				break;
			case 12: //LPF13, ESTC/COILC
			case 13: //LPF14, ESTD/COILD
				if((ch-10 + 1) > mtype.par) continue;
				break;
			default:
				break;
		};

		//get ch val
		if(ch < PSVI_CH_LED_OFFSET) {  //LFP
			index = (ch%8);
			bit0 = fb.data.mt181.psvi[0+ch/8];
			bit1 = fb.data.mt181.psvi[2+ch/8];
			bit0 = ((bit0 >> (7-index)) & 0x01); //big endian
			bit1 = ((bit1 >> (7-index)) & 0x01); //big endian
			val = ((bit1<<1)|(bit0<<0));

			if((mtype.igbt == 0)&&(ch >= 10 )&&(ch <= 13))
			{
				//ESTA~C
				index = ch - 10;
				val = fb.data.mt181.psvi[4];
				val = ((val >> (2*index))&0x03);
			}
		}
		else {  //LED
			index = ch - PSVI_CH_LED_OFFSET;
			bit0 = fb.data.mt181.psvi[5];
			bit1 = fb.data.mt181.psvi[6];
			bit0 = ((bit0 >> (7-(2+index))) & 0x01); //big endian
			bit1 = ((bit1 >> (7-(2+index))) & 0x01); //big endian
			val = ((bit1<<1)|(bit0<<0));
		}

		//ch val verify
		switch (val){
			case PSVI_LOW_DURING_OFF:
			case PSVI_HIGH_DURING_ON:
			case PSVI_INVERT:
				if(result == E_OK){
					if((ch!=9) /*MIL/CEL BYPASS*/ \
					) result = DUT_OOPS;
					message("PSVI Fault:\n");
				}
				if(ch < PSVI_CH_LED_OFFSET) message("LPF%02d = ", ch + 1);
				else message("LED%02d = ", ch -PSVI_CH_LED_OFFSET + 1);
				message("%02x: %s %s!!!\n", val, get_ch_name(ch), pStrPSVIFailType[val]);
				break;
			default:
				break;
		};
	}

	return result;
}

ERROR_CODE ReadFaultByte(void)
{
	ERROR_CODE result ;
	UINT16 len;

	if(mtype.model == MODEL_MT221) len = sizeof(fb.data.mt221);
	else len = sizeof(fb.data.mt181);

#if 0
	//wait
	result = wait_test_finish(OUTPUT_CYCLING_TESTID, DFT_OP_TIMEOUT);
	if(result != E_OK) return result;
#endif

	//read result
	result = mcamOSUpload(DW_CAN1, fb.data.buf, OUTBOX_ADDR+1, len, CAN_TIMEOUT);
	return result;
}

ERROR_CODE MT221_VerifyFaultByte(void)
{
	ERROR_CODE result;
	result = E_OK;

	if(mtype.iac == 0){
		//verify ETC driver(L9958) test result
		if((fb.data.mt221.l9958&0xe0/*0xf0*/) != 0) {
			message("L9958 fault!\n");
			return E_OOPS;
		}

		//verify LCM555
		if( fb.data.mt221.lcm555[0] )
		{
			message("LCM555 fault!\n");
			return E_OOPS;
		}
	}

	//verify VSEP
	result = Verify_VSEPFaultTest(fb.data.mt221.vsep);
	return result;
}

ERROR_CODE MT181_VerifyFaultByte(void)
{
	ERROR_CODE result;
	UINT8 mask;
	int i;
	result = E_OK;

	//IGBT overcurrent verify
	if(mtype.igbt == 1){
		if(fb.data.mt181.igbt == 0xff) {
			message("IGBT over current!\n");
			return E_OOPS;
		}
	}

	//verify uP output
	for(i=0; i<PSVI_CH_UP_NR; i++)
	{
		mask = (1<<i);
		if(fb.data.mt181.up & mask) {
			message("uP fault:\n");
			message("%s open/short fault!!!\n", get_ch_name(i+PSVI_CH_UP_OFFSET));
			if(i!=3) /*J1-12 SVS BYPASS*/
			result = E_OOPS;
		}
	}
	if(result != E_OK) return result;


	//verify PSVI
	result = Verify_PSVIFaultTest(fb.data.mt181.psvi);
	return result;
}

#define RUN_FAULT_BYTE_TEST_DELAY		1000 // first time delay 1S
ERROR_CODE VerifyFaultByte(void)
{
	ERROR_CODE result;
	UINT16 freq;

	result = E_OK;

	//init fault bytes
	if ( fb.flag == 0){
		memset(&fb, '\0', sizeof(fb));
		fb.flag = 0xff;
		fb.dwFaultByteTimer = time_get(RUN_FAULT_BYTE_TEST_DELAY);
		return result;
	}

	if ( time_left(fb.dwFaultByteTimer) > 0)
		return result;

	fb.dwFaultByteTimer = time_get(RUN_FAULT_BYTE_TEST_DUTY); // Update time.

	//read the fault bytes
	result = ReadFaultByte();
	if (result != E_OK) return result;

	//verify according to the model type
	message("T=%dS\n",(NOW()/1000));
	if(mtype.model == MODEL_MT221 ) result = MT221_VerifyFaultByte();
	if(mtype.model == MODEL_MT181 ) result = MT181_VerifyFaultByte();
	return result;
}

/* START FUNCTION DESCRIPTION *************************************************
CyclingTest
*.
*.Syntax: void CyclingTest(void)
*.
*.Description: To do the ECM CyclingTest
*.
*.Parameters
*.	error		error code
*.External References:
*.Return Value:
*.
*.Notes:
*.
*.*****************************************************************************
*.Revision:
*.
*.DATE	  USERID  DESCRIPTION
*.-------  ------  ------------------------------------------------------------
*.24Nov08	 fzwf47	> Initial release.
*.
END DESCRIPTION ***************************************************************/
#define MT221SS_SEQ_ADDR		(0X100007UL)
#define MT221SS_SEQ_NUM_SIZE			(1)

void CyclingTest(void)
{
	ERROR_CODE result;
	UINT32 fbcnt; //fault bytes error count
	UINT32 tpcnt; //tach pulse error count
	UINT32 timeout;
	UINT32 time;

	if(fail()) return;
	message("#Output Cycling Test Start... \n");
	if(mtype.mt221ss == 1){
		char temp;
		result = Read_Memory(MT221SS_SEQ_ADDR, &temp, MT221SS_SEQ_NUM_SIZE);
		if( result != E_OK ) ERROR_RETURN(SN_FAIL, "Get Sequency Number");
		if(temp != '0'){
            message("#Output Cycling Test Pass Due To the seq num is not 0...\n#seq  num is %c .... \n",temp);
			return;
		}
	}

	fbcnt = 0; //fault bytes error counter
	tpcnt = 0; //tach pulse error counter
	time = NOW();
	timeout = CND_PERIOD - time; //because we'll do the same test after cycling test

	message("Enable Waveform Capture Channel 1\n");
	ICEnblDsbl(IC_CHAN1, IC_ENABLE); // Enable input capture channel 1. Only required for SDM

	message("Start output cycling\n");
	Execute_Cycling(mtype);
	Delay10ms(10); //delay 100mS to avoid first time verify fail(no pulse fail)

	while(time < timeout)
	{
		//get current time
		time = NOW();

#ifdef CONFIG_SERV_WEBPAGE_INSIDE_TEST
		//serv the webpage
		http_handler();
#endif

		//toggle the run led
		ToggleRun();
#if 0
		//monitor tach pulse
		result = VerifyDUTSignal1();
		if ( result != E_OK ) tpcnt ++;
#endif
		//monitor fault byte
		result = VerifyFaultByte();
		if (result != E_OK) fbcnt ++;

		//if(MUMode == FALSE){
			//over thresthold?
			if( fbcnt > FAULT_BYTES_THRESTHOLD){
				ERROR(FB_FAIL, "Test Abort: Fault Bytes"); //now time will be set automatically by ERROR()
				break;
			}
#if 0
			if( tpcnt > TACH_PULSE_THRESTHOLD){
				ERROR(PULSE_FAIL, "Test Abort: TACH Pulse");//now time will be set automatically by ERROR()
				break;
			}
#endif
		//}
	}
	message("Disable Waveform Capture Channel 1\n");
	ICEnblDsbl(IC_CHAN1, IC_DISABLE); // Disable input capture channel 1.

	message("Output Cycling Test Finish\n");

	//reboot
	NestReboot();

	return;
}

#define PCB_SEQ_ADDR			(model.pcb_seq_addr)
#define SEQ_NUM_SIZE			(8)
#define BASE_MODEL_TYPE_ADDR	(model.mtype_addr)
#define BASE_MODEL_TYPE_SIZE	((UINT8)0x04)

#define PSV_AMB_ADDR			(model.psv_amb_addr)
#define PSV_CND_ADDR			(model.psv_cnd_addr)

/* START FUNCTION DESCRIPTION *************************************************
TestStart
*.
*.Syntax: void TestStart(void)
*.
*.Description: wait for DUT plug in and prepare for the tests
*.
*.Parameters
*.	error		error code
*.	fis.bUseFIS		FIS on/off switch
*.	UserBlock	global configuration
*.External References:
*.Return Value:
*.	error		error code
*.Notes:
*.
*.*****************************************************************************
*.Revision:
*.
*.DATE	  USERID  DESCRIPTION
*.-------  ------  ------------------------------------------------------------
*.24Nov08	 fzwf47	> Initial release.
*.
END DESCRIPTION ***************************************************************/
void TestStart(void)
{
	ERROR_CODE result;
	UINT8 buffer[4];
	UINT8 psv, psv_save;
	int cnt;

	//all led off
	NestLightControl(ALL_OFF);
   /*remove below redundant actions*/
//	RELAY_IGBT_SET(0);
//	RELAY_ETC_SET(0);
//	RELAY_MT221_MT601_SET(0);
//	RELAY_MT601_SET(1);
//	RELAY_MT221_SET(0);

	//wait for module insertion and serv the webpage
	message("Waiting for Module Insertion ... ");
	do {
		while (PartNotPresent(1)) {
			nest_update();
		}
	} while(!PartPresent(500));

	message("Detected!!!\n");

	message_clear();
	message("#Test Start\n");
	nest_time_init();

	//init global var
	InitError();
	fault_bytes_init(fb);

	//do ops before power on
	message("pre-set relay to low load status\n");
	mtype.igbt = 0;
	RELAY_IGBT_SET(0);
	mtype.iac = 1;
	RELAY_ETC_SET(0);
	mtype.model = MODEL_MT221;
	RELAY_MT221_MT601_SET(1); //mt22.1 or mt18.1?
	RELAY_MT601_SET(0);//set load to be MT22.1
	RELAY_MT221_SET(1);//Set load to be MT22.1

	//power on the dut
	NestPowerOn();

	//breq
	//fisBREQ(); //send FIS BREQ(build request) message

	//init CAN
	result = mcamOSInit(DW_CAN1, CAN_KBAUD);
	if( result != E_OK) ERROR_RETURN(CAN_FAIL, "Init CAN");
	message("Init CAN ... pass\n");

	//print testability s/w version
	//try mt22.1?
	memcpy(&model, &model_mt221, sizeof(model));
	message("BASIC ");
	result = PrintSoftwareVersion();
	if (result != E_OK){
		//try mt18.1?
		mtype.model = MODEL_MT181;
		RELAY_MT221_MT601_SET(0);//Set load to be MT18.1
		RELAY_MT221_SET(0);//Set load to be MT18.1
		memcpy(&model, &model_mt181, sizeof(model));
		result = PrintSoftwareVersion();
	}
	if(result != E_OK) ERROR_RETURN(CAN_FAIL, "Print S/W Version");

	//Get DUT Serial Number
	memset(fis.__id, '\0', sizeof(fis.__id));
	result = Read_Memory(PCB_SEQ_ADDR, fis.__id, SEQ_NUM_SIZE);
	if( result != E_OK ) ERROR_RETURN(SN_FAIL, "Get Sequency Number");
	message("DUT S/N: ");
	for(cnt = 0; cnt < SEQ_NUM_SIZE; cnt ++) {
		Base_Model_Nr [cnt] = fis.__id [cnt];
		message("%c",fis.__id[cnt]);
	}
	message("\n");

#if 0 //write amb psv
	psv = 0x00;
	result = Write_Memory(PSV_AMB_ADDR, &psv, 1);
	if(result == E_OK)
		message("Write AMB PSV Pass\n");
#endif

	//read and check AMB PSV
	result = Read_Memory(PSV_AMB_ADDR, &psv, 1);
	if( (result != E_OK) || (psv&(1<<7)) ) ERROR_RETURN(PSV_FAIL, "Read AMB PSV");
	message("Read AMB PSV ... pass\n");

	//write PSV(fail)
	psv_save = psv = ((1<<7)|(nest_info_get() -> id_base))&0xff;
	result = Write_Memory(PSV_CND_ADDR, &psv, 1);
	if( result != E_OK ) ERROR_RETURN(PSV_FAIL, "Write CND PSV");

	//verify PSV
	result = Read_Memory(PSV_CND_ADDR, &psv, 1);
	if( result != E_OK ) ERROR_RETURN(PSV_FAIL, "Read CND PSV");
	if ( psv !=  psv_save ) ERROR_RETURN(PSV_FAIL, "Verify CND PSV");
	message("Preset PSV_CND_FAIL ... pass\n");

	//read base model type from addr 0x10_0018 and ..19
	result = Read_Memory(BASE_MODEL_TYPE_ADDR, buffer, BASE_MODEL_TYPE_SIZE);
	if (result != E_OK) ERROR_RETURN(MTYPE_FAIL, "Read Base Model");
	if (mtype.model == MODEL_MT221) {
	/*  Base model identification bytes:
		0018-msb	0xA-> IGBT, 0X5->EST
		0018-lsb		0xA-> ETC, 0X5-> IAC
		0019		0XFF -> Paried(2EST/IGBT), 0x00 -> Sequencial(4EST/IGBT)
		001B		0xFF -> MT22.1/MT22.1.1,    0x00 -> MT60.1

	*/
		mtype.igbt = (BOOL)((buffer[0]&0xF0) == 0xA0);
		mtype.iac  = (BOOL)((buffer[0]&0x0F) == 0x05);
		mtype.par = (BOOL)((buffer[1]&0xFF) == 0xFF);
		mtype.mt601 = (BOOL)((buffer[3]&0xFF) == 0x00);
		mtype.mt221ss = (BOOL)((buffer[3]&0xFF) == 0x03);
	}
	else {
		/* Base model identification bytes:
			0018	0X2A -> 2IGBT, 0X3A -> 3IGBT, 0X45 -> 4EST
		*/
		mtype.igbt = (BOOL)((buffer[0]&0x0F) == 0x0A);
		mtype.iac = 1;
		mtype.par = ((buffer[0]&0xF0) >>4);
		mtype.mt601 = 0;
		//mt18.1 doesn't support id byte 1
		buffer[1] = 0x00;
		if ((buffer[3]) == 0x01){
			memcpy(&model, &model_mt2211ss, sizeof(model));
			result = PrintSoftwareVersion();
			if(result != E_OK)
				ERROR_RETURN(CAN_FAIL, "Print S/W Version");
		}
	}

	message("Model Type(%02x,%02x,%02x): ",buffer[0],buffer[1],buffer[3]);
	PRINT_MODEL_TYPE();

	message("Set relay according to the model type\n");
	RELAY_IGBT_SET(mtype.igbt);
	RELAY_ETC_SET(!mtype.iac);
	if(mtype.model == MODEL_MT221){
		if(mtype.mt601 == 1){
			RELAY_MT601_SET(1);	//set relay for mt601
			RELAY_MT221_SET(0);	//set relay for mt601
		}
	}

	return;
}

#define FB_ADDR		(model.fault_byte_addr)
#define NEC_ADDR	(model.error_code_addr) //NEST ERROR CODE

/* START FUNCTION DESCRIPTION *************************************************
TestStop
*.
*.Syntax: void TestStop(void)
*.
*.Description: finish the test and report test result
*.
*.Parameters
*.	error		error code
*.	fis.bUseFIS		FIS on/off switch
*.	UserBlock	global configuration
*.External References:
*.Return Value:
*.	error		error code
*.
*.Notes:
*.
*.*****************************************************************************
*.Revision:
*.
*.DATE	  USERID  DESCRIPTION
*.-------  ------  ------------------------------------------------------------
*.24Nov08	 fzwf47	> Initial release.
*.
END DESCRIPTION ***************************************************************/
void TestStop(void)
{
	ERROR_CODE result;
	UINT8 psv, psv_save;
	UINT8 nec, nec_save;
	int min;

	message("#Test Stop\n");

	//store fault bytes back to dut
	if(mtype.model == MODEL_MT221) {
		result = Write_Memory(FB_ADDR, fb.data.buf, sizeof(fb.data.mt221));
	}
	else {
		result = Write_Memory(FB_ADDR, fb.data.buf, sizeof(fb.data.mt181));
	}
	if( result != E_OK ) ERROR(MISC_FAIL, "Write back Fault Bytes");

	//set nest error code byte
	min = nest_error_get() -> time / 60000;
	if(min > 15) min = 15;
	nec_save = nec = ((min&0x0f)<<4) | (nest_error_get() -> type & 0x0f);
	message("Set Nest Error Code<0x%02x> to addr 0x%6lx ... \n", nec, NEC_ADDR);
	result = Write_Memory(NEC_ADDR, &nec, 1);
	if( result != E_OK ) ERROR(NEC_FAIL, "Write NEC");

	result = Read_Memory(NEC_ADDR, &nec, 1);
	if( result != E_OK ) ERROR(NEC_FAIL, "Read NEC");
	if ( nec !=  nec_save ) ERROR(PSV_FAIL, "Verify NEC");

	//write psv
	if(pass()) {
		psv_save = psv = ((0<<7)|nest_info_get() -> id_base)&0xff;
		result = Write_Memory(PSV_CND_ADDR, &psv, 1);
		if( result != E_OK ) ERROR(PSV_FAIL, "Write PSV");
	}

	//verify PSV
	if(pass()) {
		result = Read_Memory(PSV_CND_ADDR, &psv, 1);
		if( result != E_OK ) ERROR(PSV_FAIL, "Read PSV");
		if ( psv !=  psv_save ) ERROR(PSV_FAIL, "Verify PSV");

		message("Set PSV_CND_PASS flag ... pass\n");
	}

	//send FIS BCMP(build complete) message
	//fisBCMP();

	//test finish, power down
	NestPowerOff();	// Ensure signal, power, and lights out.
	if ( pass() ) NestLightControl(PASS_CMPLT_ON);
	else NestLightControl(FAIL_ABORT_ON);
	Delay10ms(1);//100ms is okay
	message("CONDITIONING COMPLETE\n");

	//toggle Abort light if nest error.
#ifdef CONFIG_NEST_AUTORESTART
	time_t timer_restart = time_get(3*60*1000);
#endif
	while (!PartNotPresent(0)) { // Wait for part to be removed.
		Flash_Err_Code(nest_error_get() -> type);
#ifdef CONFIG_NEST_AUTORESTART
		if(time_left(timer_restart) < 0) {
			if(pass())
				break;
		}
#endif
	}

	// Turn all indicators off.
	NestLightControl(ALL_OFF);

#if 0
	HReset(); // Reset to restart.
#endif

	return;
}


/* START FUNCTION DESCRIPTION *************************************************
main													  <MT80.c>
*.
*.Syntax: void main(void)
*.
*.Description: This is the "main" function for the conditioning application.
*.
*.Parameters ( (I)nput, (O)utput ): void
*.
*.External References:
*.	 InitCNCB
*.	 NestLightControl
*.
*.Return Value: void
*.
*.Notes:
*.
*.*****************************************************************************
*.Revision:
*.
*.DATE	  USERID  DESCRIPTION
*.-------  ------  ------------------------------------------------------------
*.07May06  zzrq7q  > Initial release.
*.27Nov08  fzwf47  > modified for mt22.1
*.
END DESCRIPTION ***************************************************************/
void main(void)
{
	nest_init();
	NestPowerOff();
	nest_message("\nPower Conditioning - MT22.1\n");
	nest_message("IAR C Version v%x.%x, Compile Date: %s,%s\n", (__VER__ >> 24),((__VER__ >> 12) & 0xfff),  __TIME__, __DATE__);
	nest_message("nest ID : MT22.1SS-%03d\n",(*nest_info_get()).id_base);


	while(1){
		TestStart();
		RAMTest();
		RAMDnld();
		CyclingTest();
		//RAMTest();
		TestStop();
	}
} // main

/*******************************************************************************
		 Conditioning Nest Control Board Description

The Conditioning Nest Control Board (CNCB) is an electronic circuit board
designed for controlling a conditioning nest. Conditioning nests are used
to power, load, and control an automotive electronics module (AEM). This
process is referred to as "conditioning". The purpose of conditioning is
to stress an AEM to precipitate latent defects. Conditioning nest function
as "stand alone" units however multiple units are usually mounted in a
common housing.

The CNCB is used to sequence power, apply stimulus, and communicate with the
AEM as needed to perform conditioning operations. The sub functions of the
CNCN are as follows: power supply, processor core, nest control circuits,
serial communications circuits, signal control circuits, and auxiliary
processor. Descriptions for each sub-function follow below.

A typical conditioning scenario is as follows: A typical conditioning
nest consists of a physical interface to the AEM, power relays for
controlling power to the AEM, indicator lights (running, complete & abort),
loads for the AEM outputs, and a power supply. When an AEM is placed
in the nest the CNCB detects this and controls the power relays to apply
power to the AEM. CNCB outputs are used to stimulate the AEM as needed
so the AEM can cycle it's outputs. The CNCB then communicates with the
AEM via a serial interface to download information and command the AEM
to enter a mode where it cycles it's outputs on and off. The CNCB will keep
the AEM in output cycling mode for a set time period. While the AEM is in
output cycling mode the CNCB toggles the Running indicator light to show
output cycling is running and monitors an AEM output to verify the AEM stays
in output cycling mode. When the time period has expired the CNCB commands
the AEM to exit the output cycling mode and writes status information into
the AEM memory. The CNCB then removes power from the AEM, turns on the Complete
or Abort indicator light and waits for the AEM to be removed. When the AEM is
removed the indicator light is turned off and the CNCB waits for another
AEM to be installed.

The CNCB is powered by a DC input voltage between 8 and 20 volts. The
power supply is a two stage DC-to-DC converter that converts the input
voltage into DC outputs of 5V and 3.3V. The first stage is a 5V switching
regulator and the second stage is a 3.3V linear regulator whose input
is the 5V from the first stage. The input voltage and 5V's are used
through the CNCB while the 3.3V's is used only for the processor core.

The processor core consists of a plug in module and some logic interface
circuits. The plug in module is an RCM3200 Rabbit Core Module (RCM),
from Z-World, which contains a microprocessor, memory and an Ethernet
interface. The major features of the RCM3200 core module are as follows:
8 bit, 44MHz microprocessor; 52 channels of I/O; 512K of FLASH memory;
512K of SRAM for program; 256K of SRAM for data; 10/100 Base-T Ethernet
port. The "C" language development tools for the core module include a
symbolic debugger and function library including a network protocol
(TCP/IP) stack. The logic interface circuits consist of logic level
translators, reset logic, and the nest ID switch (8 pos. DIP switch).

The nest control circuits interface the RCM to the nest hardware
allowing the RCM to control nest functions. Five switches (low side driver)
are used to control nest power relays ( BAT & IGN) and indicator lights
(Running, Complete/Fail, Abort). An input interface is used to detect
when an AEM is installed in the nest. This input is also used to prevent
closing power relays until an AEM is installed.

The serial communication circuits interface the RCM to the controller
and transceiver hardware for several serial communications protocols.
These protocols are used to communicate with an AEM. The serial protocol
physical layers available are as follows: dual wire CAN (MCP2515 & MCP2551
chip set), single wire CAN (MCP2515 & MC33897 chip set), Keyword or ISO9141
(MCP201 transceiver), LIN (MCP201 transceiver), and CLASS 2 or J1850 (DLC08
controller/transceiver). The RCM communicates via SPI to the MCP2515
(CAN) and DLC08 (Class 2) controllers and via a UART to the MCP201
(Keyword/LIN).

The signal control circuits provide an interface between the RCM and
discrete AEM signals. These circuits provide signal conditioning,
level translation and buffering. There are two input and six
outputs from the CNCB. The two inputs are intended for monitoring
a triangle waveform signal from the AEM and measuring pulse width
and period. The six outputs can stimulate the AEM with a high or
low level as a static level or a triangle waveform. A low level is
ground level and a high level is normally 5V or the VBAT voltage but
could be other values (determined by resistor values) between ground
and VBAT.

The Auxiliary Processor (AP) is a PIC18F1320 (PIC) microprocessor,
which provides additional I/O signals for interfacing to the AEM.
The RCM and PIC communicate via a UART serial interface and can be
used in a master (RCM) slave (PIC) arrangement. The initial programming
of the PIC can include a bootloader that allows it to be reprogrammed
by the RCM. The AP provides the CNCB with six outputs and three inputs.
The PIC includes an A/D converter and can be programmed to perform a
wide variety of signal generation and processing functions.


*******************************************************************************/

