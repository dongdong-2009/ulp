/*
	feng.ni 2017/04/08 initial version
 */

#include "config.h"
#include "ulp/sys.h"
#include "can.h"
#include "priv/usdt.h"
#include "shell/cmd.h"
#include <string.h>

#include "bsp.h"
#include "pdi.h"
#include "../ecu/ecu.h"

enum {
	MODEL_INVALID,
	//J71E
	MODEL_FRSU,
	MODEL_SRSU,

	//J71E DHS
	MODEL_DHSC,
	MODEL_DHSL,
	MODEL_DHSR,
};

typedef struct {
	int ecodes[2];
	int grp;
	int ecu_sensor_ch;
	int model;
	const char *name;
	const char *mark;
} sensor_t;

static const sensor_t rsu_sensor_list[] = {
	/*J71E*/
	{.ecodes = {0x045d, 0x045a}, .grp = 1, -1, MODEL_FRSU, .name = "J71EF", .mark = "57K1X"},
	{.ecodes = {0x0459, 0x045c}, .grp = 2, -1, MODEL_SRSU, .name = "J71ES", .mark = "57KC0"},

	/*J71E DHS, 1st is for rsu#1, 2nd is for rsu#2*/
	{.ecodes = {0x468, 0x0465}, .grp = 0, -1, MODEL_DHSC, .name = "J71EDC", .mark = "GBFT"},
	{.ecodes = {0x46a, 0x0467}, .grp = 0, -1, MODEL_DHSL, .name = "J71EDL", .mark = "GBVR"},
	{.ecodes = {0x469, 0x0466}, .grp = 0, -1, MODEL_DHSR, .name = "J71EDR", .mark = "GBVP"},
};

static int ecode_ok_list[] = { //approved ecode list
	0x0465, 0x0466, 0x0467, 0x0468, 0x0469, 0x046a, //DHS
	0x0459, 0x045a, 0x045c, 0x045d, 0x045f, 0x0462, //J71E
};

const static int can_id_tx = 0x737;
const static int can_id_rx = 0x73F;
#if CONFIG_RSU_J71E_DHS == 1
const char *pdi_fixture_id = "R519";
#endif
#if CONFIG_RSU_J71E == 1
const char *pdi_fixture_id = "R520";
#endif
const static int addr_fb = 0xfd39;
const static int addr_swv = 0xfd42;

static int rsu_model = MODEL_INVALID;//MODEL_240GT3;
static can_msg_t ecu_tx_msg;
static can_msg_t ecu_rx_msg;
#define rxdata ecu_rx_msg.data
#define txdata ecu_tx_msg.data

static int in_list(int ecode, const int *list, int nitems)
{
	int yes = 0;
	for(int i = 0; i < nitems; i ++) {
		if(list[i] == ecode) {
			yes = 1;
			break;
		}
	}

	return yes;
}

static int rsu_sensor_search_by_model(int model)
{
	int n = sizeof(rsu_sensor_list) / sizeof(sensor_t);
	for(int i = 0; i < n; i ++ ) {
		if(rsu_sensor_list[i].model == model)
			return i;
	}
	return -1;
}

static int rsu_sensor_search_by_ecode(int ecode)
{
	int n = sizeof(rsu_sensor_list) / sizeof(sensor_t);
	int necodes = sizeof(rsu_sensor_list[0].ecodes) / sizeof(int);
	for(int i = 0; i < n; i ++ ) {
		if(in_list(ecode, rsu_sensor_list[i].ecodes, necodes))
			return i;
	}
	return -1;
}

static int rsu_sensor_search_by_name(const char *name)
{
	int n = sizeof(rsu_sensor_list) / sizeof(sensor_t);
	for(int i = 0; i < n; i ++ ) {
		if(!strcmp(rsu_sensor_list[i].name, name))
			return i;
	}
	return -1;
}

/*called before ecu access*/
int ecu_start_session(void)
{
	ecu_can_init(can_id_tx, can_id_rx);
	return ecu_probe_algo(0x03, ecu_decrypt_CalculateKey);
}

int pdi_host_update_ex(const char *cmd, int cmd_len)
{
	int rsu_index, ecode = -1;
	char rsu_name[16];
	memset(rsu_name, 0x00, sizeof(rsu_name));

	cmd_len = (cmd_len > 15) ? 15 : cmd_len;
	strncpy(rsu_name, cmd, cmd_len);
	rsu_index = rsu_sensor_search_by_name(rsu_name);
	if(rsu_index >= 0) {
		rsu_model = rsu_sensor_list[rsu_index].model;
		bsp_select_grp(rsu_sensor_list[rsu_index].grp);
		ecode = 0;
	}

	return ecode;
}

/* P71A ALL RSU DISCONNECTED
PDI# did read 0xab39
CAN_TX: 02 10 01 00 00 00 00 00         ........
CAN_RX: 06 50 01 00 32 01 f4 ff         .P..2...
CAN_TX: 02 10 61 00 00 00 00 00         ..a.....
CAN_RX: 06 50 61 00 32 01 f4 ff         .Pa.2...
CAN_TX: 02 27 61 00 00 00 00 00         .'a.....
CAN_RX: 04 67 61 50 45 ff ff ff         .gaPE...
CAN_TX: 04 27 62 35 35 00 00 00         .'b55...
CAN_RX: 02 67 62 ff ff ff ff ff         .gb.....
CAN_TX: 03 22 ab 39 00 00 00 00         .".9....
CAN_RX: 11 02 62 ab 39 02 a2 0b         ..b.9...
CAN_TX: 30 00 00 00 00 00 00 00         0.......
CAN_RX: 21 02 a4 0b 04 5a 0b 04         !....Z..
CAN_RX: 22 5d 0b 04 5e 0b 04 60         "]..^..`
CAN_RX: 23 0b 04 63 0b 04 65 0b         #..c..e.
CAN_RX: 24 04 66 0b 04 67 0b 05         $.f..g..
CAN_RX: 26 00 00 00 00 00 00 00         &.......
CAN_RX: 28 00 00 00 00 00 00 00         (.......
CAN_RX: 2a 00 00 00 00 00 00 00         *.......
CAN_RX: 2c 00 00 00 00 00 00 00         ,.......
CAN_RX: 2e 00 00 00 00 00 00 00         ........
CAN_RX: 20 00 00 00 00 00 00 00          .......
CAN_RX: 22 00 00 00 00 00 00 00         ".......
CAN_RX: 24 00 00 00 00 00 00 00         $.......
CAN_RX: 26 00 00 00 00 00 00 00         &.......
CAN_RX: 28 00 00 00 00 00 00 00         (.......
CAN_RX: 2a 00 00 00 00 00 00 00         *.......
CAN_RX: 2c 00 00 00 00 00 00 00         ,.......
CAN_RX: 2e 00 00 00 00 00 00 00         ........
CAN_RX: 20 00 00 00 00 00 00 00          .......
CAN_RX: 22 00 00 00 00 00 00 00         ".......
CAN_RX: 24 00 00 00 00 00 00 00         $.......
DID: 02 a2 0b 02 a4 0b 04 5a            .......Z
DID: 0b 04 5d 0b 04 5e 0b 04            ..]..^..
DID: 60 0b 04 63 0b 04 65 0b            `..c..e.
DID: 04 66 0b 04 67 0b 05 00            .f..g...
DID: 00 00 00 00 00 00 00 00            ........
DID: 00 00 00 00 00 00 00 00            ........
DID: 00 00 00 00 00 00 00 00            ........
DID: 00 00 00 00 00 00 00 00            ........
DID: 00 00 00 00 00 00 00 00            ........
DID: 00 00 00 00 00 00 00 00            ........
DID: 00 00 00 00 00 00 00 00            ........
DID: 00 00 00 00 00 00 00 00            ........
DID: 00 00 00 00 00 00 00 00            ........
DID: 00 00 00 00 00 00 00 00            ........
DID: 00 00 00 00 00 00 00 00            ........
DID: 00 00 00 00 00 00 00 00            ........
DID: 00 00 00 00 00 00 00 00            ........
DID: 00 00 00 00 00 00 00               .......
PDI#
*/

//return nr of faults found, include history and current error
static int fault_search(void *faultbytes, int nbytes, int e)
{
	unsigned short ecode = (unsigned short) e;
	char *fb = (char *) faultbytes;
	unsigned short msb, lsb, val;

	char desc[32];
	memset(desc, 0x00, 32);
	if(e) { //search specified ecode
		int rsu_index = rsu_sensor_search_by_ecode(e);
		if(rsu_index >= 0) {
			if(rsu_sensor_list[rsu_index].ecu_sensor_ch >= 0) {
				sprintf(desc, ": crash sensor #%d error - %s", rsu_sensor_list[rsu_index].ecu_sensor_ch, rsu_sensor_list[rsu_index].name);
			}
			else {
				sprintf(desc, ": crash sensor error - %s", rsu_sensor_list[rsu_index].name);
			}
		}
	}

	int count = 0;
	for(int i = 0; i < nbytes; i += 3) {
		//error type
		msb = fb[i];
		lsb = fb[i + 1];
		val = (msb << 8) | lsb;

		if((e == 0) || (val == ecode)) {
			if(fb[i + 2] == 0x0b) { //current error
				count ++;
				printf("FB[%02X] = 0x%04x  %s\n", i, val, desc);
			}
			else { //history error
				if(e == 0) {
					if(val != 0) {
						printf("FB[%02X] = 0x%04x H%s\n", i, val, desc);
					}
				}
			}
		}
	}

	return count;
}

static int fault_is_approved(int ecode)
{
	int approved = 0;
	int n = sizeof(ecode_ok_list) / sizeof(ecode_ok_list[0]);
	for(int i = 0; i < n; i ++) {
		if(ecode_ok_list[i] == ecode) {
			approved = 1;
			break;
		}
	}
	return approved;
}

static int fault_search_abnormal(void *faultbytes, int nbytes)
{
	char *fb = (char *) faultbytes;
	unsigned short msb, lsb, val;

	int count = 0;
	for(int i = 0; i < nbytes; i += 3) {
		//error type
		msb = fb[i];
		lsb = fb[i + 1];
		val = (msb << 8) | lsb;

		if(fb[i + 2] == 0x0b) { //current error
			if(fault_is_approved(val) ) {
				printf("fb[%02X] = 0x%04x, masked\n", i, val);
				count += 0;
			}
			else {
				printf("fb[%02X] = 0x%04x\n", i, val);
				count += 1;
			}
		}
	}

	return count;
}

static void fault_dump(const char *prefix, void *faultbytes, int nbytes)
{
	int n = sizeof(rsu_sensor_list) / sizeof(sensor_t);
	int necodes = sizeof(rsu_sensor_list[0].ecodes) / sizeof(int);

	if(nbytes > 2) {
		hexdump(prefix, faultbytes, nbytes);
		for(int i = 0; i < n; i ++) {
			for(int j = 0; j < necodes; j ++) {
				int e =  rsu_sensor_list[i].ecodes[j];
				if(e != 0) {
					fault_search(faultbytes, nbytes, e);
				}
			}
		}
	}
}

static int ecu_clear_faults(void)
{
	ecu_start_session();
	ecu_tx_msg.id = can_id_tx;
	ecu_tx_msg.dlc = 8;
	//memcpy(txdata, "\x03\x2e\xfd\xff\x00\x00\x00\x00", 8);
	//memcpy(txdata, "\x04\x2e\xff\xff\x00\x00\x00\x00", 8);
	//memcpy(txdata, "\x04\x31\x01\xdc\x03\x00\x00\x00", 8);
	memcpy(txdata, "\x04\x14\xff\xff\xff\x00\x00\x00", 8);
	return ecu_transceive(&ecu_tx_msg, &ecu_rx_msg);
}

int cmd_ecu_func_ex(int argc, char *argv[])
{
	const char *usage = {
		"ecu fault		read ecu fault bytes(+ign)\n"
		"ecu clear		clear ecu fault bytes\n"
		"ecu swver		print ecu sw version info\n"
	};

	if (argc < 2) {
		printf("%s", usage);
		return 0;
	}

	if (!strcmp(argv[1], "fault")) {
		int addr = addr_fb;

		bsp_swbat(1);
		pdi_mdelay(100);
		char *text = (char *) sys_malloc(512);
		memset(text, 0x00, 512);
		int nread = ecu_did_read(addr, text, 512);
		//fault_dump("FB", text, nread);
		hexdump("FB", text, nread);
		fault_search(text, nread, 0X00);
		sys_free(text);
	}
	else if(!strcmp(argv[1], "clear")) {
		ecu_clear_faults();
	}
	else if(!strcmp(argv[1], "swver")) {
		char ver[16];
		ecu_did_read(addr_swv, ver, 16);
		printf("sw version: %s\n", ver);
	}
	return 0;
}

int pdi_test(int pos, int mask, pdi_report_t *report)
{
	char prefix[16];
	char *fb = (char *) sys_malloc(256);
	char passed[4] = {0, 0, 0, 0};
	int ecode = 0;

	pdi_mdelay_with_pull_detection(3000);
	for(int rsu = 0; rsu < 4; rsu ++) {
		if(!(mask & (1 << rsu))) {
			//no need to test
			continue;
		}

		//each ecu has 2 rsu channel
		int ecu = rsu >> 1;

		printf("pdi_test: clear ecu#%d fault bytes!\n", ecu);
		bsp_select_can(ecu);
		pdi_mdelay_with_pull_detection(100);
		ecu_clear_faults();
		pdi_mdelay_with_pull_detection(100);
		//ecu_reset();
	}

	//wait for ecu fb stable
	pdi_mdelay_with_pull_detection(5000);

	//read fault
	for(int rsu = 0; rsu < 4; rsu ++) {
		int ecu = rsu >> 1;
		int idx = rsu & 0x01; //ecodes index of specified rsu model

		if(!(mask & (1 << rsu))) {
			passed[rsu] = 1; //no need to test
			continue;
		}

		printf("pdi_test: read ecu#%d for rsu#%d ... \n", ecu, rsu);
		bsp_select_can(ecu);
		pdi_mdelay_with_pull_detection(10);

		memset(fb, 0x00, 256);
		int nread = ecu_did_read(addr_fb, fb, 256);
		if(nread == 0) {
			ecode = -1; //ecu ng
			continue;
		}

		//show debug message
		sprintf(prefix, "FB@ECU#%d", ecu);
		fault_dump(prefix, fb, nread);

		int rsu_index = rsu_sensor_search_by_model(rsu_model);
		if(rsu_index < 0) { //not found :(
			printf("<%d, Unknown RSU Model Type!!!\n", rsu_model);
			passed[rsu] = 0;
		}
		else {
			printf("pdi_test: search for ecode in fault bytes...\n");
			int nfit = fault_search(fb, nread, rsu_sensor_list[rsu_index].ecodes[idx]);
			if(nfit == 0) { //rsu works normally, check misconfig ecodes and etc
				nfit = fault_search_abnormal(fb, nread);
				if(nfit == 0) { //no extra ecodes
					passed[rsu] = 1;
				}
			}
		}
	}

	sys_free(fb);

	//emulate 0x5A0 demo ecu message
	static can_msg_t ecu_rx_msg;
	#define rxdata ecu_rx_msg.data
	memset(rxdata, 0x00, 8);

	rxdata[0] = (ecode) ? 0xc0 : 0x00;
	rxdata[0] |= (passed[0]) ? 0x00 : 0x20;
	rxdata[0] |= (passed[1]) ? 0x00 : 0x08;
	rxdata[0] |= (passed[2]) ? 0x00 : 0x02;

	rxdata[1] |= (passed[3]) ? 0x00 : 0x80;
	rxdata[1] |= 0x2a;

	rxdata[2] = 0x80;

	/*
	NOK	5A0 2a aa 80 00 00 00 00 00    0010 1010 1010 1010
	1OK	5A0 0a aa 80 00 00 00 00 00    0000 1010 1010 1010
	2OK	5A0 22 aa 80 00 00 00 00 00    0010 0010 1010 1010
	3OK	5A0 28 aa 80 00 00 00 00 00    0010 1000 1010 1010
	4OK	5A0 2a 2a 80 00 00 00 00 00    0010 1010 0010 1010
	*/

	int temp = 0;
	temp |= rxdata[0] << 8;
	temp |= rxdata[1] << 0;
	temp >>= 6;

	//ECU_NG 1 2 3 4
	debug("PDI: rsu diag result = 0x%04x\n", temp);
	if(rxdata[0] & 0xc0) { //ECU_NG
		return -2;
	}

	//fill the report
	report->result.byte = (unsigned char) (temp & 0xff);
	report->datalog[0] = rxdata[0];
	report->datalog[1] = rxdata[1];
	report->datalog[2] = rxdata[2];
	report->datalog[3] = rxdata[3];
	report->datalog[4] = rxdata[4];
	report->datalog[5] = rxdata[5];
	report->datalog[6] = rxdata[6];
	report->datalog[7] = rxdata[7];
	return 0;
}

static int cmd_rsu_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"rsu xx	config typeof rsu to test, xx = \n"
	};

	if (argc < 2) {
		printf("%s", usage);

		//list all supported rsu sensors
		int n = sizeof(rsu_sensor_list) / sizeof(sensor_t);
		for(int i = 0; i < n; i ++ ) {
			const sensor_t *s = &rsu_sensor_list[i];
			printf("%-10s: .grp = %d, .ecodes =", s->name, s->grp);

			int necodes = sizeof(s->ecodes) / sizeof(int);
			for(int j = 0; j < necodes; j ++) {
				printf(" 0x%04x", s->ecodes[j]);
			}
			printf(", .mark = %s\n", s->mark);
		}

		//list current config
		const char *rsu_name = "INVALID";
		int rsu_index = rsu_sensor_search_by_model(rsu_model);
		if(rsu_index >= 0) {
			rsu_name = rsu_sensor_list[rsu_index].name;
		}

		printf("current typeof rsu to test: %s\n", rsu_name);
		return 0;
	}

	pdi_host_update_ex(argv[1], strlen(argv[1]));
	return 0;
}

const cmd_t cmd_rsu = {"rsu", cmd_rsu_func, "rsu related cmds"};
DECLARE_SHELL_CMD(cmd_rsu)
