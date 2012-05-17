/*
 * port from mt80.c
*/
#include "lib/nest.h"
#include "chips/vsep.h"

#define CND_PERIOD	15 //unit: miniutes

#define VECTOR_SW_VERSION	0x80170000
#define VECTOR_FLASH_WRITE	0x80170004
#define VECTOR_FLASH_ERASE	0x80170008
#define VECTOR_FAULTS_CLEAR	0x80170014
#define VECTOR_FLASH_READ	0x8017001C
#define VECTOR_RAM_TEST		0x80170028
#define VECTOR_CHANGE_MODE	0x80170040
#define VECTOR_PC_OCTEST	0x80170044
#define VECTOR_INPUT_POLL	0x80170048
#define VECTOR_EEPROM_WRITE	0x8017005C

#define INBOX_ADDR	(0xD0008700) //D4 D5 D6 D7 A4 A5 A6 ..(256B)
#define OUTBOX_ADDR	(0xD0008800) //D2 D3 A2 ..(256B)
#define MFGDAT_ADDR	(0x80008000)
#define POLDAT_ADDR	(0xD0008300)
#define POLDAT_BYTES	(0x26)

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
	char fb[POLDAT_BYTES]; //0x8020 - 0x8045 fault bytes write back
} mfg_data;

#define D4_ADDR 0xD0008700
#define D5_ADDR 0xD0008704
#define D6_ADDR 0xD0008708
#define D7_ADDR 0xD000870c
#define A4_ADDR 0xD0008710
#define A5_ADDR 0xD0008714
#define A6_ADDR 0xD0008718

#define D2_ADDR 0xD0008800
#define D3_ADDR 0xD0008804
#define A2_ADDR 0xD0008808

//base model id
static int bmr;
static union {
	struct {
		unsigned d4; //0xD0008700
		unsigned d5; //0xD0008704
		unsigned d6; //0xD0008708
		unsigned d7; //0xD000870c
		unsigned a4; //0xD0008710
		unsigned a5; //0xD0008714
		unsigned a6; //0xD0008718
	} inbox;
	struct {
		unsigned d2;
		unsigned d3;
		unsigned a2;
	} outbox;
	char bytes[64];
} mailbox;

const struct mcamos_s mcamos_mt92 = {
	.can = &can1,
	.baud = 500000,
	.id_cmd = 0x7EE,
	.id_dat = 0x7EF,
	.timeout = 50,
};

//base model types declaration
enum {
	BM_DK277130,
	BM_28351894,
	BM_28351885,
	BM_28236632,
};

//base model number to id maps
const struct nest_map_s bmr_map[] = {
	{"DK277130", BM_DK277130},
	{"28351894", BM_28351894},
	{"28351885", BM_28351885},
	{"28236632", BM_28236632},
	END_OF_MAP,
};

//little endian -> big endian
unsigned htonl(unsigned data)
{
	union {
		unsigned data;
		char bytes[4];
	} x, y;
	x.data = data;
	y.bytes[0] = x.bytes[3];
	y.bytes[1] = x.bytes[2];
	y.bytes[2] = x.bytes[1];
	y.bytes[3] = x.bytes[0];
	return y.data;
}

unsigned ntohl(unsigned data)
{
	union {
		unsigned data;
		char bytes[4];
	} x, y;
	x.data = data;
	y.bytes[0] = x.bytes[3];
	y.bytes[1] = x.bytes[2];
	y.bytes[2] = x.bytes[1];
	y.bytes[3] = x.bytes[0];
	return y.data;
}

void ntoh_array(void *p, int bytes)
{
	unsigned short *array = p;
	unsigned short hb, lb;
	int i, n;

	n = bytes / 2;
	for(i = 0; i < n; i ++) {
		hb = (array[i] >> 8) & 0xff;
		lb = (array[i] >> 0) & 0xff;
		array[i] = (lb << 8) | hb;
	}
}

void dump(unsigned addr, const void *p, int bytes)
{
	unsigned v, i;
	const unsigned char *pbuf = p;
	for(i = 0; i < bytes; i ++) {
			v = (unsigned) (pbuf[i] & 0xff);
			if(i % 16 == 0)
				nest_message("0x%08x: %02x", addr + i, v);
			else if((i % 16 == 15) || (i == bytes - 1))
				nest_message(" %02x\n", v);
			else
				nest_message(" %02x", v);
	}
}

static void chips_bind(void)
{
	vsep_bind("PCH01", "E-04 ESTCOILA");
	vsep_bind("PCH02", "E-20 ESTCOILB");
	vsep_bind("PCH03", "E-03 ESTCOILC");
	vsep_bind("PCH04", "E-05 ESTCOILD");
	vsep_bind("PCH05", "E-58 VCP_Intake");
	vsep_bind("PCH06", "E-44 VCP_Exhaust");
	vsep_bind("PCH07", "E-59 VVL(INTAKE)/RES OUT");
	vsep_bind("PCH08", "E-57 LEGR/VCM PWM");
	vsep_bind("PCH09", "N.C.");
	vsep_bind("PCH10", "N.C.");
	vsep_bind("PCH11", "N.C.");
	vsep_bind("PCH12", "N.C.");
	vsep_bind("PCH13", "C-81 CANISTER VENT VALVE");
	vsep_bind("PCH14", "E-29 TURBO RELIEF VALVE");
	vsep_bind("PCH15", "E-13 VGIS");
	vsep_bind("PCH16", "E-28 WASTEGATE VALVE CTRL");
	vsep_bind("PCH17", "C-75 TACHOMETER");
	vsep_bind("PCH18", "C-74 FUEL CONSUMPTION/TRIP COMPUTER");
	vsep_bind("PCH19", "C-51 O2HTRA");
	vsep_bind("PCH20", "C-73 O2HTRB");
	vsep_bind("PCH21", "E-14 CCP");
	vsep_bind("PCH22", "C-85 FAN HI PWM");
	vsep_bind("PCH23", "C-80 STARTRELAY");
	vsep_bind("PCH24", "C-55 FPRLY(HSD)");
	vsep_bind("PCH25", "XXXX WSS CURRENT PROTECT");
	vsep_bind("PCH26", "C-53 CRUISE SET LAMP");
	vsep_bind("PCH27", "C-90 ALTERNATOR PWM/COOLANT GAUGE");
	vsep_bind("PCH28", "C-52 FUEL LEVEL GAUGE");
	vsep_bind("PCH29", "C-91 THERMOSTAT OUTPUT");
	vsep_bind("PCH30", "C-88 IMMO_REQ");
}

enum {
	MODE_MANUFACTURE_OC = 1,
	MODE_VALIDATION_OC,
	MODE_INPUT_POLLING,
	MODE_CONDITIONING_OC,
};

static int Change_Mode(int mode)
{
	nest_mdelay(100);
	memset(mailbox.bytes, 0, sizeof(mailbox));
	mailbox.inbox.d4 = htonl(mode);
	int fail = mcamos_dnload_ex(INBOX_ADDR, mailbox.bytes, sizeof(mailbox.inbox));
	if(fail)
		return -1;

	nest_mdelay(100);
	fail = mcamos_execute_ex(VECTOR_CHANGE_MODE);
	if(fail)
		return -2;

	nest_mdelay(100);
	memset(mailbox.bytes, 0, sizeof(mailbox));
	fail = mcamos_upload_ex(OUTBOX_ADDR, mailbox.bytes, sizeof(mailbox.outbox));
	if(fail)
		return -3;

	if(ntohl(mailbox.outbox.d2) != mode)
		return -4;

	return 0;
}

static int Read_Memory(int addr, char *data, int n)
{
	int fail, is_ram = ((addr & 0xFFFF0000) == 0xD0000000);
	if(is_ram) {
		fail = mcamos_upload_ex(addr, data, n);
		dump(addr, data, n);
		return fail;
	}

	//read data from program flash or data flash
	memset(mailbox.bytes, 0, sizeof(mailbox));
	mailbox.inbox.d4 = htonl(n);
	mailbox.inbox.a4 = htonl(addr);
#if 1 /*mt92 is too much garbage!!!*/
	fail = mcamos_dnload_ex(D4_ADDR, &mailbox.inbox.d4, 4);
	fail += mcamos_dnload_ex(A4_ADDR, &mailbox.inbox.a4, 4);
#else
	fail = mcamos_dnload_ex(INBOX_ADDR, mailbox.bytes, sizeof(mailbox.inbox));
#endif
	if(fail)
		return -1;

	nest_mdelay(100);
	fail = mcamos_execute_ex(VECTOR_FLASH_READ);
	if(fail)
		return -2;

	nest_mdelay(100);

	//dummy read
	memset(mailbox.bytes, 0, sizeof(mailbox));
	fail = mcamos_upload_ex(OUTBOX_ADDR, mailbox.bytes, sizeof(mailbox.outbox));
	if(fail)
		return -3;

	nest_mdelay(100);
	fail = mcamos_upload_ex(OUTBOX_ADDR + sizeof(mailbox.outbox), data, n);
	dump(addr, data, n);
	return (fail) ? -4 : 0;
}

static int Write_Memory(int addr, const char *data, int n)
{
	int fail, is_ram = (addr & 0xFFFF0000 == 0xD0000000);
	dump(addr, data, n);
	if(is_ram) {
		fail = mcamos_dnload_ex(addr, data, n);
		return fail;
	}

	memset(mailbox.bytes, 0, sizeof(mailbox));
	mailbox.inbox.d4 = htonl(n);
	mailbox.inbox.a4 = htonl(addr);
#if 1 /*mt92 is too much garbage!!!*/
	fail = mcamos_dnload_ex(D4_ADDR, &mailbox.inbox.d4, 4);
	fail += mcamos_dnload_ex(A4_ADDR, &mailbox.inbox.a4, 4);
#else
	fail = mcamos_dnload_ex(INBOX_ADDR, mailbox.bytes, sizeof(mailbox.inbox));
#endif
	if(fail)
		return -1;

	nest_mdelay(100);

	//dnload data to mt92, each time 8 bytes at most, mt92 garbage
	for(int i, bytes = i = 0; i < n; i += bytes) {
		bytes = n - i;
		bytes = (bytes >= 8) ? 8 : bytes;
		fail = mcamos_dnload_ex(INBOX_ADDR + sizeof(mailbox.inbox) + i, data + i, bytes);
		if(fail)
			return -2;
	}

	nest_mdelay(100);
	fail = mcamos_execute_ex(VECTOR_EEPROM_WRITE);
	if(fail)
		return -3;

	nest_mdelay(500);
	memset(mailbox.bytes, 0, sizeof(mailbox));
	fail = mcamos_upload_ex(OUTBOX_ADDR, mailbox.bytes, sizeof(mailbox.outbox));
	if(fail)
		return -4;

	if(ntohl(mailbox.outbox.d2) != 0x00003333)
		return -5;

	return 0;
}

static void RAMTest(void)
{
	int fail, addr = 0;
	if(nest_fail())
		return;

	nest_message("#RAM Test Start ... \n");
	fail = mcamos_execute_ex(VECTOR_RAM_TEST);
	if(!fail) {
		nest_mdelay(100);
		fail = mcamos_upload_ex(OUTBOX_ADDR, mailbox.bytes, sizeof(mailbox.outbox));
		if(!fail) {
			fail = (ntohl(mailbox.outbox.d2) == 0x00003333) ? 0 : -1;
			addr = (fail) ? ntohl(mailbox.outbox.a2) : 0;
		}
	}

	if(fail) {
		if(addr != 0)
			nest_message("RAM Test Fail at 0x%08x\n", addr);
		nest_error_set(RAM_FAIL, "RAM Test");
	}

	//restart the dut
	nest_power_reboot();
	return;
}

static void CyclingTest(void)
{
	int fail, min, deadline;
	if(nest_fail())
		return;

	//cyc ign, necessary???
	if(0) { //!nest_ignore(RLY)) {
		for(int cnt = 0; cnt < 75; cnt ++) {
			RELAY_IGN_SET(0);
			nest_mdelay(100);
			RELAY_IGN_SET(1);
			nest_mdelay(100);
		}
	}

	vsep_init();
	//pls to do mask here
	switch(bmr) {
	case BM_DK277130:
		vsep_mask("PCH09"); //NC
		vsep_mask("PCH10"); //NC
		vsep_mask("PCH11"); //NC
		vsep_mask("PCH12"); //NC
		break;
	default:
		break;
	}

	nest_message("#Output Cycling Test Start ... \n");
	fail = Change_Mode(MODE_CONDITIONING_OC);
	if(fail) {
		nest_error_set(PULSE_FAIL, "Enter into Cycling mode");
		return;
	}

	for(min = 0; min < CND_PERIOD; min ++) {
		//clear faults
		mcamos_execute_ex(VECTOR_FAULTS_CLEAR);

		//delay 1 min
		deadline = nest_time_get(1000 * 10);
		while(nest_time_left(deadline) > 0) {
			nest_update();
			nest_light(RUNNING_TOGGLE);
		}

		nest_message("T = %02d min\n", min);
		fail = Read_Memory(POLDAT_ADDR, mailbox.bytes, POLDAT_BYTES);
		ntoh_array(mailbox.bytes, POLDAT_BYTES);
		fail += vsep_verify(mailbox.bytes + 0x08); /*verify [D0008308 - D000831f]*/
		if (fail) {
			memcpy(mfg_data.fb, mailbox.bytes + 0x08, sizeof(mfg_data.fb));
			nest_error_set(FB_FAIL, "Cycling");
			//break;
		}
	}

	//restart the dut
	nest_power_reboot();
}

void TestStart(void)
{
	int fail = 0;

	//wait for dut insertion
	nest_wait_plug_in();
	nest_power_on();

	//get dut info through can bus
	nest_can_sel(DW_CAN);
	mcamos_init_ex(&mcamos_mt92);

	//try to fetch dut software version info
	memset(mailbox.bytes, 0, sizeof(mailbox));
	mcamos_execute_ex(VECTOR_SW_VERSION);
	nest_mdelay(100);
	mcamos_upload_ex(OUTBOX_ADDR + sizeof(mailbox.outbox), mailbox.bytes, 32);
	nest_message("DUT SW: %s\n", mailbox.bytes);

	if(0) {
		static const char mfg_data_dk277130[] = {
			0x32, 0x31, 0x36, 0x31, 0x30, 0x30, 0x33, 0x39,
			0x30, 0x44, 0x4b, 0x32, 0x37, 0x37, 0x31, 0x33,
			0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		};
		memcpy(&mfg_data, &mfg_data_dk277130, sizeof(mfg_data_dk277130));
		mfg_data.psv = 0x03;
		Write_Memory(MFGDAT_ADDR, (char *) &mfg_data, sizeof(mfg_data));
	}

	//read mfg data
	memset((void *) &mfg_data, 0, sizeof(mfg_data));
	fail = Read_Memory(MFGDAT_ADDR, (char *) &mfg_data, sizeof(mfg_data));
	if(fail) {
		nest_error_set(CAN_FAIL, "CAN");
		return;
	}

	//check base model nr
	mfg_data.rsv1[0] = 0;
	nest_message("DUT S/N: %s\n", mfg_data.bmr);
	if(!nest_ignore(BMR)) {
		bmr = nest_map(bmr_map, mfg_data.bmr);
		if(bmr < 0) {
			nest_error_set(MTYPE_FAIL, "Model Type");
			return;
		}
	}

	//relay settings
	if(1) {
		//default 4 cyl
		cncb_signal(SIG1,SIG_LO); //C71 FPR LOAD6(30Ohm + 70mH) JMP1 = GND, HSD
		cncb_signal(SIG2,SIG_LO); //C70 SMR LOAD7(30Ohm + 70mH) JMP2 = GND, HSD
		cncb_signal(SIG3,SIG_LO); //E7 = NC
		if(bmr == BM_28351894) { //3 cyl
			cncb_signal(SIG1,SIG_HI); //C71 FPR LOAD6(30Ohm + 70mH) JMP1 = GND, HSD
			cncb_signal(SIG2,SIG_HI); //C70 SMR LOAD7(30Ohm + 70mH) JMP2 = GND, HSD
			cncb_signal(SIG3,SIG_HI); //E7 = NC
		}
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
	int fail = 0;
	nest_message("#Test Stop\n");

	//store nest_psv & nest_nec
	if(nest_pass())
		mfg_data.nest_psv |= 0x80;
	else
		mfg_data.nest_psv &= 0x7F;
	mfg_data.nest_nec = nest_error_get() -> nec;
	//for(int i = 0; i < sizeof(mfg_data); i ++) *((char *)&mfg_data + i) = i;
	fail += Write_Memory(MFGDAT_ADDR, (void *)&mfg_data, sizeof(mfg_data));
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
	nest_message("\nPower Conditioning - MT92\n");
	nest_message("IAR C Version v%x.%x, Compile Date: %s,%s\n", (__VER__ >> 24),((__VER__ >> 12) & 0xfff),  __TIME__, __DATE__);
	nest_message("nest ID:MT92-%03d\n",(*nest_info_get()).id_base);

	while(1){
		TestStart();
		//RAMTest();
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
