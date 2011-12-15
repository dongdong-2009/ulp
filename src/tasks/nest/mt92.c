/* START LIBRARY DESCRIPTION ***************************************************
 * init version: miaofng, cc
 * modified : David
END DESCRIPTION ***************************************************************/
#include "lib/nest.h"
#include "obsolete/obsolete.h"
#include "mt92.h"

fis_data_t fis;
unsigned char  Fault_Data[256];
unsigned char  *Data_Pntr;

//vsep faultbyte
const char *xVsepChName[] = {
	"A57 LEGR",
	"A59 VVL",
	"A44 VCPEXOUT",
	"A58 VCPEIOUT",
	"A5 ESTCOILD",
	"A3 ESTCOILC",
	"A20 ESTCOILB",
	"A4 ESTCOILA",

	"A28 WASTE_GATE",
	"J1-07 VCIS1",
	"J1-25 TURBOOUT",
	"J1-08 CVV",
	"NONE",
	"NONE",
	"NONE",
	"NONE",

	"J1-43 ACRELAY",
	"J1-22 FANLO",
	"J1-16 FANHI",
	"A14 CCP",
	"K73 O2HTRA2S2",
	"K51 O2HTRA1S2",
	"J1-63 FCOUT",
	"J1-13 TACHOUT",

	"NONE",
	"NONE",
	"J1-61 SPAREDISCRETE",
	"J1-15 TPSPWMOUT",
	"J1-80 FLGAUGE",
	"J1-14 ALTPMOUT",
	"CRUZENLAMP",
	"WSSFB"
};

//local funcitons declairation for nest power control
static void NestPowerOn(void);
static void NestReboot(void);
static void NestPowerOff(void);

static void NestPowerOn(void)
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

static void NestReboot(void)
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

static void NestPowerOff(void)
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

#define MEMORYR_ADDR  ((UINT32)0x8017001C)
#define READDATA_ADDR ((UINT32)0xD000880C)
ERROR_CODE Read_Memory(UINT32 addr, UINT8* pdata, int len)
{
	ERROR_CODE result;
	UINT8 buffer[5];

	memset(buffer + 1, 0x00, 3);
	buffer[0] = (UINT8)len;

	result = mcamOSDownload(DW_CAN1, D4_ADDR, buffer, 4, CAN_TIMEOUT);
	if (result != E_OK) return result;

	buffer[0] = (addr >> 0) & 0xff;
	buffer[1] = (addr >> 8) & 0xff;
	buffer[2] = (addr >> 16) & 0xff;
	buffer[3] = (addr >> 24) & 0xff;

	result = mcamOSDownload(DW_CAN1, A4_ADDR, buffer, 4, CAN_TIMEOUT);
	if (result != E_OK) return result;

	result = mcamOSExecute(DW_CAN1, MEMORYR_ADDR, CAN_TIMEOUT);
	if(result != E_OK) return result;

	Delay10ms(40);  //it seems to need delay 300ms for mcamos server executing at least

	result = mcamOSUpload(DW_CAN1, pdata, READDATA_ADDR, len, CAN_TIMEOUT);
	if (result != E_OK) return result;

	return result;
}

#define WRITEDATA_ADDR ((UINT32)0XD000871C)
#define MEMORYW_ADDR   ((UINT32)0X80170004)
ERROR_CODE Write_Memory(UINT32 addr, UINT8* pdata, int len)
{
	ERROR_CODE result;
	UINT8 buffer[5];

	buffer[0] = (UINT8)len;
	memset(buffer + 1, 0x00, 3);
	result = mcamOSDownload(DW_CAN1, D4_ADDR, buffer, 4, CAN_TIMEOUT);
	if (result != E_OK) return result;

	buffer[0] = (addr>> 0)&0xff;
	buffer[1] = (addr>> 8)&0xff;
	buffer[2] = (addr>>16)&0xff;
	buffer[3] = (addr>>24)&0xff;

	result = mcamOSDownload(DW_CAN1, A4_ADDR, buffer, 4, CAN_TIMEOUT);
	if (result != E_OK) return result;

	result = mcamOSDownload(DW_CAN1, WRITEDATA_ADDR, pdata, len, CAN_TIMEOUT);
	if (result != E_OK) return result;

	result = mcamOSExecute(DW_CAN1, MEMORYW_ADDR, CAN_TIMEOUT);
	if(result != E_OK) return result;

	Delay10ms(10);

	result = mcamOSUpload(DW_CAN1, buffer, D2_ADDR, 4, CAN_TIMEOUT);
	if (result != E_OK) return result;
	if(buffer[3] == 0x33) result = E_OK;
	else result = DUT_OOPS;

	return result;
}

#define VERSION_ADDR ((UINT32)0X80170000)
#define VERDATA_ADDR ((UINT32)0XD000880C)
ERROR_CODE PrintSoftwareVersion(void)
{
	ERROR_CODE result ;
	UINT8 buffer[24];
	result = mcamOSExecute(DW_CAN1, VERSION_ADDR, CAN_TIMEOUT);
	if(result != E_OK) return result;

	Delay10ms(10);  //it seems to need delay 100ms for mcamos server executing at least

	result = mcamOSUpload(DW_CAN1, buffer, VERDATA_ADDR, 8, CAN_TIMEOUT);
	if (result != E_OK) return result;
	result = mcamOSUpload(DW_CAN1, buffer + 8, VERDATA_ADDR + 8, 8, CAN_TIMEOUT);
	if (result != E_OK) return result;
	result = mcamOSUpload(DW_CAN1, buffer + 16, VERDATA_ADDR + 16, 8, CAN_TIMEOUT);
	if (result != E_OK) return result;
	message("TESTABILITY  VERSION: %s\n", buffer);
	return result;
}

const char * get_ch_name(UINT8 ch)
{
	return xVsepChName[ch];
}

static const char *Type[] =
{
	"ok",
	"open load",
	"short to ground",
	"short to battery"
};

#define RUN_FAULT_BYTE_TEST_DELAY		1000 // first time delay 1S
#define PHDH_ADDR ((UINT32)0XD0008306)
#define VSEP_ADDR ((UINT32)0XD0008308)
#define EMCD_ADDR ((UINT32)0XD0008310)
#define DIFO_ADDR ((UINT32)0XD000831E)
static time_t dwCollectFaultTimer = 0;
ERROR_CODE  Collect_Verify_Fault(void)
{
	ERROR_CODE result,result2;
	UINT8 i, j, val, n;

	if (dwCollectFaultTimer == 0) {
		//first time
		dwCollectFaultTimer = time_get(RUN_FAULT_BYTE_TEST_DELAY);
		return E_OK;
	}

	Data_Pntr = &Fault_Data[0];
	result = E_OK;
	result2 = E_OK;
	if (time_left(dwCollectFaultTimer) < 0) {
		result2 = mcamOSUpload(DW_CAN1, Data_Pntr, PHDH_ADDR, 1, CAN_TIMEOUT);
		if(result2 == E_OK) {
			if(*Data_Pntr&0x80){result=E_ERR;message("PHDH:ShBat\n");}
			if(*Data_Pntr&0x40){result=E_ERR;message("PHDH:ShGnd\n");}
			if(*Data_Pntr&0x20){result=E_ERR;message("PHDH:OpenFlt\n");}
		} else {
			return result2;
		}

		result2 = mcamOSUpload(DW_CAN1, Data_Pntr+2, VSEP_ADDR, 8, CAN_TIMEOUT);
		if (result2 == E_OK) {
			for (j = 2; j < 8; j++) {
				for (i = 0; i < 7; i = i + 2) {
					val = (*(Data_Pntr+j)<<i)&0xc0;
					switch(val){
					case 0x00:
						result=E_OK;
						break;
					case 0x40:
						result=E_ERR;
						n=1;
						break;
					case 0x80:
						result=E_ERR;
						n=2;
						break;
					case 0xc0:
						result=E_ERR;
						n=3;
						break;
					}
					if(result!=E_OK)
						message("VSEP:%s %s\n" ,get_ch_name((j-2)*4+i-i/2),Type[n]);
				}
			}
		} else {
			return result2;
		}

		result2 = mcamOSUpload(DW_CAN1, Data_Pntr+10, EMCD_ADDR, 1, CAN_TIMEOUT);
		if(result2 == E_OK) {
			if(*(Data_Pntr+10)&0x80){result=E_ERR;message("K59 CRUZONLAMP\n");}
			if(*(Data_Pntr+10)&0x40){result=E_ERR;message("K77 IMMOLAMP\n");}
			if(*(Data_Pntr+10)&0x20){result=E_ERR;message("K31 MIL\n");}
			if(*(Data_Pntr+10)&0x10){result=E_ERR;message("K55 FPRLY_HSD\n");}
			if(*(Data_Pntr+10)&0x08){result=E_ERR;message("K56 FPRLY_LSD\n");}
			if(*(Data_Pntr+10)&0x04){result=E_ERR;message("K80 STARTRLY_HSD\n");}
			if(*(Data_Pntr+10)&0x02){result=E_ERR;message("EMCD:INTFAN\n");}
			if(*(Data_Pntr+10)&0x01){result=E_ERR;message("J1-K78 SVS\n");}
		} else {
			return result2;
		}

		result2 = mcamOSUpload(DW_CAN1, Data_Pntr+24, EMCD_ADDR, 4, CAN_TIMEOUT);
		if(result2 == E_OK) {
			if(*(Data_Pntr+25)&0x02){result=E_ERR;message("DIFO:OPEN6\n");}
			if(*(Data_Pntr+25)&0x01){result=E_ERR;message("DIFO:OPEN5\n");}
			if(*(Data_Pntr+26)&0x02){result=E_ERR;message("DIFO:OPEN4\n");}
			if(*(Data_Pntr+26)&0x01){result=E_ERR;message("DIFO:OPEN3\n");}
			if(*(Data_Pntr+27)&0x02){result=E_ERR;message("DIFO:OPEN2\n");}
			if(*(Data_Pntr+27)&0x01){result=E_ERR;message("DIFO:OPEN1\n");}
		} else {
			return result2;
		}

		Fault_Data[0]=*Data_Pntr;
		for (i=2;i<11;i++)
			Fault_Data[i-1] = *(Data_Pntr+i);
		for (i=24;i<28;i++)
			Fault_Data[i-14] = *(Data_Pntr+i);
		dwCollectFaultTimer = time_get(RUN_FAULT_BYTE_TEST_DUTY);
	}
	return result;
}

#define CYC_ADDR ((UINT32)0x80170040)
ERROR_CODE Execute_Cycling()
{
	ERROR_CODE result ;
	UINT8 buffer[4];

	buffer[0]=0x04;
	memset(buffer + 1, 0x00, 3);

	result = mcamOSDownload(DW_CAN1, D4_ADDR, buffer, 1, CAN_TIMEOUT);
	if(result != E_OK) return result;
	result = mcamOSExecute(DW_CAN1, CYC_ADDR, CAN_TIMEOUT); 
	if(result != E_OK) return result;
	result = mcamOSUpload(DW_CAN1, buffer, D2_ADDR, 1, CAN_TIMEOUT);
	if(result != E_OK) return result;

	if(buffer[0] != 0x04)
		result = E_ERR;

	return result;
}

/* START FUNCTION DESCRIPTION *************************************************
*.CyclingTest
*.Syntax: void CyclingTest(void)
*.Description: To do the ECM CyclingTest
END DESCRIPTION ***************************************************************/
void CyclingTest(void)
{
	ERROR_CODE result;
	UINT32 fbcnt; //fault bytes error count
	UINT32 timeout;
	UINT32 time;

	if(fail()) return;
	message("#Output Cycling Test Start... \n");

	fbcnt = 0; //fault bytes error counter
	time = NOW();
	timeout = CND_PERIOD - time; //because we'll do the same test after cycling test

	message("Enable Waveform Capture Channel 1 ...\n");
	ICEnblDsbl(IC_CHAN1, IC_ENABLE); // Enable input capture channel 1. Only required for SDM

	result=Execute_Cycling();
	if (result != E_OK) return;
	Delay10ms(10); //delay 100mS to avoid first time verify fail(no pulse fail)
	message("Start output cycling ...\n");
	while(time < timeout)
	{
		time = NOW();  //get current time
		ToggleRun();   //toggle the run led

		//monitor fault byte
		result =  Collect_Verify_Fault();
		if (result != E_OK) fbcnt ++;
		if( fbcnt > FAULT_BYTES_THRESTHOLD){
			ERROR(FB_FAIL, "Test Abort: Fault Bytes"); //now time will be set automatically by ERROR()
			break;
		}

		//monitor tachometer fault conter
	}
	message("Disable Waveform Capture Channel 1 ...\n");
	ICEnblDsbl(IC_CHAN1, IC_DISABLE); // Disable input capture channel 1.

	message("Output Cycling Test Finish ...\n");

	//reboot
	NestReboot();

	return;
}

/* START FUNCTION DESCRIPTION *************************************************
*.TestStart
*.Syntax: void TestStart(void)
*.Description: wait for DUT plug in and prepare for the tests
END DESCRIPTION ***************************************************************/
void TestStart(void)
{
	ERROR_CODE result;
	UINT8 buffer[8];
	UINT8 psv, psv_save;
	int cnt;

	NestLightControl(ALL_OFF);  //all led off

	//wait for module insertion and serv the webpage
	message("Waiting for Module Insertion ...\n");
	do {
		while (PartNotPresent(1)) {
			nest_update();
		}
	} while(!PartPresent(500));

	message("Detected the DUT ...\n");
	message_clear();
	message("Test Start ...\n");
	nest_time_init();

	InitError();    //init global var
	NestPowerOn();  //power on the dut
	message("Power on the nest .. \n");

	result = mcamOSInit(DW_CAN1, CAN_KBAUD);  //init mcamos module
	if( result != E_OK) ERROR_RETURN(CAN_FAIL, "Init CAN");
	message("Init CAN ... pass\n");

	message("Read Software Version ... \n");
	result = PrintSoftwareVersion();
	if(result != E_OK) ERROR_RETURN(CAN_FAIL, "Print S/W Version");

	message("Read Base Model to pre-set relay to low load status ... \n");  //do ops before power on
	//DK269089 is 3 cyc-clind  DK269088 is 4 cyc-clind
	result = Read_Memory(BASE_MODEL_TYPE_ADDR, buffer, BASE_MODEL_TYPE_SIZE);
	if (result != E_OK) ERROR_RETURN(MTYPE_FAIL, "Read Base Model");
	if (buffer[7] == 9) {
		RELAY_SIG1_SET(1);
		RELAY_SIG2_SET(1);
		RELAY_SIG3_SET(1);
	} else if(buffer[7] == 8) {
		RELAY_SIG1_SET(0);
		RELAY_SIG2_SET(0);
		RELAY_SIG3_SET(0);
	}

	//Get DUT Serial Number
	message("Read DUT Sequence Number ...\n");
	memset(fis.__id, '\0', sizeof(fis.__id));
	result = Read_Memory(PCB_SEQ_ADDR, (UINT8 *)fis.__id, SEQ_NUM_SIZE);
	if( result != E_OK ) ERROR_RETURN(SN_FAIL, "Get Sequency Number");
	message("DUT S/N: ");
	for(cnt = 0; cnt < SEQ_NUM_SIZE; cnt ++) message("%c",fis.__id[cnt]);
	message("\n");

	//read and check AMB PSV
	result = Read_Memory(PSV_AMB_ADDR, &psv, 1);
	if( (result != E_OK) || (psv&(1<<7)) ) ERROR_RETURN(PSV_FAIL, "Read AMB PSV");
	message("Read AMB PSV ... pass\n");

	//write PSV(fail)
	psv_save = psv = ((1<<7)|nest_info_get() -> id_base)&0xff;
	result = Write_Memory(PSV_CND_ADDR, &psv, 1);
	if( result != E_OK ) ERROR_RETURN(PSV_FAIL, "Write CND PSV");

	//verify PSV
	result = Read_Memory(PSV_CND_ADDR, &psv, 1);
	if( result != E_OK ) ERROR_RETURN(PSV_FAIL, "Read CND PSV");
	if ( psv !=  psv_save ) ERROR_RETURN(PSV_FAIL, "Verify CND PSV");
	message("Preset PSV_CND_FAIL ... pass\n");
	return;
}

void TestStop(void)
{
	ERROR_CODE result;
	UINT8 psv, psv_save;
	UINT8 nec, nec_save;

	message("#Test Stop\n");

	//store fault bytes back to dut
	result = Write_Memory(FB_ADDR, Fault_Data, 14);
	if( result != E_OK ) ERROR(MISC_FAIL, "Write back Fault Bytes");

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
	while (!PartNotPresent(0)) { // Wait for part to be removed.
		Flash_Err_Code(nest_error_get() -> type);
	}

	// Turn all indicators off.
	NestLightControl(ALL_OFF);

#if 0
	HReset(); // Reset to restart.
#endif

	return;
}

void main(void)
{
	nest_init();
	nest_message("\nPower Conditioning - MT22.1\n");
	nest_message("IAR C Version v%x.%x, Compile Date: %s,%s\n", (__VER__ >> 24),((__VER__ >> 12) & 0xfff),  __TIME__, __DATE__);

	while(1){
		TestStart();
	  	CyclingTest();
		TestStop();
	}
} // main


#if 1
#include "shell/cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int cmd_mt92_func(int argc, char *argv[])
{
	int addr, len, i, result, temp;
	unsigned char buf[8];

	const char *usage = {
		"mt92 , usage:\n"
		"mt92 pwr on/off, turn on/off the nest power\n"
		"mt92 rm addr len, read len(hex) data from memory of addr(hex)\n"
		"mt92 wm addr data, write a byte(hex) to addr(hex)\n"
		"mt92 crun, start the mt92 cycling test\n"
	};

	if (argc < 2) {
		printf("%s", usage);
		return 0;
	}

	if(argv[1][0] == 'p') {
		if(argv[2][1] == 'n') {
			printf("##Power on##\n");
			//power on the dut
			NestPowerOn();
		} else {
			NestPowerOff();
			printf("##Power off##\n");
		}
	}

	if(argv[1][0] == 'r') {
		sscanf(argv[2], "%x", &addr);
		sscanf(argv[3], "%x", &len);
		result = Read_Memory(addr, buf, len);
		if (result) {
			printf("##Error##\n");
		} else {
			printf("from addr:0x%x read 0x%x bytes\n", addr, len);
			for (i = 0; i < len; i++)
				printf("0x%x ", buf[i]);
		}
	}

	if(argv[1][0] == 'w') {
		sscanf(argv[2], "%x", &addr);
		sscanf(argv[3], "%x", &temp);
		buf[0] = (unsigned char)temp;
		result = Write_Memory(addr, buf, 1);
		if (result) {
			printf("##Error Code is 0x%x##\n", result);
		} else {
			printf("##OK##\n");
		}
	}

	if(argv[1][0] == 'c') {
		result = Execute_Cycling();
		if (result) {
			printf("##Error Code is 0x%x##\n", result);
		} else {
			printf("##OK##\n");
		}
	}

	return 0;
}

const cmd_t cmd_mt92 = {"mt92", cmd_mt92_func, "nest mt92 debug cmd"};
DECLARE_SHELL_CMD(cmd_mt92)
#endif
