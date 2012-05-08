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
	unsigned short ip[4]; //0x8040 - 0x8047
	unsigned char wp[4]; //0x8048 - 0x804b, peak width, unit: uS, resolution: 1uS
} mfg_data;

//base model id
static int bmr;
static char mailbox[32];

//base model types declaration
enum {
	/*2IGBT*/
	/*4IGBT*/
	BM_DK262473_28169026, /*now change to 28220231*/

	/*UNKNOWN*/
	BM_DK262466_28122736,
	BM_DK262480_28209021,
	BM_DK262490_28140820,
	BM_DK262516_28140808,
	BM_DK262522_28140809,
	BM_28122746,
	BM_28211012,
	BM_28169019,
	BM_28219154,
	BM_28216910,
};

//base model number to id maps
const struct nest_map_s bmr_map[] = {
	{"DK262466", BM_DK262466_28122736},
	{"28122736", BM_DK262466_28122736},
	{"DK262473", BM_DK262473_28169026},
	{"28169026", BM_DK262473_28169026},
	{"28220231", BM_DK262473_28169026},
	{"DK262480", BM_DK262480_28209021},
	{"28209021", BM_DK262480_28209021},
	{"DK262490", BM_DK262490_28140820},
	{"28140820", BM_DK262490_28140820},
	{"DK262516", BM_DK262516_28140808},
	{"28140808", BM_DK262516_28140808},
	{"DK262522", BM_DK262522_28140809},
	{"28140809", BM_DK262522_28140809},
	{"28122746", BM_28122746},
	{"28211012", BM_28211012},
	{"28169019", BM_28169019},
	{"28219154", BM_28219154},
	{"28216910", BM_28216910},
	END_OF_MAP,
};

static void chips_bind(void)
{
	//PWM Output
	vsep_bind("PCH17", "J101-98 LEGR"); //Linear EGR, 300Hz, 50% duty cycle
	vsep_bind("PCH06", "J101-95 CCP"); //Canister purge solenoid, 16Hz, 50% duty cycle
	vsep_bind("PCH23", "J101-91 FUEL_LEVEL_GUAGE"); // Fuel level guage, 128Hz, 50% duty cycle
	vsep_bind("PCH22", "J101-89 THERMAL_MGMT"); //Thermal management, 100Hz, 50% duty cycle
	vsep_bind("PCH19", "J101-88 O2HTRA"); //Front oxygen sensor heater, 16Hz 50% duty cycle
	vsep_bind("PCH20", "J101-90 O2HTRB"); //Rear oxygen sensor heater, 16Hz 50% duty cycle
	//Port 0.8	J101-100	GENL TRM	Generator L terminal	128Hz 50% duty cycle
	vsep_bind("PCH16", "J101-85 COOLANT_GAUGE"); //Coolant gauge, 128Hz, 50% duty cycle.
	vsep_bind("PCH18", "J101-99 VVT1"); //Oil Control Valve, 300Hz, 50% duty cycle

	//Pulse/Frequency Output
	vsep_bind("PCH09", "J101-80 INJ1"); //Fuel injector 1, 50Hz, 50% duty cycle.
	vsep_bind("PCH10", "J101-79 INJ 2"); //Fuel injector 2, 50Hz, 50% duty cycle.
	vsep_bind("PCH11", "J101-81 INJ 3"); //Fuel injector 3, 50Hz, 50% duty cycle.
	vsep_bind("PCH12", "J101-78 INJ 4"); //Fuel injector 4, 50Hz, 50% duty cycle.
	vsep_bind("PCH01", "J101-26 Coil A/EST A"); //Up integrated ignition coil A, 50 Hz and 3msec ON time.
	vsep_bind("PCH02", "J101-02 Coil B/EST B"); //Up integrated ignition coil B, 50 Hz and 3msec ON time.
	vsep_bind("PCH03", "J101-01 Coil C/EST C"); //Up integrated ignition coil C, 50 Hz and 3msec ON time.
	vsep_bind("PCH04", "J101-03 Coil D/EST D"); //Up integrated ignition coil D, 50 Hz and 3msec ON time.
	//Port 0.4			J101-23	IACBHI	IACB idle air control	 IACB = 75Hz, 50% duty cycle.
	//			J101-22	IACBLO
	//Output Register [7:5]		J101-49	ETC Motor HI/ IACALO	Electronic throttle control (ETC) / IACA idle air control	ETC = 4khz, 50% duty cycle. Reverse direction every 10 seconds
	//			J101-24	ETC Motor LO/ IACAHI		IACA = 75Hz, 50% duty cycle.
	//N.A			J101-97	Tachometer	Tachometer signal	 250Hz , 50% duty cycle
	vsep_bind("PCH05", "J101-84 Fuel Consumption"); //Fuel consumption / VSS meter	 250Hz, 50% duty cycle

	//Discrete Output
	vsep_bind("PCH21", "J101-76 PDA Actuator"); //PDA Actuator, 128Hz, 50% duty cycle
	vsep_bind("PCH07", "J101-96 FPR_LSD"); //	Fuel pump relay, 0.5 Hz, 80% duty cycle
	vsep_bind("PCH25", "J101-23 Oil Pressure Low Lamp"); //Oil Pressure Low Lamp,  300Hz, 50% duty cycle.
	vsep_bind("PCH28", "J101-52 FAN #1"); //Fan control 1, 0.5 Hz, 80% duty cycle
	vsep_bind("PCH29", "J101-51 FAN #2"); // Relay Fan control 2, 0.5 Hz, 80% duty cycle
	vsep_bind("PCH13", "J101-86 FPR"); //Fuel pump relay, 0.5 Hz, 80% duty cycle
	vsep_bind("PCH30", "J101-92 MIL"); //Malfunctions indicator lamp, 0.5 Hz, 80% duty cycle
	vsep_bind("PCH15", "J101-77 ACCR"); //AC clutch relay, 0.5 Hz, 80% duty cycle
	vsep_bind("PCH27", "J101-94 MPR"); //Main power relay, 0.5 Hz, 80% duty cycle
	vsep_bind("PCH08", "J101-83 VGIS"); //Variable Geometry Intake Solenoid, 0.5 Hz, 80% duty cycle
	vsep_bind("PCH26", "J101-82 SVS"); //Upshift Indication Teltale/ SVS Lamp	0.5 Hz, 80% duty cycle
	vsep_bind("PCH14", "J101-87 SMR"); //Starter control relay, 0.5 Hz, 80% duty cycle
	vsep_bind("PCH24", "J101-93 EVAP Diag Solenoid"); //Evaporative diagnostic solenoid (Canister vent solenoid), 0.5 Hz, 80% duty cycle

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
	case BM_DK262466_28122736:
		vsep_mask("PCH18");
		vsep_mask("PCH22");
		vsep_mask("PCH24");
		vsep_mask("PCH25");
		vsep_mask("PCH26");
		break;
	case BM_28219154:
	case BM_28216910:
	case BM_DK262473_28169026: //28220231
	case BM_DK262480_28209021:
		vsep_mask("PCH05");
		vsep_mask("PCH14");
		vsep_mask("PCH16");
		vsep_mask("PCH17");
		vsep_mask("PCH22");
		vsep_mask("PCH24");
		vsep_mask("PCH25");
		vsep_mask("PCH26");
		break;
	case BM_DK262490_28140820:
		vsep_mask("PCH08");
		vsep_mask("PCH22");
		vsep_mask("PCH24");
		vsep_mask("PCH26");
	case BM_DK262516_28140808:
		vsep_mask("PCH03");
		vsep_mask("PCH04");
		vsep_mask("PCH05");
		vsep_mask("PCH16");
		vsep_mask("PCH18");
		vsep_mask("PCH25");
		break;
	case BM_DK262522_28140809:
		vsep_mask("PCH14");
		vsep_mask("PCH18");
		vsep_mask("PCH25");
		break;
	case BM_28122746:
		vsep_mask("PCH03");
		vsep_mask("PCH04");
		vsep_mask("PCH25");
		break;
	case BM_28211012:
		vsep_mask("PCH03");
		vsep_mask("PCH04");
		break;
	case BM_28169019:
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

	if(bmr != BM_28122746) {
		nest_message("#PHDH Fault Test Start ...\n");
		mailbox[0] = TESTID_PHDHTST;
		mailbox[1] = 0x00; //open fault test
		fail += Write_Memory(INBOX_ADDR, mailbox, 2);
		nest_mdelay(5000);
		fail += Read_Memory(OUTBOX_ADDR, mailbox, 1);
		fail += phdh_verify(mailbox);
	}

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
	int fail = 0, min, i, deadline;
	if(nest_fail())
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
	hfps_trap("STATUS_L", 0xff, 0x61);
	phdh_init();
	vsep_init();
	burn_init();

	switch(bmr) {
	case BM_DK262466_28122736:
		vsep_mask("PCH01");
		vsep_mask("PCH02");
		vsep_mask("PCH03");
		vsep_mask("PCH04");
		vsep_mask("PCH13");
		vsep_mask("PCH14");
		vsep_mask("PCH18");
		vsep_mask("PCH22");
		vsep_mask("PCH24");
		vsep_mask("PCH25");
		vsep_mask("PCH26");
		vsep_mask("PCH27");
		break;
	case BM_DK262473_28169026: //28220231
	case BM_DK262480_28209021:
		vsep_mask("PCH05");
		vsep_mask("PCH13");
		vsep_mask("PCH14");
		vsep_mask("PCH16");
		vsep_mask("PCH17");
		vsep_mask("PCH22");
		vsep_mask("PCH24");
		vsep_mask("PCH25");
		vsep_mask("PCH26");
		vsep_mask("PCH27");
		break;
	case BM_DK262490_28140820:
		vsep_mask("PCH03");
		vsep_mask("PCH04");
		vsep_mask("PCH05");
		vsep_mask("PCH08");
		vsep_mask("PCH16");
		vsep_mask("PCH18");
		vsep_mask("PCH22");
		vsep_mask("PCH24");
		vsep_mask("PCH25");
		vsep_mask("PCH26");
		vsep_mask("PCH29");

	case BM_DK262516_28140808:
		vsep_mask("PCH03");
		vsep_mask("PCH04");
		vsep_mask("PCH05");
		vsep_mask("PCH16");
		vsep_mask("PCH18");
		vsep_mask("PCH25");
		vsep_mask("PCH26");
		vsep_mask("PCH29");
		break;
	case BM_DK262522_28140809:
		vsep_mask("PCH14");
		vsep_mask("PCH18");
		vsep_mask("PCH25");
		vsep_mask("PCH26");
		vsep_mask("PCH29");
		break;
	case BM_28122746:
		vsep_mask("PCH25");
		vsep_mask("PCH26");
		vsep_mask("PCH29");
		break;
	case BM_28211012:
		vsep_mask("PCH25");
		vsep_mask("PCH26");
		vsep_mask("PCH29");
		break;
	case BM_28169019:
		vsep_mask("PCH25");
		vsep_mask("PCH26");
		vsep_mask("PCH29");
		break;
	default:
		break;
	}

	nest_message("#Output Cycling Test Start ... \n");
	mailbox[0] = TESTID_OCYLTST;
	mailbox[1] = 0x00; //cycle ETC/EST in sequencial mode
	if(bmr == BM_DK262490_28140820 || bmr == BM_28211012)
		mailbox[1] = 0x01; //cycle ETC/EST in paired mode
	else if(bmr == BM_28122746)
		mailbox[1] = 0x03; //cycle IAC/EST in paired mode
	fail += Write_Memory(INBOX_ADDR, mailbox, 2);
	for(min = 0; min < CND_PERIOD; min ++) {
		//delay 1 min
		deadline = nest_time_get(1000 * 60);
		while(nest_time_left(deadline) > 0) {
			nest_update();
			nest_light(RUNNING_TOGGLE);
			fail = burn_verify(mfg_data.vp, mfg_data.ip, mfg_data.wp);
			if(fail) {
				nest_error_set(PULSE_FAIL, "Cycling");
				break;
			}
		}

		if(!fail) {
		nest_message("T = %02d min\n", min);
		fail += Read_Memory(OUTBOX_ADDR, mailbox, 0x17);
		fail += vsep_verify(mailbox + (0x1084 - 0x1080)); /*verify [1084 - 108b]*/
		fail += hfps_verify(mailbox + (0x108c - 0x1080)); /*verify [108c - 1091]*/
		fail += phdh_verify(mailbox + (0x1092 - 0x1080)); /*verify [1092 - 1092]*/
		for(i = 0x1093; i < 0x1097; i ++) {  /*verify [1093 - 1096]*/
			if(mailbox[i - 0x1080] != 0) {
				nest_message("Undefined Fault, [0xd000%04x] = %02x\n", i, mailbox[i - 0x1080]);
				fail += -1;
			}
		}

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
	nest_message("Switch to ETC load type\n");
	cncb_signal(SIG1, SIG_HI); //J101-22 IACBHI = NC
	cncb_signal(SIG2, SIG_HI); //J101-23 IACBLO = LOAD1B(12Ohm + 470uH)
	cncb_signal(SIG6, SIG_HI); //J101-49 ETCM_OP|IACALO = LOAD27B(1.25Ohm + 8mH)

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
	bmr = nest_map(bmr_map, mfg_data.bmr);
	if(bmr < 0) {
		nest_error_set(MTYPE_FAIL, "Model Type");
		return;
	}

	//relay settings
	if(bmr == BM_28122746) { //iac
		nest_message("Switch to IAC load type\n");
		cncb_signal(SIG1, SIG_LO); // J101-22 IACBHI = LOAD9A
		cncb_signal(SIG2, SIG_LO); // J101-23 IACBLO = JMP1A
		cncb_signal(SIG6, SIG_LO); // J101-49 ETCM_OP|IACALO = LOAD19B(39Ohm + 32mH)
	}

	//chip pinmaps
	chips_bind();

	//preset glvar
	mfg_data.nest_psv = (char) ((nest_info_get() -> id_base) & 0x7f);
	mfg_data.nest_nec = 0x00; //no err
	memset(mfg_data.fb, 0x00, sizeof(mfg_data.fb));

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
	size = sizeof(mfg_data.fb) + 21; //+rsv3 + vp[4] + ip[4] + wp[4]
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
	nest_message("\nPower Conditioning - MT60\n");
	nest_message("IAR C Version v%x.%x, Compile Date: %s,%s\n", (__VER__ >> 24),((__VER__ >> 12) & 0xfff),  __TIME__, __DATE__);
	nest_message("nest ID:MT60-%03d\n",(*nest_info_get()).id_base);

	while(1){
		TestStart();
		RAMTest();
		FaultTest();
		CyclingTest();
		FaultTest();
		RAMTest();
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