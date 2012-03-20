#include "lib/nest.h"
#include "chips/tc1762.h"
#include "chips/vsep.h"
#include "chips/hfps.h"
#include "chips/phdh.h"

#define CND_PERIOD	15//unit: miniutes

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
} mfg_data;

//base model id
static int bmr;
static char mailbox[32];

//base model types declaration
enum {
	BM_28314340,
        BM_28325231,
        BM_28323237,
};

//base model number to id maps
const struct nest_map_s bmr_map[] = {
	{"28314340", BM_28314340},
        {"28325231", BM_28325231},
        {"28323237", BM_28323237},
	END_OF_MAP,
};

static void chips_bind(void)
{
	//PWM Output
	vsep_bind("PCH30", "C-48 AC Speed Command/PWM output	55Hz, 50% duty cycle"); //PCH [30]
	vsep_bind("PCH21", "C-50 PWM Out GD21/PWM output	16Hz, 50% duty cycle"); //PCH [21]
	vsep_bind("PCH15", "C-51 PWM Out GD15/PWM output	16Hz, 50% duty cycle"); //PCH [15]
	vsep_bind("PCH16", "C-68 PWM Out GD16/PWM output	16Hz, 50% duty cycle"); //PCH [16]
	vsep_bind("PCH20", "E-33 PWM Out GD20/PWM output	16Hz, 50% duty cycle"); //PCH [20]
	vsep_bind("PCH24", "E-53 PWM Out GD24/PWM output	16Hz, 50% duty cycle"); //PCH [24]
	vsep_bind("PCH22", "E-58 PWM Out GD22/PWM output	16Hz, 50% duty cycle"); //PCH [22]
	vsep_bind("PCH06", "E-57 PWM Out GD6/PWM output	16Hz, 50% duty cycle"); //PCH [06]

	//Discrete Output
	vsep_bind("PCH23", "C-52 Handshake Line Out Resistor	0.5Hz, 80% duty cycle"); //PCH [23]
	vsep_bind("PCH25", "C-60 EV Mode Work Lamp Control LED	0.5Hz, 80% duty cycle"); //PCH [25]
	vsep_bind("PCH26", "C-61 Auto Stop Off Lamp Control LED	0.5Hz, 80% duty cycle"); //PCH [26]
	vsep_bind("PCH27", "C-62 HCU Main Relay Control Relay	0.5Hz, 80% duty cycle"); //PCH [27]
	vsep_bind("PCH29", "C-66 HV Ready Lamp LED		0.5Hz, 80% duty cycle"); //PCH [29]
	vsep_bind("PCH28", "C-67 Vacuum Pump Enable Relay	0.5Hz, 80% duty cycle"); //PCH [28]
	vsep_bind("PCH14", "C-70 HSD Out GD14 Relay		0.5Hz, 80% duty cycle"); //PCH [14]
	vsep_bind("PCH13", "C-71 HSD Out GD13 Relay		0.5Hz, 80% duty cycle"); //PCH [13]
	vsep_bind("PCH05", "E-01 HSD Out GD5 Relay		0.5Hz, 80% duty cycle"); //PCH [05]
	vsep_bind("PCH08", "E-15 Discrete Out GD8 Relay		0.5Hz, 80% duty cycle"); //PCH [08]
	vsep_bind("PCH19", "E-17 HSD Out GD19 Relay		0.5Hz, 80% duty cycle"); //PCH [19]
	vsep_bind("PCH07", "E-31 BMS Enable Resistor		0.5Hz, 80% duty cycle"); //PCH [07]
	vsep_bind("PCH18", "E-54 Water Pump1 Disable Relay	0.5Hz, 80% duty cycle"); //PCH [18]
	vsep_bind("PCH17", "E-59 Water Pump2 Disable Relay	0.5Hz, 80% duty cycle"); //PCH [17]
	vsep_bind("PCH12", "E-63 HV Enable Relay		0.5Hz, 80% duty cycle"); //PCH [12]
	vsep_bind("PCH10", "E-64 Engine Starter Control Relay	0.5Hz, 80% duty cycle"); //PCH [10]
	vsep_bind("PCH09", "E-65 ISG Enable Resistor		0.5Hz, 80% duty cycle"); //PCH [09]
	vsep_bind("PCH11", "E-66 ERAD Enable Resistor		0.5Hz, 80% duty cycle"); //PCH [11]

	//Communication
	//C-14/15 Hybrid CAN Hi/Lo CAN Communication	N.A.	CAN CCP 2.1
	//C-32/16 PT CAN Hi/Lo     CAN Communication	N.A.	CAN CCP 2.1

	/*Power Output
	VKAM5V	VKAM5V, 	N.A.	STATUS [8]
	VKAML2	VKAML2, 	N.A.	STATUS [1]
	V5REF1	Reference Voltage 1	N.A.	DIAG1 [2:0]
	V5REF2	Reference Voltage 2	N.A.	DIAG1 [5:3]
	V5REF3	Reference Voltage 3	N.A.	DIAG1 [8:6]
	VCC	VCC, 	N.A.	DIAG1 [11:10]
	VCCL	VCCL, 	N.A.	DIAG1 [13:12]
	VCCL2	VCCL2, 	N.A.	DIAG1 [15:14]
	V3_3REF4	Reference Voltage 4	N.A.	DIAG0 [12:10]
	V3_3REF5	Reference Voltage 5	N.A.	DIAG0 [15:13]
	BST	Boost Regulator	N.A.	DIAG0 [1:0]
	VBUCKH	BuckH Regulator	N.A.	DIAG0 [3:2]
	VBUCKL	BuckL Regulator	N.A.	DIAG0 [5:4]*/
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
	case BM_28314340:
        break;
        case BM_28325231:
          vsep_mask("PCH13");
        break;
        case BM_28323237:
          vsep_mask("PCH13");
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

	/*if(bmr != BM_28314340) {
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

	hfps_init();
	//hfps_trap("STATUS_H", 0xff, 0x2f);
	//hfps_trap("STATUS_L", 0xff, 0x71);
	phdh_init();
	vsep_init();

	switch(bmr) {
	case BM_28314340:
		break;
        case BM_28325231:
        break;
        case BM_28323237:
        break;
	default:
		break;
	}

	nest_message("#Output Cycling Test Start ... \n");
	mailbox[0] = TESTID_OCYLTST;
	mailbox[1] = 0x00;
	fail += Write_Memory(INBOX_ADDR, mailbox, 2);
	for(min = 0; min < CND_PERIOD; min ++) {
		//delay 1 min
		deadline = nest_time_get(1000 * 60);
		while(nest_time_left(deadline) > 0) {
			nest_update();
			nest_light(RUNNING_TOGGLE);
		}

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
#if 0
	//cyc ign, necessary???
	for(int cnt = 0; cnt < 75; cnt ++) {
		RELAY_IGN_SET(0);
		nest_mdelay(100);
		RELAY_IGN_SET(1);
		nest_mdelay(100);
	}
#endif

	//get dut info through can bus
	nest_can_sel(DW_CAN);
	ccp_Init(&can1, 500000);

#if 0
	memcpy(mfg_data.bmr, "28314340", 8);
	mfg_data.psv = 3; //pass ict&amb
	fail = Write_Memory(MFGDAT_ADDR, (char *) &mfg_data, 0x20); //sizeof(mfg_data));
	if(fail)
		nest_error_set(CAN_FAIL, "Eeprom Write");

	memset(mailbox, 0, 0x20);
	fail += Read_Memory(INBOX_ADDR, mailbox, 0x20);
        fail += Read_Memory(OUTBOX_ADDR, mailbox, 0x20);
#endif

	fail = Read_Memory(MFGDAT_ADDR, (char *) &mfg_data, sizeof(mfg_data));
	if(fail) {
		nest_error_set(CAN_FAIL, "CAN");
		return;
	}

	//check base model nr
	mfg_data.rsv1[0] = 0;
	nest_message("DUT S/N: %s\n", mfg_data.bmr);
	bmr = nest_map(bmr_map, mfg_data.bmr);
	if(bmr < 0) {
		nest_error_set(MTYPE_FAIL, "Model Type");
		return;
	}

	//chip pinmaps
	chips_bind();

	//preset glvar
	mfg_data.nest_psv = (char) ((nest_info_get() -> id_base) & 0x7f);
	mfg_data.nest_nec = 0x00; //no err
	memset(mfg_data.fb, 0x00, sizeof(mfg_data.fb));

	//check psv of amb
	if((mfg_data.psv & 0x02) == 0) {
		nest_error_set(PSV_FAIL, "PSV");
		return;
	}
}

void TestStop(void)
{
	int fail = 0, addr;
	nest_message("#Test Stop\n");

	//store fault bytes back to dut
	addr = MFGDAT_ADDR + (int) &((struct mfg_data_s *)0) -> fb[0];
	fail += Write_Memory(addr, mfg_data.fb, sizeof(mfg_data.fb));

	//store nest_psv & nest_nec
	mfg_data.nest_psv |= 0x80;
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
	nest_message("\nPower Conditioning - GAC HCU\n");
	nest_message("IAR C Version v%x.%x, Compile Date: %s,%s\n", (__VER__ >> 24),((__VER__ >> 12) & 0xfff),  __TIME__, __DATE__);

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
