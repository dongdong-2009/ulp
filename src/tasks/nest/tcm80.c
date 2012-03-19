/*
* there are three types of mt80 nest in delphi
*	type1, designed by ball system - named as mt80_old
*	type2, designed by linktron before 2009? the same as ball system's nest - named as mt80-old
*	type3, designed by linktron in 2011 - named as mt80
*
* difference between mt80_old & mt80
*	1, add lock coil, controlled by SIG_6(BALL SYSTEM'S BUG NAME SIG_4)
*	2, old SIG_6 move to SIG_5(to switch 0: LOAD37  8Ohm + 1mH > JMP5 E61  & 1: LOAD 9 30mH + 53Ohm > JMP4 E61, DUT PIN E67 DC_MOTOR/IAC)
*	3, add fixture id cpu
*/
#include "lib/nest.h"
#include "chips/tc1762.h"
#include "chips/csd.h"
#include "chips/emcd.h"
#include "priv/mcamos.h"
#include "obsolete/obsolete.h"

#define CND_PERIOD	15 //unit: miniutes

// Defines for CAN routines.
#define CAN_TIMEOUT		(100)	// milli-seconds.
#define CAN_MSG_DLY		(10)	  // 10 * 100us = 1ms intermessage delay.
#define CAN_KBAUD		(500)

#define DFT_OP_TIMEOUT			((UINT32)100)//unit ms

#define INBOX_ADDR		(0XD000871CUL)
#define OUTBOX_ADDR		(0XD000880CUL)
#define REGD2_ADDR		(0XD0008800UL)
#define REGD4_ADDR		(0XD0008700UL)
#define REGA4_ADDR		(0XD0008710UL)
#define REGD5_ADDR		(0XD0008704UL)
#define FB_ADDR			(0XD0008300UL)

#define CHECK_SWV_CMD	(0X80170000UL)
#define CHECK_MODEL_CMD	(0X80170020UL)
#define SET_MODEL_CMD	(0X80170038UL)
#define MEMORY_READ_CMD	(0X8017001CUL)
#define ERASE_FLASH_CMD	(0X80170008UL)
#define WRITE_FLASH_CMD	(0X80170004UL)

#define MFGDAT_ADDR		(0X8001C000UL)  //MANUFACTURING_READ_LOCATION, 0x8001C000

//mfg data structure
struct mfg_data_s {
	char date[3];//0x8000 - 0x8002
	char year; //0x8003 last digit of year
	char sn[4]; //0x8004 - 0x8007 sequency nr
	char mfg_code; //0x8008 manufacturing code
	char bmr[8]; //0x8009 - 0x8010 full base model nr
	char rsv0; //0x8011 
	char part_id;  //0x8012
	char rsv1[2]; //0x8013- 0x8014
	char nest_psv; //0x8015, nest <pass/fail flag | nest id>
	char nest_nec; //0x8016, nest error <time | type>
	char psv; //0x8017, process flag < ..|nest|amb|ict>, won't touched by nest???
	char rsv2[8]; //0x8018 - 0x801f
	char fb[0x10]; //0x8020 - 0x8036 fault bytes write back
} mfg_data,mfg_data_bk;

//base model id
static int bmr;
static char mailbox[16];
static char Model_LINorPWM;


//
enum{
	MODEL_IS_LIN = 0x01,  // previously MODEL_IS_PCM
	MODEL_IS_PWM,	  // previously MODEL_IS_ECM
	Linear_Mid, //
	Linear_Low,   //
};

//base model types declaration
enum {
	BM_28194324,
	BM_28194330,
	BM_28246719,
};

//base model number to id maps
const struct nest_map_s bmr_map[] = {
	/*Linear Mid Module*/
	{"28194324", BM_28194324},
	/*Linear Low Module*/
	{"28194330", BM_28194330},
	{"28246719", BM_28246719},
	END_OF_MAP,
};
enum {
	POWER_CONDITION,
	RAM_DOWNLOAD,
};
static void chips_bind(void)
{
	//PWM Output
	emcd_bind("DISCOUT5", "J1-71 Discrete Output5"); //Linear EGR, 300Hz, 50% duty cycle
	emcd_bind("DISCOUT6", "J1-72 Discrete Output6"); //Canister purge solenoid, 16Hz, 50% duty cycle
	emcd_bind("VSSOUT", "J1-73 Vehicle Speed Output"); // Fuel level guage, 128Hz, 50% duty cycle
	emcd_bind("PWMOUT3", "J1-74 PWM Output3"); //Thermal management, 100Hz, 50% duty cycle
	emcd_bind("PWMOUT4", "J1-77 PWM Output4"); //Front oxygen sensor heater, 16Hz 50% duty cycle
	emcd_bind("PWMOUT5", "J1-78 PWM Output5"); //Rear oxygen sensor heater, 16Hz 50% duty cycle
	emcd_bind("DISCOUT7", "J1-75 Discrete Output7");
	emcd_bind("DISCOUT8", "J1-76 Discrete Output8");

	//Port 0.8	J101-100	GENL TRM	Generator L terminal	128Hz 50% duty cycle
	csd_bind("SOLPWRA",  "J1-62 HS8Flt1-0"); //Coolant gauge, 128Hz, 50% duty cycle.
	csd_bind("SOLPWRB",  "J1-63 HS9Flt1-0"); //Oil Control Valve, 300Hz, 50% duty cycle
	csd_bind("SPAREHSD1", "J1-42 HS10Flt1-0");
	csd_bind("SPAREHSD2", "J1-43 HS11Flt1-0");
	csd_bind("PWMSOLA", "J1-70 LS1Flt1-0");
	csd_bind("PWMSOLB", "J1-69 LS2Flt1-0");
	csd_bind("PWMSOLC", "J1-68 LS3Flt1-0");

	csd_bind("PWMSOLD", "J1-64 LS4Flt1-0");
	csd_bind("PWMSOLE", "J1-65 LS5Flt1-0");
	csd_bind("PWMSOLF", "J1-66 LS6Flt1-0");
	csd_bind("LPSOL", "J1-67 LS7Flt1-0");
	csd_bind("PWMOUT1", "J1-44 LS8Flt1-0");
	csd_bind("PWMOUT2", "J1-45 LS9Flt1-0");
	csd_bind("DISCOUT1", "J1-22 LS10Flt1-0");
	csd_bind("DISCOUT2", "J1-23 LS11Flt1-0");
	csd_bind("DISCOUT3", "J1-24 LS12Flt1-0");
	csd_bind("HSG1", "CSD Pin17");
	csd_bind("HSG2", "CSD Pin18");
	csd_bind("HSG3", "CSD Pin19");
	csd_bind("HSG4", "CSD Pin20");
	csd_bind("HSG5", "CSD Pin21");
	csd_bind("HSG6", "CSD Pin22");
	csd_bind("DISCOUT4", "J1-25 LS13Flt1-0");
	
}


static void Test_Model(int model)
{
        cncb_signal(SIG1, SIG_LO);
	if(model == POWER_CONDITION)
		cncb_signal(SIG2, SIG_HI);
	else if (model == RAM_DOWNLOAD)
		cncb_signal(SIG2, SIG_LO);
	else 
		return;
}

#define MEMORY_READ_TIMEOUT		DFT_OP_TIMEOUT
ERROR_CODE Read_Memory(UINT32 addr, UINT8* pdata, UINT32 len)
{
	ERROR_CODE result ;
	UINT8 buffer[4] = "\0";
	buffer[3] = len & 0xFF; 
	//printf("%02X   %02X",buffer[2],buffer[3]);
	
	result = mcamOSDownload(DW_CAN1, REGD4_ADDR, buffer, 4, MEMORY_READ_TIMEOUT);
	if (result != E_OK) return result;
	//check for finish
	Delay10ms(10);

	//*(int *)& buffer[0] = len >> 16;
	//*(int *)& buffer[2] = len & 0xFF;
	buffer[0] = (addr >> 24) & 0xFF;
	buffer[1] = (addr >> 16) & 0xFF;
	buffer[2] = (addr >> 8 ) & 0xFF;
	buffer[3] = (addr >> 0 ) & 0xFF;

	result = mcamOSDownload(DW_CAN1, REGA4_ADDR, buffer, 4, MEMORY_READ_TIMEOUT);
	if (result != E_OK) return result;
	Delay10ms(10);
	
	result = mcamOSExecute(DW_CAN1, MEMORY_READ_CMD, MEMORY_READ_TIMEOUT);
	if( result != E_OK) return result;
	Delay10ms(10);
	
	//read data from mcamos outbox(note: byte 0 is TestID, should be ignored)
	result = mcamOSUpload(DW_CAN1, pdata, OUTBOX_ADDR, len, CAN_TIMEOUT);
	return result;
}


#define MEMORY_WRITE_TIMEOUT		DFT_OP_TIMEOUT //unit mS
#define EEPROM_WRITE_TIMEOUT		DFT_OP_TIMEOUT //uint mS
#define FLASH_WRITE_TIMEOUT		DFT_OP_TIMEOUT //uinit mS
ERROR_CODE Write_Memory(UINT32 addr, UINT8* pdata, UINT16 len)
{
	ERROR_CODE result ;
	int loopcnt = 0;
	UINT8 buffer[4]="\0";
	do{
		loopcnt++;
		//Sets Erase address 
		buffer[0] = 0x80;
		buffer[1] = 0x01;
		buffer[2] = 0xC0;
		buffer[3] = 0x00;
		result = mcamOSDownload(DW_CAN1,REGA4_ADDR, buffer, sizeof(buffer),MEMORY_WRITE_TIMEOUT);
		if (result != E_OK) return result;
		Delay10ms(1);
		result = mcamOSExecute(DW_CAN1,ERASE_FLASH_CMD, MEMORY_WRITE_TIMEOUT);
		if (result != E_OK) return result;
		Delay10ms(100);
                
                
		buffer[0] = (addr >> 24 ) & 0xFF;
		buffer[1] = (addr >> 16 ) & 0xFF;
		buffer[2] = (addr >> 8) & 0xFF;
		buffer[3] = (addr >> 0) & 0xFF;

		//send erase/write addr 
		result = mcamOSDownload(DW_CAN1, REGA4_ADDR, buffer, sizeof(buffer), MEMORY_WRITE_TIMEOUT);
		if (result != E_OK) return result;
		Delay10ms(1);
		/*result = mcamOSExecute(DW_CAN1,ERASE_FLASH_CMD, MEMORY_WRITE_TIMEOUT)
		if (result != E_OK) return result;*/
		//send write size 
		buffer[0] = (len >> 24 ) & 0xFF;
		buffer[1] = (len >> 16 ) & 0xFF;
		buffer[2] = (len >> 8) & 0xFF;
		buffer[3] = (len >> 0) & 0xFF;
		result = mcamOSDownload(DW_CAN1, REGD4_ADDR, buffer, sizeof(buffer), MEMORY_WRITE_TIMEOUT);
		if (result != E_OK) return result;
		Delay10ms(1);
		//send data payload
		result = mcamOSDownload(DW_CAN1, INBOX_ADDR, pdata, len, MEMORY_WRITE_TIMEOUT);
		if (result != E_OK) return result;
		Delay10ms(1);
		
		//excute
		result = mcamOSExecute(DW_CAN1, WRITE_FLASH_CMD, FLASH_WRITE_TIMEOUT);
		if (result != E_OK) return result;
		Delay10ms(100);

		result = mcamOSUpload(DW_CAN1, buffer, REGD2_ADDR, 0x04, CAN_TIMEOUT);
		if (result != E_OK) return result;
	}while( (buffer[2] != 0x33) && (buffer[3] != 0x33) && (loopcnt < 3));
		
		
		if(loopcnt == 3) result = DUT_OOPS;
		else result = E_OK;
		
		return result;
}


ERROR_CODE PrintSoftwareVersion(void)
{
	ERROR_CODE result ;
	UINT8 buffer[8]="\0";

	result = mcamOSExecute(DW_CAN1, CHECK_SWV_CMD , CAN_TIMEOUT);
	if( result != E_OK) return result;
        Delay10ms(100);
	result = mcamOSUpload(DW_CAN1, buffer, OUTBOX_ADDR, 8, CAN_TIMEOUT);
	if(result != E_OK) return result;
	if ( buffer[0] == 'L' ){
		Model_LINorPWM = MODEL_IS_LIN ;  //0x01
		message("LINEAR Module Detected\n");
		cncb_signal(LSD,SIG_HI);         //connects 7mH, 4.2ohms load
		
		if(!strcmp(mfg_data.bmr,"28194324")){
			Model_LINorPWM = Linear_Mid ;   //0x03
			message("Linear Mid Module (28194324) Detected\n");
		}else if(!strcmp(mfg_data.bmr,"28194330")){
		Model_LINorPWM = Linear_Low ;  //0x04
		message("Linear Low Module (28194330) Detected\n");
		}//else 
		//	result = E_INVALID_BASE_MODEL_ID;
			
	}else if ( buffer[0] == 'P' ){
		Model_LINorPWM = MODEL_IS_PWM;   //0x02
		message("PWM Module Detected\n");
		cncb_signal(LSD,SIG_LO);          //connects 4mH, 3.2ohms load
	}else 
		result = E_INVALID_BASE_MODEL_ID;

	return result;

	/*
	result = mcamOSExecute(DW_CAN1, CHECK_MODEL_CMD , CAN_TIMEOUT);
	if( result != E_OK) return result;
	result = mcamOSUpload(DW_CAN1, buffer, REGD2_ADDR, 4, CAN_TIMEOUT);
	if(result != E_OK) return result;
			message("\n***************current model:");
			for (int i = 0;i<4;i++)
				printf("%X",buffer[i]);

	*/

	//change model to pwer condition
	/*
         buffer[3]=0x04;
        Delay10ms(100);
	result = mcamOSDownload(DW_CAN1,REGD4_ADDR, buffer, 4, CAN_TIMEOUT);
	if( result != E_OK) return result;
                Delay10ms(100);
	result = mcamOSExecute(DW_CAN1, SET_MODEL_CMD , CAN_TIMEOUT);
	if(result != E_OK) return result;
	buffer[3]='\0';
        //        Delay10ms(100);
	//result = mcamOSUpload(DW_CAN1, buffer, REGD2_ADDR, 4, CAN_TIMEOUT);
	//if(result != E_OK) return result;
	//if(buffer[3] != 0x04)
	//{
	//	message("Change model error!\n");
	//	message("%2x",buffer[3]);
	//	return DUT_OOPS;
	//}

	result = mcamOSExecute(DW_CAN1, CHECK_MODEL_CMD , CAN_TIMEOUT);
	if( result != E_OK) return result;
	result = mcamOSUpload(DW_CAN1, buffer, REGD2_ADDR, 4, CAN_TIMEOUT);
	if(result != E_OK) return result;
	message("\n***************current model:");
	for (int i = 0;i<4;i++)
		printf("%X",buffer[i]);*/
	//return result;
}



ERROR_CODE PrintECMModel(void)
{
	ERROR_CODE result ;
	UINT8 buffer[8]="\0";

	result = mcamOSExecute(DW_CAN1, CHECK_MODEL_CMD , CAN_TIMEOUT);
	if( result != E_OK) return result;
	result = mcamOSUpload(DW_CAN1, buffer, REGD2_ADDR, 4, CAN_TIMEOUT);
	if(result != E_OK) return result;
	//message("current model:");
	if(buffer[3]==0x04)
          message("Model 4:Power Conditon Model\n");
	else if(buffer[3] == 0x03)
          message("Model 3:Input Polling Or RAM Download Model");
        else
		{
			message("\nWrong Model");
			result = NEST_OOPS;
		}
			//for (int i = 0;i<4;i++)
				//printf("%X",buffer[i]);
                        
	return result;

}


static void CyclingTest(void)
{
	int fail = 0, min, deadline;
	if(nest_fail())
		return;
	emcd_init();
	csd_init();


	switch(bmr) {
	case BM_28194324:
		csd_mask("HSG5");
	default:
		break;
	}

	nest_message("#Output Cycling Test Start ... \n");
	/*ccp_Init(&can1, 500000);
	//mailbox[0] = TESTID_OCYLTST;
	//mailbox[1] = 0x00; //cycle ETC/EST in sequencial mode
	if(bmr == BM_DK245105 || bmr == BM_28180087 || bmr == BM_28159907 || bmr == BM_28164665)
		mailbox[1] = 0x01; //cycle ETC/EST in paired mode
	else if(bmr == BM_28077390 || bmr == BM_28119979)
		mailbox[1] = 0x03; //cycle IAC/EST in paired mode
	//fail += Write_Memory(INBOX_ADDR, mailbox, 2);*/
	for(min = 0; min < CND_PERIOD; min ++) {
		//delay 1 min
		deadline = nest_time_get(1000 * 60);
		while(nest_time_left(deadline) > 0) {
			nest_update();
			nest_light(RUNNING_TOGGLE);
			/*fail = burn_verify(mfg_data.vp, mfg_data.ip);
			if(fail) {
				nest_error_set(PULSE_FAIL, "Cycling");
				break;
			}*/
		}

		if(!fail) {
			nest_message("T = %02d min\n", min);
                        //RELAY_IGN_SET(0);
                        //RELAY_BAT_SET(0);
                        //Test_Model(RAM_DOWNLOAD);
                        //nest_mdelay(100);
                        //RELAY_IGN_SET(1);
                        //RELAY_BAT_SET(1);
                        //fail=PrintECMModel();
                        //if(fail) break;
                        fail += mcamOSUpload(DW_CAN1, (UINT8 *)mailbox, FB_ADDR, 0x10, CAN_TIMEOUT);
                        //fail += emcd_verify(mailbox); /*verify [8300 - 8301]*/
			//fail += csd_verify(mailbox + (0x8304 - 0x8300)); /*verify [8304 - 8309]*/
			if (fail) {
				memcpy(mfg_data.fb, mailbox, sizeof(mfg_data.fb));
				nest_error_set(FB_FAIL, "Cycling");
				break;
			}
		}
		else break;
	}

	//restart the dut
	nest_power_reboot();
	//ccp_Init(&can1, 500000);
}

void TestStart(void)
{
	int fail = 0;
	//wait for dut insertion
	nest_wait_plug_in();
	Test_Model(POWER_CONDITION);
	nest_power_on();

	//get dut info through can bus
	fail = mcamOSInit(DW_CAN1, 500);
	if( fail != E_OK) ERROR_RETURN(CAN_FAIL, "Init CAN");
	message("Init CAN ... pass\n");
        PrintECMModel();
        //nest_power_off();
        //Test_Model(RAM_DOWNLOAD);
        //nest_power_on();
	//PrintECMModel();
	if( fail != E_OK) ERROR_RETURN(MTYPE_FAIL, "Model Fail");


	fail = Read_Memory(MFGDAT_ADDR, (UINT8 *) &mfg_data, sizeof(mfg_data));
	if(fail) {
		nest_error_set(CAN_FAIL, "CAN");
		return;
	}
	
	Delay10ms(100);
	//check base model nr
	fail = PrintSoftwareVersion();
	if( fail != E_OK) ERROR_RETURN(CAN_FAIL, "Base Model Fail");
	mfg_data.rsv0 = 0;
	nest_message("DUT S/N: %s\n", mfg_data.bmr);
	bmr = BM_28194324; //tricky, debug dut
	if(!nest_ignore(BMR)) {
		bmr = nest_map(bmr_map, mfg_data.bmr);
		if(bmr < 0) {
			nest_error_set(MTYPE_FAIL, "Model Type");
			return;
		}
	}

	//relay settings
	//cncb_signal(LSD,SIG_LO); //C71 FPR LOAD6(30Ohm + 70mH) JMP1 = GND, HSD
	//cncb_signal(SIG2,SIG_LO); //C70 SMR LOAD7(30Ohm + 70mH) JMP2 = GND, HSD
	//cncb_signal(SIG3,SIG_LO); //E7 = NC
	//cncb_signal(SIG6,SIG_LO); //ETC
	//if(bmr == BM_28077390 || bmr == BM_28119979)
	//	cncb_signal(SIG6,SIG_HI); //IAC

	//chip pinmaps
	chips_bind();

	//preset glvar
	mfg_data.nest_psv = (char) ((nest_info_get() -> id_base) & 0x7f);
	mfg_data.nest_nec = 0x00; //no err
	memset(mfg_data.fb, 0x00, sizeof(mfg_data.fb)); //+vp[4], ip[4]

	//check psv of amb
	if(!nest_ignore(PSV)) {
		if((mfg_data.psv & 0x80) != 0x80) {
			nest_error_set(PSV_FAIL, "PSV");
			return;
		}
	}

	//cyc ign, necessary???
	if(!nest_ignore(RLY)) {
		for(int cnt = 0; cnt < 50; cnt ++) {
			//RELAY_BAT_SET(0);
			//nest_mdelay(50);
			RELAY_IGN_SET(0);
			nest_mdelay(100);
			//RELAY_BAT_SET(1);
			//nest_mdelay(50);
			RELAY_IGN_SET(1);
			nest_mdelay(100);
		}
	}
}

void TestStop(void)
{
	int fail = 0, addr, size;
	nest_message("#Test Stop\n");

	//store fault bytes back to dut
	//addr = MFGDAT_ADDR + (int) &((struct mfg_data_s *)0) -> fb[0];
	//size = sizeof(mfg_data.fb);
	//fail += Write_Memory(addr, mfg_data.fb, size);

	if(pass())
	//store nest_psv & nest_nec
		mfg_data.nest_psv |= 0x80;
	else
		mfg_data.nest_psv &= 0x7F;
	mfg_data.nest_nec = nest_error_get() -> nec;
	addr = MFGDAT_ADDR ;//+ (int) &((struct mfg_data_s *)0) -> nest_psv;
        if(strcmp(mfg_data.bmr,"\0"))
	    fail += Write_Memory(addr, (UINT8 *)&mfg_data, sizeof(mfg_data));
        
	if(fail)
		nest_error_set(CAN_FAIL, "Eeprom Write");
        
        
        
	//fail = Read_Memory(MFGDAT_ADDR, (UINT8 *) &mfg_data_bk, sizeof(mfg_data_bk));
	//if(fail) {
		//nest_error_set(CAN_FAIL, "CAN");
	//}

	//send FIS BCMP(build complete) message
	//fisBCMP();

	//test finish
	nest_mdelay(1000 * 5); //wait 5s for eeprom write finish
	nest_power_off();
	nest_message("CONDITIONING COMPLETE\n");
	nest_wait_pull_out();
}

void main(void)
{
	nest_init();
	nest_power_off();
	nest_message("\nPower Conditioning - TCM8.0\n");
	nest_message("IAR C Version v%x.%x, Compile Date: %s,%s\n", (__VER__ >> 24),((__VER__ >> 12) & 0xfff),  __TIME__, __DATE__);
	while(1){
		TestStart();
		//RAMTest();
		//FaultTest();
		CyclingTest();
		//FaultTest();
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

The nest control circuits interface the MCU to the nest hardware
allowing the MCU to control nest functions. Five switches (low side driver)
are used to control nest power relays ( BAT & IGN) and indicator lights
(Running, Complete/Fail, Abort). An input interface is used to detect
when an AEM is installed in the nest. This input is also used to prevent
closing power relays until an AEM is installed.

The serial communication circuits interface the MCU to the controller
and transceiver hardware for several serial communications protocols.
These protocols are used to communicate with an AEM. The serial protocol
physical layers available are as follows: dual wire CAN, single wire CAN,
Keyword or ISO9141 and LIN.

The signal control circuits provide an interface between the MCU and
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
The MCU and PIC communicate via a UART serial interface and can be
used in a master (MCU) slave (PIC) arrangement. The initial programming
of the PIC can include a bootloader that allows it to be reprogrammed
by the MCU. The AP provides the CNCB with six outputs and three inputs.
The PIC includes an A/D converter and can be programmed to perform a
wide variety of signal generation and processing functions.


*******************************************************************************/
