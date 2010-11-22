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
* 10Oct10  cong.chen > revised for mt92
* 22Nov10 fzwf47 > revised for arm platform
END DESCRIPTION ***************************************************************/

#include "nest.h"
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

#define MEMORYR_ADDR ((UINT32)0X8017001C)
#define READDATA_ADDR ((UINT32)0XD000880C)
ERROR_CODE Read_Memory(UINT32 addr, UINT8* pdata, int len)
{
	ERROR_CODE result ;
	UINT8 buffer[5];
	buffer[0]=buffer[1]=buffer[2]=0x00;buffer[3] =(UINT8)len;
	result = mcamOSDownload(DW_CAN1, D4_ADDR, buffer, 4, CAN_TIMEOUT);
	if (result != E_OK) return result;

	buffer[0] = (addr>>24)&0xff;
	buffer[1] = (addr>>16)&0xff;
	buffer[2] = (addr>> 8)&0xff;
	buffer[3] = (addr>> 0)&0xff;
	result = mcamOSDownload(DW_CAN1, A4_ADDR, buffer, 4, CAN_TIMEOUT);
	if (result != E_OK) return result;

	result = mcamOSExecute(DW_CAN1, MEMORYR_ADDR, CAN_TIMEOUT);
	if(result != E_OK) return result;

	result = mcamOSUpload(DW_CAN1, pdata, READDATA_ADDR, len, CAN_TIMEOUT);
	if (result != E_OK) return result;

	return result;
}


#define WRITEDATA_ADDR ((UINT32)0XD000871C)
#define MEMORYW_ADDR ((UINT32)0X80170004)
ERROR_CODE Write_Memory(UINT32 addr, UINT8* pdata, int len)
{
	ERROR_CODE result ;
	UINT8 buffer[5];

	buffer[3] =(UINT8)len;
	result = mcamOSDownload(DW_CAN1, D4_ADDR, buffer, 4, CAN_TIMEOUT);
	if (result != E_OK) return result;

	buffer[0] = (addr>>24)&0xff;
	buffer[1] = (addr>>16)&0xff;
	buffer[2] = (addr>> 8)&0xff;
	buffer[3] = (addr>> 0)&0xff;
	result = mcamOSDownload(DW_CAN1, A4_ADDR, buffer, 4, CAN_TIMEOUT);
	if (result != E_OK) return result;

	result = mcamOSDownload(DW_CAN1, WRITEDATA_ADDR, pdata, len, CAN_TIMEOUT);
	if (result != E_OK) return result;

	result = mcamOSExecute(DW_CAN1, MEMORYW_ADDR, CAN_TIMEOUT);
	if(result != E_OK) return result;

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
	UINT8 i;
	result = mcamOSExecute(DW_CAN1, VERSION_ADDR, CAN_TIMEOUT);
	if(result != E_OK) return result;
	result = mcamOSUpload(DW_CAN1, buffer, VERDATA_ADDR, 8, CAN_TIMEOUT);
	if (result != E_OK) return result;
	result = mcamOSUpload(DW_CAN1, buffer+8, VERDATA_ADDR+8, 8, CAN_TIMEOUT);
	if (result != E_OK) return result;
	result = mcamOSUpload(DW_CAN1, buffer+16, VERDATA_ADDR+16, 8, CAN_TIMEOUT);
	if (result != E_OK) return result;
	message("TESTABILITY  VERSION: %s\n",buffer);
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
	UINT8 i,j;
	UINT8 val,n;
	if(dwCollectFaultTimer == 0) {
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
		}
		else return;
		result2 = mcamOSUpload(DW_CAN1, Data_Pntr+2, VSEP_ADDR, 8, CAN_TIMEOUT);
		if(result2 == E_OK) {
			for(j=2;j<8;j++) {
				for(i=0;i<7;i=i+2) {
					val=(*(Data_Pntr+j)<<i)&0xc0;
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
		}
		else return;
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
		}
		else return;
		result2 = mcamOSUpload(DW_CAN1, Data_Pntr+24, EMCD_ADDR, 4, CAN_TIMEOUT);
		if(result2 == E_OK) {
			if(*(Data_Pntr+25)&0x02){result=E_ERR;message("DIFO:OPEN6\n");}
			if(*(Data_Pntr+25)&0x01){result=E_ERR;message("DIFO:OPEN5\n");}
			if(*(Data_Pntr+26)&0x02){result=E_ERR;message("DIFO:OPEN4\n");}
			if(*(Data_Pntr+26)&0x01){result=E_ERR;message("DIFO:OPEN3\n");}
			if(*(Data_Pntr+27)&0x02){result=E_ERR;message("DIFO:OPEN2\n");}
			if(*(Data_Pntr+27)&0x01){result=E_ERR;message("DIFO:OPEN1\n");}
		}
		else return;
		Fault_Data[0]=*Data_Pntr;
		for(i=2;i<11;i++)
			Fault_Data[i-1]=*(Data_Pntr+i);
		for(i=24;i<28;i++)
			Fault_Data[i-14]=*(Data_Pntr+i);
		dwCollectFaultTimer = time_get(RUN_FAULT_BYTE_TEST_DUTY);
	}
	return result;
}


#define CYC_ADDR ((UINT32)0x80170040)
ERROR_CODE Execute_Cycling()
{
	ERROR_CODE result ;
	UINT8 buffer[4];
	buffer[0]=0x00;buffer[1]=0x00;buffer[2]=0x00;buffer[3]=0x04;
	result = mcamOSDownload(DW_CAN1, D4_ADDR, buffer, 4, CAN_TIMEOUT);
	if(result != E_OK) return result;
	result = mcamOSExecute(DW_CAN1, CYC_ADDR, CAN_TIMEOUT);
	if(result != E_OK) return result;
	result = mcamOSUpload(DW_CAN1, buffer, D2_ADDR, 4, CAN_TIMEOUT);
	if(result != E_OK) return result;
	if(buffer[3]!=0x04) result=E_ERR;
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
void CyclingTest(void)
{
	ERROR_CODE result;
	UINT32 fbcnt; //fault bytes error count
	UINT32 tpcnt; //tach pulse error count
	UINT32 timeout;
	UINT32 time;

	if(fail()) return;
	message("#Output Cycling Test Start... \n");

	fbcnt = 0; //fault bytes error counter
	tpcnt = 0; //tach pulse error counter
	time = NOW();
	timeout = CND_PERIOD - time; //because we'll do the same test after cycling test

	message("Enable Waveform Capture Channel 1\n");
	ICEnblDsbl(IC_CHAN1, IC_ENABLE); // Enable input capture channel 1. Only required for SDM

	result=Execute_Cycling();
	if (result != E_OK) return;
	Delay10ms(10); //delay 100mS to avoid first time verify fail(no pulse fail)
	message("Start output cycling\n");
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

		//monitor tach pulse
		//result = VerifyDUTSignal1();
		//if ( result != E_OK ) tpcnt ++;

		//monitor fault byte
		result =  Collect_Verify_Fault();
		if (result != E_OK) fbcnt ++;


		if( fbcnt > FAULT_BYTES_THRESTHOLD){
			ERROR(FB_FAIL, "Test Abort: Fault Bytes"); //now time will be set automatically by ERROR()
			break;
		}

		if( tpcnt > TACH_PULSE_THRESTHOLD){
			ERROR(PULSE_FAIL, "Test Abort: TACH Pulse");//now time will be set automatically by ERROR()
			break;
	  	}
	}
	message("Disable Waveform Capture Channel 1\n");
	ICEnblDsbl(IC_CHAN1, IC_DISABLE); // Disable input capture channel 1.

	message("Output Cycling Test Finish\n");

	//reboot
	NestReboot();

	return;
}



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
	UINT8 buffer[8];
	UINT8 psv, psv_save;
	int cnt;

	//all led off
	NestLightControl(ALL_OFF);


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

	//do ops before power on
	message("pre-set relay to low load status\n");

	//DK269089 is 3 cyc-clind  DK269088 is 4 cyc-clind
	result = Read_Memory(BASE_MODEL_TYPE_ADDR, buffer, BASE_MODEL_TYPE_SIZE);
	if (result != E_OK) ERROR_RETURN(MTYPE_FAIL, "Read Base Model");
	if(buffer[7]==9)
	{	RELAY_SIG1_SET(1);
		RELAY_SIG2_SET(1);
		RELAY_SIG3_SET(1);
	}
	else if(buffer[7]==8)
	{	RELAY_SIG1_SET(0);
		RELAY_SIG2_SET(0);
		RELAY_SIG3_SET(0);
	}

	//power on the dut
	NestPowerOn();

	//breq
	//fisBREQ(); //send FIS BREQ(build request) message

	//init CAN
	result = mcamOSInit(DW_CAN1, CAN_KBAUD);
	if( result != E_OK) ERROR_RETURN(CAN_FAIL, "Init CAN");
	message("Init CAN ... pass\n");

	message("BASIC ");
	result = PrintSoftwareVersion();
   if(result != E_OK) ERROR_RETURN(CAN_FAIL, "Print S/W Version");
	//Get DUT Serial Number

	memset(fis.__id, '\0', sizeof(fis.__id));
	result = Read_Memory(PCB_SEQ_ADDR, fis.__id, SEQ_NUM_SIZE);
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

	message("#Test Stop\n");

	//store fault bytes back to dut
	result = Write_Memory(FB_ADDR, Fault_Data, 14);
	if( result != E_OK ) ERROR(MISC_FAIL, "Write back Fault Bytes");

	//set nest error code byte   NEC_ADDR?
  	/*if(error.time > 15) error.time = 15;
	nec_save = nec = (((error.time&0x0f)<<4) | (error.type & 0x0f));
	message("Set Nest Error Code<0x%02x> to addr 0x%6lx ... \n", nec, NEC_ADDR);
  	result = Write_Memory(NEC_ADDR, &nec, 1);
	if( result != E_OK ) ERROR(NEC_FAIL, "Write NEC");

  	result = Read_Memory(NEC_ADDR, &nec, 1);
	if( result != E_OK ) ERROR(NEC_FAIL, "Read NEC");
	if ( nec !=  nec_save ) ERROR(PSV_FAIL, "Verify NEC"); */

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
	nest_message("\nPower Conditioning - MT22.1\n");
	nest_message("IAR C Version v%x.%x, Compile Date: %s,%s\n", (__VER__ >> 24),((__VER__ >> 12) & 0xfff),  __TIME__, __DATE__);

	while(1){
		TestStart();
	  	CyclingTest();
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

