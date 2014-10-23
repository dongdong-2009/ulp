#include "lib/nest.h"
#include "obsolete/obsolete.h"
#include "chips/c2mio.h"
#include "mt621.h"

#define CND_PERIOD		20 //unit: 10 seconds
//Test IDs
#define FACTORY_TEST_MODE_TESTID	((UINT8)0x00)
#define RAM_TEST_TESTID			((UINT8)0x04)
#define MEMORY_READ_TESTID		((UINT8)0x05)
#define EEPROM_WRITE_TESTID		((UINT8)0x07)
#define PHDL_FAULTTEST_TESTID		((UINT8)0x0A)
#define C2PSE_FAULTTEST_TESTID		((UINT8)0x0C)
#define C2MIO_FAULTTEST_TESTID		((UINT8)0x14)
#define OUTPUT_CYCLING_TESTID		((UINT8)0x21)

#define INBOX_ADDR		(0x40017400)
#define OUTBOX_ADDR		(0x40017420)
#define MFGDAT_ADDR		(0x00000000)
//mfg data structure
struct mfg_data_s {	
  UINT8 date[3];	// Julian day.0x00000000 - 0x00000002	
  UINT8 year;	//Last digit of year.	
  UINT8 sn[4];	//Sequence number or unit serial number.	
  UINT8 mfg_code;	//Manufacturing code.	
  UINT8 bmr[8];	//Full Base model number.	
  UINT8 ict;	//ICT STATION	
  UINT8 mol;	//MOL STATION	
  UINT8 psv;	//Power Conditioning Process Flag	
  UINT8 amb;	//AMB	UINT8 hot;	//HOT	
  UINT8 ups;	//UPS	UINT8 rsv1;	//Reserved	
  UINT8 modelid[2];//Model id  	
  UINT8 cnec;	//Conditioning Nest Error Code	
  UINT8 rsv2[5];	//Reserved	
  UINT8 ctfb[16];  //Conditioning Test Fault Byte	
  UINT8 rsv3[208]; //Reserved
} mfg_data;

static int bmr;
static char buf[32];
ModelType mtype;

/*
const char *xC2mioChName[] = 
{		
//Refer to Load List.xls,used in C2MIO test	
"J1-E35 COIL1(ESTCOILA)",	
"J1-E36 COIL2(ESTCOILB)",	
"J1-E47 COIL3(ESTCOILC)",	
"J1-E48 COIL4(ESTCOILD)",	
"J1-C14 MPR",	
"J1-C32 FUEL_PUMP_LO",	
"J1-E03 INJA",	
"J1-E04 INJB",
	
"J1-E07 INJC",	
"J1-E08 INJD",	
"J1-E05 VGIS",	
"J1-E06 TURRLF",	
"J1-E10 EGR",	
"J1-E09 TURBO_AWG",	
"J1-E11 VVT1",	
"J1-E12 VVT2",
	
"J1-E44 GENLT",	
"J1-E32 WPUMP_PWM",	
"J1-C46 CLTGAUGE",	
"J1-C59 FLGAUGE",	
"J1-C43 TACH",	
"J1-C44 FUELCON",	
"J1-C45 MIL",	
"J1-C62 CRSLAMP",
	
"J1-E42 PDA",	
"J1-C07 BBPRLY",	
"J1-C03 CCP",	
"J1-C60 FPR",	
"J1-E27 THCNTL",	
"J1-C04 FAN1",	
"J1-C05 FAN2",	
"J1-C06 INTFAN",
	
"J1-C09 ACCRLY",	
"J1-E41 O2HT_F",	
"J1-C13 O2HT_R",	
"J1-C56 IMMOREQ",	
"J1-C55 SVS"};
*/

//base model types declaration
enum 
{	
  BM_28340053,	
  BM_28358081,	
  BM_28358082,	
  BM_28358083,	
  BM_28358084,	
  BM_28358085,	
  BM_28358086,	
  BM_28362808,
};

//base mode number to id maps
const struct nest_map_s bmr_map[] = {	
  // 4 IGBT	
  { "28340053", BM_28340053},	
  { "28358081", BM_28358081},	
  { "28358082", BM_28358082},	
  { "28358083", BM_28358083},	
  { "28358084", BM_28358084},	
  { "28358085", BM_28358085},	
  { "28358086", BM_28358086},	
  { "28362808", BM_28362808},	
  END_OF_MAP,
};

static void chips_bind(void){	
  //PWM Output(23)	
  c2mio_bind("PCH07", "J1-E03 INJA");	
  c2mio_bind("PCH08", "J1-E04 INJB");	
  c2mio_bind("PCH09", "J1-E07 INJC");	
  c2mio_bind("PCH10", "J1-E08 INJD");	
  c2mio_bind("PCH01", "J1-E35 COILA");	
  c2mio_bind("PCH02", "J1-E36 COILB");	
  c2mio_bind("PCH03", "J1-E47 COILC");	
  c2mio_bind("PCH04", "J1-E48 COILD");
	
  c2mio_bind("PCH21", "J1-C43 TACH");	
  c2mio_bind("PCH27", "J1-C03 CCP");	
  c2mio_bind("PCH34", "J1-E41 O2HT_F");
  c2mio_bind("PCH35", "J1-C13 O2HT_R");	
  c2mio_bind("PCH22", "J1-C44 FUELCON");	
  c2mio_bind("PCH19", "J1-C46 CLTGAUGE");	
  c2mio_bind("PCH20", "J1-C59 FLGAUGE");	
  c2mio_bind("PCH17", "J1-E44 GENLT");
	
  c2mio_bind("PCH15", "J1-E11 VVT1");	
  c2mio_bind("PCH16", "J1-E12 VVT2");	
  c2mio_bind("PCH13", "J1-E10 EGR");	
  c2mio_bind("PCH29", "J1-E27 THCNTL");	
  c2mio_bind("PCH14", "J1-E09 TURBO_AWG");	
  c2mio_bind("PCH18", "J1-E32 WPUMP_PWM");	
  c2mio_bind("PCH06", "J1-C32 FUEL_PUMP_LO");
	
  //Discrete Output(14)	
  c2mio_bind("PCH33", "J1-C09 ACCRLY");	
  c2mio_bind("PCH12", "J1-E06 TURRLF");	
  c2mio_bind("PCH28", "J1-C60 FPR");	
  c2mio_bind("PCH05", "J1-C14 MPR");	
  c2mio_bind("PCH25", "J1-E42 PDA");	
  c2mio_bind("PCH11", "J1-E05 VGIS");	
  c2mio_bind("PCH23", "J1-C45 MIL");	
  c2mio_bind("PCH30", "J1-C04 FAN1");
	
  c2mio_bind("PCH31", "J1-C05 FAN2");	
  c2mio_bind("PCH32", "J1-C06 INTFAN");	
  c2mio_bind("PCH26", "J1-C07 BBPRLY");	
  c2mio_bind("PCH36", "J1-C56 IMMOREQ");	
  c2mio_bind("PCH37", "J1-C55 SVS");	
  c2mio_bind("PCH24", "J1-C62 CRSLAMP");
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
  timeout = timeout * 10; //unit change: ms -> 100uS//1000	
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
#if 0
  result = wait_test_finish(FACTORY_TEST_MODE_TESTID, FACTORY_TEST_MODE_TIMEOUT);	
  if (result != E_OK) return result;
	
#endif
  nest_mdelay(1000);
  //print result	
  result = mcamOSUpload(DW_CAN1, buffer, OUTBOX_ADDR, 3, CAN_TIMEOUT);	
  if (result != E_OK) return result;
	
  //verify magic byte and get software version	
  if(buffer[0] != FACTORY_TEST_MODE_TESTID) return E_OOPS;	
  major = buffer[1];	
  minor = buffer[2];	
  message("TESTABILITY S/W VERSION: mt62.1 0x%x-0x%x\n", major, minor);
	
  return result;
}

#define MEMORY_READ_TIMEOUT		RW_OP_TIMEOUT
ERROR_CODE Read_Memory(UINT32 addr, UINT8* pdata, int len)
{	
  ERROR_CODE result ;	
  UINT8 buffer[6];
	
  //send memory read command to mcamos inbox 	
  buffer[0] = MEMORY_READ_TESTID;	
  buffer[1] = len&0xff;	
  buffer[2] = (addr>>24)&0xff;	
  buffer[3] = (addr>>16)&0xff;	
  buffer[4] = (addr>> 8)&0xff;	
  buffer[5] = (addr>> 0)&0xff;	
  result = mcamOSDownload(DW_CAN1, INBOX_ADDR, buffer, sizeof(buffer), CAN_TIMEOUT);	
  if (result != E_OK) return result;
	
  //wait	
  Delay10ms(100);	
  result = wait_test_finish(MEMORY_READ_TESTID, MEMORY_READ_TIMEOUT);	
  if (result != E_OK) return result;
	
  //read data from mcamos outbox(note: byte 0 is TestID, should be ignored)	
  result = mcamOSUpload(DW_CAN1, pdata, OUTBOX_ADDR+1, len, CAN_TIMEOUT);	
  return result;
}

#define EEPROM_WRITE_TIMEOUT		RW_OP_TIMEOUT //uint mS
// flash eeprom write function, not RAM memory or register write function!
ERROR_CODE Write_Memory(UINT32 addr, UINT8* pdata, int len){	
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
  Delay10ms(100);  //1S time for memory writing!!!	
  result = wait_test_finish(EEPROM_WRITE_TESTID, EEPROM_WRITE_TIMEOUT);	
  if (result != E_OK) return result; 
	
  //read op result	
  result = mcamOSUpload(DW_CAN1, buffer, OUTBOX_ADDR, 2, CAN_TIMEOUT);	
  if (result != E_OK) return result;
	
  if(buffer[1] == 0x00) result = E_OK;	
  else result = DUT_OOPS;
	
  return result;
}

#define RAM_TEST_TIMEOUT			((UINT32)10000)  //10s
ERROR_CODE Execute_RAMTest(UINT32 Start_Addr, UINT32 End_Addr)
{	
  ERROR_CODE result ;	
  UINT8 buffer[9];
	
  //send ram test command to mcamos inbox	
  buffer[0] = RAM_TEST_TESTID;	
  buffer[1] = (Start_Addr>>24)&0xff;	
  buffer[2] = (Start_Addr>>16)&0xff;	
  buffer[3] = (Start_Addr>> 8)&0xff;	
  buffer[4] = (Start_Addr>> 0)&0xff;	
  buffer[5] = (End_Addr>>24)&0xff;	
  buffer[6] = (End_Addr>>16)&0xff;	
  buffer[7] = (End_Addr>> 8)&0xff;	
  buffer[8] = (End_Addr>> 0)&0xff;
	
  result = mcamOSDownload(DW_CAN1, INBOX_ADDR, buffer, 9, CAN_TIMEOUT);	
  if(result != E_OK) return result;
	
  //wait	
  result = wait_test_finish(RAM_TEST_TESTID, RAM_TEST_TIMEOUT);	
  if(result != E_OK) return result;
	
  //read op result	
  result = mcamOSUpload(DW_CAN1, buffer, OUTBOX_ADDR, 2, CAN_TIMEOUT);	
  if(result != E_OK) return result;
	
  //check result	
  if(buffer[1] == 0) return E_OK;	
  else return DUT_OOPS;
}

#define RAM_TEST_START_ADDR		(0x40000000)
#define RAM_TEST_END_ADDR		(0x40001FFF)
static void RAMTest(void)
{	
  ERROR_CODE result;
	
  if(nest_fail())		
    return;
	
  message("#RAM Test Start ... \n");	
  result = Execute_RAMTest(RAM_TEST_START_ADDR, RAM_TEST_END_ADDR);	
  if (result != E_OK) 	
  {		
    ERROR(RAM_FAIL, "RAM Test");	
  }	
  else 		
    message("RAM test ... pass\n");
	
  //restart the dut	
  nest_power_reboot();	
  mcamOSInit(DW_CAN1, CAN_KBAUD);	
  return;
}

enum 
{	
  PHDL_FAULTTEST_MODE_OPEN_FAULT = 0,	
  PHDL_FAULTTEST_MODE_SHORT_FAULT,	
  PHDL_FAULTTEST_MODE_OVER_CURRENT
};

#define PHDL_FAULTTEST_TIMEOUT		DFT_OP_TIMEOUT //unit mS
ERROR_CODE Execute_PHDLFaultTest(char mode,UINT8* pdata)
{	
  ERROR_CODE result ;	
  UINT8 buffer[3];
	
  //send test command	
  buffer[0] = PHDL_FAULTTEST_TESTID;	
  buffer[1] = (mode&0xff);	
  buffer[2] = 0x01;	
  result = mcamOSDownload(DW_CAN1, INBOX_ADDR, buffer, 3, CAN_TIMEOUT);	
  if(result != E_OK) return result;
	
  //wait	
  result = wait_test_finish(PHDL_FAULTTEST_TESTID, PHDL_FAULTTEST_TIMEOUT);	
  if(result != E_OK) return result;
	
  //read result	
  result = mcamOSUpload(DW_CAN1, pdata, OUTBOX_ADDR+1, 1, CAN_TIMEOUT);	
  return result;
}

enum 
{	
  C2MIO_FAULTTEST_MODE_OPENSHORT_TO_GND = 0,	
  C2MIO_FAULTTEST_MODE_SHORT_TO_BAT
};

#define C2MIO_FAULTTEST_TIMEOUT		DFT_OP_TIMEOUT //unit mS
ERROR_CODE Execute_C2MIOFaultTest(char mode, UINT8* pdata)
{	
  ERROR_CODE result ;	
  UINT8 buffer[2];
	
  //send ram test command to mcamos inbox	
  buffer[0] = C2MIO_FAULTTEST_TESTID;	
  buffer[1] = (mode&0xff);	
  result = mcamOSDownload(DW_CAN1, INBOX_ADDR, buffer, 2, CAN_TIMEOUT);	
  if(result != E_OK) return result;
	
  //wait	
  result = wait_test_finish(C2MIO_FAULTTEST_TESTID, C2MIO_FAULTTEST_TIMEOUT);	
  if(result != E_OK) return result;
	
  //read result	
  result = mcamOSUpload(DW_CAN1, pdata, OUTBOX_ADDR+1, 10, CAN_TIMEOUT);
	
  return result;
}

#define C2PSE_FAULTTEST_TIMEOUT		DFT_OP_TIMEOUT //unit mS
ERROR_CODE Execute_C2PSEFaultTest(UINT8* pdata)
{	
  ERROR_CODE result ;	
  UINT8 buffer[2];
	
  //send ram test command to mcamos inbox	
  buffer[0] = C2PSE_FAULTTEST_TESTID;	
  result = mcamOSDownload(DW_CAN1, INBOX_ADDR, buffer, 1, CAN_TIMEOUT);	
  if(result != E_OK) return result;
	
  //wait	
  result = wait_test_finish(C2PSE_FAULTTEST_TESTID, C2PSE_FAULTTEST_TIMEOUT);	
  if(result != E_OK) return result;
	
  //read result	
  result = mcamOSUpload(DW_CAN1, pdata, OUTBOX_ADDR+1, 1, CAN_TIMEOUT);	
  return result;
}

ERROR_CODE Verify_PHDLFaultTest(UINT8 *pdata)
{	
  ERROR_CODE result;	
  result = E_OK;
	
  if((pdata[0]&0xf0) != 0 )
  {		
    result = DUT_OOPS;		
    ERROR(FB_FAIL, "PHDL Fault Diagnostic Test");		
    message( "PHDL fault data byte is %x \n",pdata );		
    if( pdata[0]&0x80 ) 
    {			
      message( "PHDL short to VBAT! \n" );		
    }		
    else if( pdata[0]&0x40 ) 
    {			
      message( "PHDL short to GND! \n" );		
    }		
    else if( pdata[0]&0x20 ) 
    {			
      message( "PHDL open load! \n" );		
    }		
    else if( pdata[0]&0x10 ) 
    {			
      message( "PHDL over current! \n" );		
    }	
  }	
  else message("PHDL Fault Diagnostic Test ...pass\n");
	
  return result;
}

ERROR_CODE Verify_ETCPWM(UINT8 *pdata)
{	
  ERROR_CODE result;	
  result = E_OK;
	
  if( pdata[0] ) 
  {		
    result = DUT_OOPS;		
    ERROR(FB_FAIL, "ETC PWMOUT Fault!!!");	
  }	
  else 
    message("ETC PWMOUT Frequency is %dHz\n",((pdata[1] << 8) | pdata[2]));
	
  return result;
}

ERROR_CODE Verify_C2PSEFaultTest(UINT8 *pdata)
{	
  ERROR_CODE result;	
  result = E_OK;
	
  if( pdata[0] != 0 )
  {		
    result = DUT_OOPS;		
    ERROR(FB_FAIL, "C2PSE Fault Diagnostic Test");		
    message( "C2PSE fault data byte is %x \n",pdata[0] );		
    if( pdata[0]&0x03 == 0x01 )			
      message( "C2PSE open load! \n" );		
    else if( pdata[0]&0x03 == 0x02 )			
      message( "C2PSE short to VBAT! \n" );		
    else if( pdata[0]&0x03 ==0x03 )			
      message( "C2PSE over current! \n" );	
  }	
  else message("C2PSE Fault Diagnostic Test ...pass\n");
	
  return result;
}

static void mask_pch06(void)
{
	switch(bmr) {
	case BM_28358081:
		c2mio_mask("PCH06");
		break;
	case BM_28358082:
		c2mio_mask("PCH06");
		break;
	case BM_28358083:
		c2mio_mask("PCH06");
		break;
	case BM_28358084:
		c2mio_mask("PCH06");
		break;
	default:
		break;
	}
}
static void FaultTest(void)
{	
  ERROR_CODE result = 0;	
  int i;	
  if(nest_fail())		
    return;
	
  if((mfg_data.sn[3]!='0')&&(mfg_data.sn[3]!='1'))
    return;
  c2mio_init();
  mask_pch06();
//  c2mio_mask("PCH33");
  nest_mdelay(1000);
  nest_message("#C2MIO Open/Short to GND Test Start ... \n");	
  result += Execute_C2MIOFaultTest(C2MIO_FAULTTEST_MODE_OPENSHORT_TO_GND, buf);	
  for(i=0;i<10;i++)	
  {		
    message("%02x ",buf[i]);	
  }	
  message("\n");	
  result += c2mio_verify(buf);
	
  nest_mdelay(1000);	
#if 0
  nest_message("#C2MIO Short to VBAT Test Start ... \n");	
  result += Execute_C2MIOFaultTest(C2MIO_FAULTTEST_MODE_SHORT_TO_BAT, buf);	
  for(i=0;i<10;i++)	
  {		
    message("%02x ",buf[i]);	
  }	
  message("\n");	
  result += c2mio_verify(buf);
#endif
	
  nest_message("#PHDL Open Fault Test Start ... \n");	
  result += Execute_PHDLFaultTest(PHDL_FAULTTEST_MODE_OPEN_FAULT, buf);
  result += Verify_PHDLFaultTest(buf);	
  nest_mdelay(1000);	

#if 0 ///////////////////////add yu////////////////////////////
  nest_message("#PHDL Short Fault Test Start ... \n");	
  result += Execute_PHDLFaultTest(PHDL_FAULTTEST_MODE_SHORT_FAULT, buf);	
  result += Verify_PHDLFaultTest(buf);	
  nest_mdelay(1000);	
  nest_message("#PHDL Over Current Test Start ... \n");	
  result += Execute_PHDLFaultTest(PHDL_FAULTTEST_MODE_OVER_CURRENT, buf);	
  result += Verify_PHDLFaultTest(buf);	
  nest_mdelay(1000);
#endif        //////////////////////////////////////////////////
	
  nest_message("#C2PSE Fault Diagnostic Test Start ... \n");	
  result += Execute_C2PSEFaultTest(buf);	
  result += Verify_C2PSEFaultTest(buf);
	
  if(result)
  {		
    ERROR(FB_FAIL, "Fault Byte Test");	
  }
	
  //restart the dut	
  nest_power_reboot();	
  mcamOSInit(DW_CAN1, CAN_KBAUD);	
  return;
}

enum 
{	
  CYCLING_LOOP_IN = 0,	
  CYCLING_LOOP_OUT
};

ERROR_CODE Execute_Cycling(char mode)
{	
  ERROR_CODE result ;	
  UINT8 buffer[2];
	
  /* base model type option:		
  0	cycle ETC, 4EST/IGBT in sequential mode and all other output		
  1	cycle ETC, 2EST/IGBT in paired mode and all other output	
  */
	
  //send output cycling command to mcamos inbox	
  buffer[0] = OUTPUT_CYCLING_TESTID;	
  if(mode == CYCLING_LOOP_IN)		
    buffer[1] = mtype.par;	
  else if(mode == CYCLING_LOOP_OUT)		
    buffer[1] = 0x77;
	
  result = mcamOSDownload(DW_CAN1, INBOX_ADDR, buffer, 2, CAN_TIMEOUT);
	
  return result;
}

ERROR_CODE ReadFaultByte(void)
{	
  ERROR_CODE result;	
  UINT16 len;	
  int i;	
  len = 16;
#if 0	
  //wait	
  result = wait_test_finish(OUTPUT_CYCLING_TESTID, DFT_OP_TIMEOUT);	
  if(result != E_OK) 
    return result;
#endif
	
    //read result	
  result = mcamOSUpload(DW_CAN1, buf, OUTBOX_ADDR+1, len, CAN_TIMEOUT);
	
  for(i = 0;i < len;i++)	
  {		
    message("FaultByte%02d = %02x\n", i+1, buf[i]);	
  }
	
  return result;
}

static void CyclingTest(void)
{	
  ERROR_CODE result = 0;	
  int tensec, deadline;
	
  if(nest_fail())
    return;
  if(mfg_data.sn[3]!='0')
    return;
	
  c2mio_init();
  mask_pch06();
  message("#Output Cycling Test Start... \n");	
  mcamOSInit(DW_CAN1, CAN_KBAUD);	
  result += Execute_Cycling(CYCLING_LOOP_IN);	
  nest_mdelay(500);
  for(tensec = 0; tensec < CND_PERIOD; tensec ++) 
  {		
    //delay 60 seconds
    deadline = nest_time_get(1000 * 60);
    while(nest_time_left(deadline) > 0) 
    {			
      nest_update();			
      nest_light(RUNNING_TOGGLE);		
    }
		
    nest_message("T = %d min\n", (tensec));
    result += ReadFaultByte();		
    if(tensec != 0) 
    {			
      result += Verify_PHDLFaultTest(buf);			
      result += c2mio_verify(buf + 1);			
      result += Verify_ETCPWM(buf + 11);			
      result += Verify_C2PSEFaultTest(buf + 14);		
    }		
    if (result) 
    {			
      nest_error_set(FB_FAIL, "Cycling Test");			
//      break;		
    }	
    if(nest_fail())
      return;
      mask_pch06();
  }
	
  Execute_Cycling(CYCLING_LOOP_OUT);	
  nest_mdelay(2000);	
  message("Output Cycling Test Finished \n");
	
  nest_power_reboot();	
  mcamOSInit(DW_CAN1, CAN_KBAUD);	
  return;
}

#define PCB_SEQ_ADDR			(0x00000004)
#define SEQ_NUM_SIZE			(4)
#define BASE_MODEL_TYPE_ADDR		(0x00000018)
#define BASE_MODEL_TYPE_SIZE		((UINT8)0x02)
#define PSV_AMB_ADDR			(0x00000014)
#define PSV_CND_ADDR			(0x00000013)
void TestStart(void)
{	
  ERROR_CODE result;	
  UINT8 buffer[4];	
  UINT8 amb;
  UINT8 i;
  UINT8 j=0,mm=0;
  //	UINT8 psv, psv_save;
  //	int cnt;
	
  //all led off	
  nest_light(ALL_OFF);
	
  //wait for module insertion	
  nest_wait_plug_in();
	
  //power on the dut	
  nest_power_on();
	
  //init CAN	
  mcamOSInit(DW_CAN1, CAN_KBAUD);
	
  //print testability s/w version	
  result = PrintSoftwareVersion();	
  if(result != E_OK) 
    ERROR_RETURN(CAN_FAIL, "Print S/W Version");
	
  result = Read_Memory(MFGDAT_ADDR,(char *) &mfg_data, sizeof(mfg_data));	
  if(result) 
  {		
    nest_error_set(CAN_FAIL, "CAN");		
    return;	
  }	
/////////////////////add yu////////////////////////
	for(i=0;i<48;i++){
		printf("  %02x", mfg_data.date[i]);
		j++;
		if(j==16){
			j=0;
			printf("\n");
		}
	}
////////////////////////////////////////////
  //test Sample model ID	
  if(0) 
  {		
    memcpy(mfg_data.bmr, "28340053", sizeof(mfg_data.bmr));		
    Write_Memory(MFGDAT_ADDR, (char *) &mfg_data, sizeof(mfg_data));	
  }	
  message("DUT S/N: %s\n", mfg_data.bmr);	
  bmr = nest_map(bmr_map, mfg_data.bmr);	
  if(bmr < 0) {		
    ERROR_RETURN(MTYPE_FAIL, "Model Type");		
    return;	
  }
	
  chips_bind();
#if 0	//write amb psv	
  amb = 0x00;	
  result = Write_Memory(PSV_AMB_ADDR, &amb, 1);	
  if(result == E_OK) 
    message("Write AMB PSV Pass\n");
#endif
	
  //read AMB PSV	
  result = Read_Memory(PSV_AMB_ADDR, &amb, 1);	
  if( (result != E_OK) || (amb&(1<<7)) ) 
  {		
    ERROR_RETURN(PSV_FAIL, "Read AMB PSV");		
    return;	
  }	
  else 
  {		
    message("Read AMB PSV ... pass\n");	
  }
	
  // preset model,psv,cnec	
  mfg_data.psv = (char) ((nest_info_get() -> id_base) & 0x7f);	
  mfg_data.cnec = 0x00;	
  memset(mfg_data.ctfb, 0x00, sizeof(mfg_data.ctfb));
#if 0	//write CND PSV	
  psv_save = psv = ((1<<7)|(nest_info_get() -> id_base))&0xff;	
  esult = Write_Memory(PSV_CND_ADDR, &psv, 1);	
  if( result != E_OK ) 
    ERROR_RETURN(PSV_FAIL, "Write CND PSV");
	
  //verify PSV	
  result = Read_Memory(PSV_CND_ADDR, &psv, 1);	
  if( result != E_OK ) 
    ERROR_RETURN(PSV_FAIL, "Read CND PSV");	
  if ( psv != psv_save ) 
    ERROR_RETURN(PSV_FAIL, "Verify CND PSV");	
  message("Preset PSV_CND_BYTE ... pass\n");
#endif
	
  //read base model ID from addr 0x0000_0018 and 0x0000_0019	
  result = Read_Memory(BASE_MODEL_TYPE_ADDR, buffer, BASE_MODEL_TYPE_SIZE);	
  if (result != E_OK) 
    ERROR_RETURN(MTYPE_FAIL, "Read Base Model");	
  mtype.igbt = (BOOL)((buffer[0]&0xF0) == 0xA0);	
  mtype.iac  = (BOOL)((buffer[0]&0x0F) == 0x05);	
  mtype.par = (BOOL)((buffer[1]&0xFF) == 0xFF);	
  message("Model ID (%02x,%02x): \n",buffer[0],buffer[1]);	
  PRINT_MODEL_TYPE();
	
  return;
}

#define FB_ADDR		(0x00000020)
#define NEC_ADDR	(0x0000001A) //NEST ERROR CODE
void TestStop(void)
{	
  ERROR_CODE result;	
  UINT8 psv, psv_save;	
  UINT8 nec, nec_save;	
  int tensec;
	
  message("#Test Stop\n");
	
  //store fault bytes back to dut	
  memcpy(mfg_data.ctfb, buf, sizeof(mfg_data.ctfb));
  result = Write_Memory(FB_ADDR, mfg_data.ctfb, sizeof(mfg_data.ctfb));	
  if( result != E_OK ) 
    ERROR(MISC_FAIL, "Write back Fault Bytes");
	
  //set nest error code byte	
  tensec = nest_error_get() -> time / 10000;	
  if(tensec > 20) 
    tensec = 20;	
  nec_save = nec = ((tensec&0x0f)<<4) | (nest_error_get() -> type & 0x0f);	
  message("Set Nest Test Code<0x%02x> to addr 0x%08x ... \n", nec, NEC_ADDR);	
  result = Write_Memory(NEC_ADDR, &nec, 1);	
  if( result != E_OK ) 
    ERROR(NEC_FAIL, "Write NEC");	
  result = Read_Memory(NEC_ADDR, &nec, 1);	
  if( result != E_OK ) 
    ERROR(NEC_FAIL, "Read NEC");	
  if ( nec !=  nec_save )
    ERROR(PSV_FAIL, "Verify NEC");
	
  //write psv	
  if(pass()) 
  {		
    psv_save = psv = (0<<7)|((nest_info_get() -> id_base) & 0x7f);		
    result = Write_Memory(PSV_CND_ADDR, &psv, 1);		
    if( result != E_OK ) 
      ERROR(PSV_FAIL, "Write PSV");		
    result = Read_Memory(PSV_CND_ADDR, &psv, 1);		
    if( result != E_OK ) 
      ERROR(PSV_FAIL, "Read PSV");		
    if ( psv !=  psv_save ) 
      ERROR(PSV_FAIL, "Verify PSV");	}
	
  //test finish, power down	
  nest_mdelay(2000);	
  nest_power_off(); // Ensure signal, power, and lights out.	
  if ( pass() )
  {
    message("All Test Passing!!!\n");
    nest_light(PASS_CMPLT_ON);
  }
  else 
    nest_light(FAIL_ABORT_ON);	
  Delay10ms(1);//10ms is okay	
  message("CONDITIONING COMPLETE\n");	
  nest_wait_pull_out();
}

void main(void){	
  nest_init();	
  nest_power_off();	
  nest_message("\nPower Conditioning - MT62.1\n");	
  nest_message("IAR C Version v%x.%x, Compile Date: %s,%s\n", (__VER__ >> 24),((__VER__ >> 12) & 0xfff),  __TIME__, __DATE__);	
  nest_message("nest ID : MT62.1-%03d\n",(*nest_info_get()).id_base);
	while(1)	
        {		
          TestStart();		
          RAMTest();		
          FaultTest();		
          CyclingTest();		
//          FaultTest();		
//          RAMTest();		
          TestStop();	
        }
}