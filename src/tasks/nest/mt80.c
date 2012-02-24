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
#include "chips/vsep.h"
#include "chips/hfps.h"
#include "chips/phdh.h"

#define CND_PERIOD	15 //unit: miniutes

#define TESTID_PHDHTST	0x06
#define TESTID_VSEPTST	0x07
#define TESTID_RAMTEST	0x0f
#define TESTID_EEWRITE	0x1b
#define TESTID_OCYLTST	0x20

#define INBOX_ADDR	(0xd0001000)
#define OUTBOX_ADDR	(0xd0001080)
#define MFGDAT_ADDR	(0x80008000)

//mfg data structure
struct mfg_data_s {
	char date[3];//0x8000 - 0x8002
	char year; //0x8003 last digit of year
	char sn[4]; //0x8004 - 0x8007 sequency nr
	char mfg_code; //0x8008 manufacturing code
	char bmr[8]; //0x8009 - 0x8010 full base model nr
	char rsv1[4]; //0x8011 - 0x8014
	char nest_psv; //0x8015, nest <pass/fail flag | nest id>
	char nest_nec; //0x8016, nest error <time | type>
	char psv; //0x8017, process flag < ..|nest|amb|ict>, won't touched by nest???
	char rsv2[8]; //0x8018 - 0x801f
	char fb[0x17]; //0x8020 - 0x8036 fault bytes write back
	char rsv3; //0x8037
	unsigned short vp[4]; //0x8038 - 0x803f
	unsigned short ip[4]; //0x8040 - 0x8048
} mfg_data;

//base model id
static int bmr;
static char mailbox[32];

//base model types declaration
enum {
	BM_28077390,
	BM_28119979,
	BM_28164665,
	BM_28180087,
	BM_28264387,

	BM_28159907,
	BM_28190870,
	BM_28249355,

	BM_DK245105,
};

//base model number to id maps
const struct nest_map_s bmr_map[] = {
	/*2IGBT*/
	{"28077390", BM_28077390},
	{"28119979", BM_28119979},
	{"28164665", BM_28164665},
	{"28180087", BM_28180087},
	{"28264387", BM_28264387},

	/*4IGBT*/
	{"28159907", BM_28159907},
	{"28190870", BM_28190870},
	{"28249355", BM_28249355},

	/*OBSOLETE?*/
	{"DK245105", BM_DK245105},
	END_OF_MAP,
};

static void chips_bind(void)
{
	//PWM Output
	vsep_bind("PCH17", "J4-E59 LEGR"); //Linear EGR, 300Hz, 50% duty cycle
	vsep_bind("PCH06", "J4-E57 CCP"); //Canister purge solenoid, 16Hz, 50% duty cycle
	vsep_bind("PCH23", "J4-E52 FUEL_LEVEL_GUAGE"); // Fuel level guage, 128Hz, 50% duty cycle
	vsep_bind("PCH22", "J4-E58 THERMAL_MGMT"); //Thermal management, 100Hz, 50% duty cycle
	vsep_bind("PCH19", "J4-E14 O2HTRA"); //Front oxygen sensor heater, 16Hz 50% duty cycle
	vsep_bind("PCH20", "J4-C46 O2HTRB"); //Rear oxygen sensor heater, 16Hz 50% duty cycle
	//Port 0.8	J101-100	GENL TRM	Generator L terminal	128Hz 50% duty cycle
	vsep_bind("PCH16", "J4-C68 COOLANT_GAUGE"); //Coolant gauge, 128Hz, 50% duty cycle.
	vsep_bind("PCH18", "J4-E54 VVT1"); //Oil Control Valve, 300Hz, 50% duty cycle

	//Pulse/Frequency Output
	vsep_bind("PCH09", "J4-E65 INJA"); //Fuel injector 1, 50Hz, 50% duty cycle.
	vsep_bind("PCH10", "J4-E64 INJB"); //Fuel injector 2, 50Hz, 50% duty cycle.
	vsep_bind("PCH11", "J4-E66 INJB"); //Fuel injector 3, 50Hz, 50% duty cycle.
	vsep_bind("PCH12", "J4-E63 INJD"); //Fuel injector 4, 50Hz, 50% duty cycle.
	vsep_bind("PCH01", "J4-E1 Coil A/EST A"); //Up integrated ignition coil A, 50 Hz and 3msec ON time.
	vsep_bind("PCH02", "J4-E17 Coil B/EST B"); //Up integrated ignition coil B, 50 Hz and 3msec ON time.
	vsep_bind("PCH03", "J4-E33 Coil C/EST C"); //Up integrated ignition coil C, 50 Hz and 3msec ON time.
	vsep_bind("PCH04", "J4-E53 Coil D/EST D"); //Up integrated ignition coil D, 50 Hz and 3msec ON time.
	//Port 0.4			J101-23	IACBHI	IACB idle air control	 IACB = 75Hz, 50% duty cycle.
	//			J101-22	IACBLO
	//Output Register [7:5]		J101-49	ETC Motor HI/ IACALO	Electronic throttle control (ETC) / IACA idle air control	ETC = 4khz, 50% duty cycle. Reverse direction every 10 seconds
	//			J101-24	ETC Motor LO/ IACAHI		IACA = 75Hz, 50% duty cycle.
	//N.A			J101-97	Tachometer	Tachometer signal	 250Hz , 50% duty cycle
	vsep_bind("PCH05", "J4-E62 O2HTRC"); //Fuel consumption / VSS meter	 250Hz, 50% duty cycle

	//Discrete Output
	vsep_bind("PCH21", "J4-C50 PWMFAN"); //PDA Actuator, 128Hz, 50% duty cycle
	vsep_bind("PCH07", "J4-E31 CMCV/PDA"); //	Fuel pump relay, 0.5 Hz, 80% duty cycle
	vsep_bind("PCH25", "J4-C60 EVAP"); //Oil Pressure Low Lamp,  300Hz, 50% duty cycle.
	vsep_bind("PCH28", "J4-C67 FAN #1"); //Fan control 1, 0.5 Hz, 80% duty cycle
	vsep_bind("PCH29", "J4-C66 FAN #2"); // Relay Fan control 2, 0.5 Hz, 80% duty cycle
	vsep_bind("PCH13", "J4-C71 FPR"); //Fuel pump relay, 0.5 Hz, 80% duty cycle
	vsep_bind("PCH30", "J4-C48 MIL"); //Malfunctions indicator lamp, 0.5 Hz, 80% duty cycle
	vsep_bind("PCH15", "J4-C51 AC_CLUTCH"); //AC clutch relay, 0.5 Hz, 80% duty cycle
	vsep_bind("PCH27", "J4-C62 MPR"); //Main power relay, 0.5 Hz, 80% duty cycle
	vsep_bind("PCH08", "J4-E15 VGIS"); //Variable Geometry Intake Solenoid, 0.5 Hz, 80% duty cycle
	vsep_bind("PCH26", "J4-C61 SVS"); //Upshift Indication Teltale/ SVS Lamp	0.5 Hz, 80% duty cycle
	vsep_bind("PCH14", "J4-C70 SMR"); //Starter control relay, 0.5 Hz, 80% duty cycle
	vsep_bind("PCH24", "J4-C47 O2HTRD"); //Evaporative diagnostic solenoid (Canister vent solenoid), 0.5 Hz, 80% duty cycle

	//Communication
	//CAN CCP 2.1	J101-54, 55	CAN_Hi / CAN_Lo	CAN Communication	N.A.

	/*Power Output
	STATUS [8]		VKAM5V	VKAM5V	N.A.
	STATUS [1]		VKAML2	VKAML2	N.A.
	DIAG1 [2:0]		V5REF1	Reference Voltage 1	N.A.
	DIAG1 [5:3]		V5REF2	Reference Voltage 2	N.A.
	DIAG1 [8:6]		V5REF3	Reference Voltage 3	N.A.
	DIAG1 [11:10]		VCC	VCC	N.A.
	DIAG1 [13:12]		VCCL	VCCL	N.A.
	DIAG1 [15:14]		VCCL2	VCCL2	N.A.
	DIAG0 [12:10]		V3_3REF4	Reference Voltage 4	N.A.
	DIAG0 [15:13]		V3_3REF5	Reference Voltage 5	N.A.
	DIAG0 [1:0]		VBST	Boost Regulator	N.A.
	DIAG0 [3:2]		VBUCKH	BuckH Regulator	N.A.
	DIAG0 [5:4]		VBUCKL	BuckL Regulator	N.A.
	*/
}

static int Read_Memory(int addr, char *data, int n)
{
	int fail, bsz;
	ccp_cro_t cro;
	ccp_dto_t dto;

	//setMTA
	cro.Params.SetMTA.byMTA = 0x00;
	cro.Params.SetMTA.byAddrExt = 0x00;
	cro.Params.SetMTA.dwAddr = addr;
	fail = ccp_SetMTA(&cro);

	//Dnload
	while ( !fail && (n > 0) ) {
		bsz = (n > 5) ? 5 : n;
		cro.Params.Upload.byByteCnt = bsz;
		fail = ccp_Upload(&cro, &dto);
		memcpy(data, dto.Params.Upload.pbyData, bsz);
		data += bsz;
		n -= bsz;
	}

	return fail;
}

/* ram or eeprom write, its distinguished by addr range:
0x8000xxxx	emulated eeprom access
others		ram access
*/
static int Write_Memory(int addr, const char *data, int n)
{
	int fail, bsz, eeflag;
	ccp_cro_t cro;
	eeflag = ((addr & 0xffff0000) == 0x80000000) ? 1 : 0;

	cro.Params.SetMTA.byMTA = 0x00;
	cro.Params.SetMTA.byAddrExt = 0x00;
	cro.Params.SetMTA.dwAddr = eeflag ? INBOX_ADDR : addr;
	fail = ccp_SetMTA(&cro);

	if(!fail && eeflag) {
		cro.Params.Dnload6.pbyData[0] = TESTID_EEWRITE;
		cro.Params.Dnload6.pbyData[1] = n;
		cro.Params.Dnload6.pbyData[2] = (char) (addr >> 24);
		cro.Params.Dnload6.pbyData[3] = (char) (addr >> 16);
		cro.Params.Dnload6.pbyData[4] = (char) (addr >> 8);
		cro.Params.Dnload6.pbyData[5] = (char) (addr >> 0);
		fail = ccp_Dnload6(&cro, NULL);
	}

	while ( !fail && (n > 0) ) {
		bsz = (n > 5) ? 5 : n;
		cro.Params.Dnload.byByteCnt = bsz;
		memcpy(cro.Params.Dnload.pbyData, data, bsz);
		fail = ccp_Dnload(&cro, NULL);
		data += bsz;
		n -= bsz;
	}

	//execute the test
	if(!fail && ((addr == INBOX_ADDR) || eeflag)) {
		cro.Params.ActionService.bySvc = 0x00;
		cro.Params.ActionService.pbyParams[0] = 0x04;
		cro.Params.ActionService.pbyParams[1] = 0x80;
		cro.Params.ActionService.pbyParams[2] = 0x00;
		cro.Params.ActionService.pbyParams[3] = 0xc0;
		cro.Params.ActionService.pbyParams[4] = 0x00;
		fail = ccp_ActionService(&cro, 5, 1, NULL);
	}

	return fail;
}


int write_nest_psv(int argc, char *cond_flag[]){
	if(argc == 3){
		unsigned int temp;
		sscanf(cond_flag[2],"%2X",&temp);
		mfg_data.psv = (char)temp;
		unsigned int addr = MFGDAT_ADDR + (int) &((struct mfg_data_s *)0) -> psv;
		int fail = Write_Memory(addr, &mfg_data.psv, 1);
		if(fail){
			nest_error_set(CAN_FAIL, "Eeprom Write");
			nest_message("nest cond_flag write error\n");
			return -1;
		}
	}
	nest_message("nest cond_flag 0x%02x \n",((int)mfg_data.psv) & 0xff);
	return 0;
}


static void RAMTest(void)
{
	int fail = 0, saddr, eaddr, i, addr;
	saddr = 0xc0000000;
	eaddr = 0xc0000fff;
	if(nest_fail())
		return;

	nest_message("#RAM Test Start ... \n");
	mailbox[0] = TESTID_RAMTEST;
	mailbox[1] = 0x00; //not care
	mailbox[2] = (char) (saddr >> 24);
	mailbox[3] = (char) (saddr >> 16);
	mailbox[4] = (char) (saddr >> 8);
	mailbox[5] = (char) (saddr >> 0);
	mailbox[6] = (char) (eaddr >> 24);
	mailbox[7] = (char) (eaddr >> 16);
	mailbox[8] = (char) (eaddr >> 8);
	mailbox[9] = (char) (eaddr >> 0);
	fail += Write_Memory(INBOX_ADDR, mailbox, 10);
	if(!fail) {
		fail += Read_Memory(OUTBOX_ADDR, mailbox, 4);
		for(addr = mailbox[0], i = 1; i < 4; i ++) {
			addr <<= 8;
			addr |= mailbox[i];
		}
		if(addr)
			nest_message("RAM Test Fail at 0x%08x\n", addr);
	}

	if (fail) {
		nest_error_set(RAM_FAIL, "RAM Test");
	}

	//restart the dut
	nest_power_reboot();
	ccp_Init(&can1, 500000);
	return;
}

static void FaultTest(void)
{
	int fail = 0;
	if(nest_fail())
		return;

	vsep_init();
	phdh_init();
	switch(bmr) {
	case BM_28077390:
		vsep_mask("PCH03");
		vsep_mask("PCH04");
		vsep_mask("PCH05");
		vsep_mask("PCH14");
		vsep_mask("PCH18");
		vsep_mask("PCH24");
		vsep_mask("PCH25");
		vsep_mask("PCH26");
		vsep_mask("PCH27");
        break;
	default:
		break;
	}

	nest_message("#VSEP Open/Short to GND Test Start ... \n");
	mailbox[0] = TESTID_VSEPTST;
	mailbox[1] = 0x00; //open or short to gnd test
	fail += Write_Memory(INBOX_ADDR, mailbox, 2);
	nest_mdelay(5000);
	fail += Read_Memory(OUTBOX_ADDR, mailbox, 8);
	fail += vsep_verify(mailbox);

	nest_message("#VSEP Short to VBAT Test Start ... \n");
	mailbox[0] = TESTID_VSEPTST;
	mailbox[1] = 0x01; //short to battery test
	fail += Write_Memory(INBOX_ADDR, mailbox, 2);
	nest_mdelay(5000);
	fail += Read_Memory(OUTBOX_ADDR, mailbox, 8);
	fail += vsep_verify(mailbox);

	/*if(bmr != BM_28122746) {
		nest_message("#PHDH Fault Test Start ...\n");
		mailbox[0] = TESTID_PHDHTST;
		mailbox[1] = 0x00; //open fault test
		fail += Write_Memory(INBOX_ADDR, mailbox, 2);
		nest_mdelay(5000);
		fail += Read_Memory(OUTBOX_ADDR, mailbox, 1);
		fail += phdh_verify(mailbox);
	}
*/
	if (fail) {
		nest_error_set(FB_FAIL, "Fault Byte Test");
	}

	//restart the dut
	nest_power_reboot();
	ccp_Init(&can1, 500000);
	return;
}

static void CyclingTest(void)
{
	int fail = 0, min, deadline;
	if(nest_fail())
		return;

	if((bmr!= BM_28164665) && (bmr != BM_28190870)) //ignore other nest, requested by jingfeng
		return;

	//cyc ign, necessary???
	if(!nest_ignore(RLY)) {
		for(int cnt = 0; cnt < 75; cnt ++) {
			RELAY_IGN_SET(0);
			nest_mdelay(100);
			RELAY_IGN_SET(1);
			nest_mdelay(100);
		}
	}

	hfps_init();
	hfps_trap("STATUS_H", 0xff, 0x2f);
	hfps_trap("STATUS_L", 0xff, 0x71);
	phdh_init();
	vsep_init();
	burn_init();

	switch(bmr) {
	case BM_28077390:
	case BM_28119979:
	case BM_28164665:
	case BM_28180087:
	case BM_28264387:
		burn_mask(BURN_CH_COILC);
		burn_mask(BURN_CH_COILD);
		vsep_mask("PCH03"); //2 way IGBT
		vsep_mask("PCH04"); //2 way IGBT
	case BM_28159907:
	case BM_28190870:
	case BM_28249355:
		vsep_mask("PCH13"); //FPR Short to Ground?
		vsep_mask("PCH14"); //SMR Short to Ground?
		vsep_mask("PCH27"); //MPR Short to Ground?
		vsep_mask("PCH17"); //LEGR Short to Battery, not committed by delphi!
		vsep_mask("PCH18"); //VVT1 Short to Battery, not committed by delphi!
		vsep_mask("PCH24"); //O2HTRD Short to Ground, not committed by delphi!
		vsep_mask("PCH25"); //EVAP Short to Battery, not committed by delphi!
		break;
	default:
		break;
	}

	nest_message("#Output Cycling Test Start ... \n");
	ccp_Init(&can1, 500000);
	mailbox[0] = TESTID_OCYLTST;
	mailbox[1] = 0x00; //cycle ETC/EST in sequencial mode
	if(bmr == BM_DK245105 || bmr == BM_28180087 || bmr == BM_28159907 || bmr == BM_28164665)
		mailbox[1] = 0x01; //cycle ETC/EST in paired mode
	else if(bmr == BM_28077390 || bmr == BM_28119979)
		mailbox[1] = 0x03; //cycle IAC/EST in paired mode
	fail += Write_Memory(INBOX_ADDR, mailbox, 2);
	for(min = 0; min < CND_PERIOD; min ++) {
		//delay 1 min
		deadline = nest_time_get(1000 * 60);
		while(nest_time_left(deadline) > 0) {
			nest_update();
			nest_light(RUNNING_TOGGLE);
			fail = burn_verify(mfg_data.vp, mfg_data.ip);
			if(fail) {
				nest_error_set(PULSE_FAIL, "Cycling");
				break;
			}
		}

		if(!fail) {
			nest_message("T = %02d min\n", min);
			fail += Read_Memory(OUTBOX_ADDR, mailbox, 0x13);
			fail += vsep_verify(mailbox + (0x1084 - 0x1080)); /*verify [1084 - 108b]*/
			fail += hfps_verify(mailbox + (0x108c - 0x1080)); /*verify [108c - 1091]*/
			fail += phdh_verify(mailbox + (0x1092 - 0x1080)); /*verify [1092 - 1092]*/
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
	ccp_Init(&can1, 500000);
}

void TestStart(void)
{
	int fail = 0;

	//wait for dut insertion
	nest_wait_plug_in();
	nest_power_on();

	//get dut info through can bus
	nest_can_sel(DW_CAN);
	ccp_Init(&can1, 500000);
	fail = Read_Memory(MFGDAT_ADDR, (char *) &mfg_data, sizeof(mfg_data));
	if(fail) {
		nest_error_set(CAN_FAIL, "CAN");
		return;
	}

	if(0) {
		memcpy(mfg_data.bmr, "28164665", sizeof(mfg_data.bmr));
		mfg_data.psv = 0x03;
		Write_Memory(MFGDAT_ADDR, (char *) &mfg_data, sizeof(mfg_data));
	}

	//check base model nr
	mfg_data.rsv1[0] = 0;
	nest_message("DUT S/N: %s\n", mfg_data.bmr);
	bmr = BM_28180087; //tricky, debug dut
	if(!nest_ignore(BMR)) {
		bmr = nest_map(bmr_map, mfg_data.bmr);
		if(bmr < 0) {
			nest_error_set(MTYPE_FAIL, "Model Type");
			return;
		}
	}

	//relay settings
	cncb_signal(SIG1,SIG_LO); //C71 FPR LOAD6(30Ohm + 70mH) JMP1 = GND, HSD
	cncb_signal(SIG2,SIG_LO); //C70 SMR LOAD7(30Ohm + 70mH) JMP2 = GND, HSD
	cncb_signal(SIG3,SIG_LO); //E7 = NC
	cncb_signal(SIG6,SIG_LO); //ETC
	if(bmr == BM_28077390 || bmr == BM_28119979)
		cncb_signal(SIG6,SIG_HI); //IAC

	//chip pinmaps
	chips_bind();

	//preset glvar
	mfg_data.nest_psv = (char) ((nest_info_get() -> id_base) && 0x7f);
	mfg_data.nest_nec = 0x00; //no err
	memset(mfg_data.fb, 0x00, sizeof(mfg_data.fb) + 17); //+vp[4], ip[4]

	//check psv of amb
	if(!nest_ignore(PSV)) {
		if((mfg_data.psv & 0x02) == 0) {
			nest_error_set(PSV_FAIL, "PSV");
			nest_message("PSV addr %04X : %02X \n",MFGDAT_ADDR+(int)&(((struct mfg_data_s *)0)->psv),mfg_data.psv);
			return;
		}
	}
}

void TestStop(void)
{
	int fail = 0, addr, size;
	nest_message("#Test Stop\n");

	//store fault bytes back to dut
	addr = MFGDAT_ADDR + (int) &((struct mfg_data_s *)0) -> fb[0];
	size = sizeof(mfg_data.fb) + 17; //+rsv3 + vp[4] + ip[4]
	fail += Write_Memory(addr, mfg_data.fb, size);

	//store nest_psv & nest_nec
	if(nest_pass())
		mfg_data.nest_psv |= 0x80;
	else
		mfg_data.nest_psv &= 0x7F;
	mfg_data.nest_nec = nest_error_get() -> nec;
	addr = MFGDAT_ADDR + (int) &((struct mfg_data_s *)0) -> nest_psv;
	fail += Write_Memory(addr, &mfg_data.nest_psv, 2);

	if(fail)
		nest_error_set(CAN_FAIL, "Eeprom Write");

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
	nest_message("\nPower Conditioning - MT80\n");
	nest_message("IAR C Version v%x.%x, Compile Date: %s,%s\n", (__VER__ >> 24),((__VER__ >> 12) & 0xfff),  __TIME__, __DATE__);
	nest_message("nest ID:MT80-%03d\n",(*nest_info_get()).id_base);

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
