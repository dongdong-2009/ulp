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
#include "ecu.h"
#include "pdi.h"

//to be filled by ecu_can_init
int can_id_tx = 0;
int can_id_rx = 0;

//to be filled by ecu_probe_algo
int ecu_session = 0;
ecu_algo_t ecu_decrypt_algo = NULL;

can_msg_t req_flow_msg = {
	.id = 0, .dlc = 8, .data = {0x30, 0x00, 0, 0, 0, 0, 0, 0},
};
static can_msg_t ecu_tx_msg;
static can_msg_t ecu_rx_msg;
#define rxdata ecu_rx_msg.data
#define txdata ecu_tx_msg.data

int ecu_can_init(int txid, int rxid) {
	can_id_tx = txid;
	can_id_rx = rxid;

	const can_cfg_t cfg = {.baud = ECU_BAUD, .silent = 0};
	can_filter_t filter = {.id = can_id_rx, .mask = 0xffff, .flag = 0};

	bsp_can_bus.init(&cfg);
	bsp_can_bus.filt(&filter, 1);
	return 0;
}

__weak int ecu_start_session(void)
{
	if((can_id_tx == 0) || (ecu_decrypt_algo == NULL)) {
		return -1;
	}

	ecu_can_init(can_id_tx, can_id_rx);
	return ecu_probe_algo(ecu_session, ecu_decrypt_algo);
}

void ecu_report_error(int ecode) {
	const struct ecode_item_s {
		int ecode;
		const char *desc;
	} ecode_list[] = {
		{0x10, "general reject"},
		{0x11, "service not supported"},
		{0x12, "sub function not supported"},
		{0x14, "response too long"},
		{0x21, "busy repeat request"},
		{0x22, "condition not correct"},
		{0x24, "request sequence error"},
		{0x31, "request out of range"},
		{0x33, "security access denied"},
		{0x35, "invalid key"},
		{0x36, "exceed number of attempts"},
		{0x37, "required time delay not expired"},
		{0x72, "general programming failure"},
		{0x73, "wrong block sequence counter"},
		{0x78, "request correctly received - response pending"},
		{0x7e, "sub function not supported in active session"},
		{0x7f, "service not supported in active session"}
	};

	int nitems = sizeof(ecode_list) / sizeof(struct ecode_item_s);
	for(int i = 0; i < nitems; i ++) {
		if(ecode_list[i].ecode == ecode) {
			printf("ECU: 0x%02X, %s\n", ecode, ecode_list[i].desc);
			return;
		}
	}
	printf("ECU: 0x%02X, undefined error\n", ecode);
}

int ecu_transceive(const can_msg_t *txmsg, can_msg_t *rxmsg)
{
	pdi_can_tx(txmsg);
	int ecode = pdi_can_rx(rxmsg);
	if(rxmsg->data[0] < 0x10) {
		/*single frame situation, such as "did read 0xabce" return 2B + 3
		CAN_TX: 03 22 ab ce 00 00 00 00         ."......
		CAN_RX: 05 62 ab ce 75 0a ff ff         .b..u...
		*/
		ecode |= (rxmsg->data[1] != txmsg->data[1] + 0x40);
		//ecode |= (rxmsg->data[2] != txmsg->data[2]);
		if(ecode && (rxmsg->data[1] == 0x7F)) {
			//CAN_RX: 03 7f 14 78 ff ff ff ff
			int ecu_ecode = (unsigned char) rxmsg->data[3];
			ecu_report_error(ecu_ecode);
		}
	}
	else {
		/*multiframe situation, such as "did read 0xab39" return 255B + 3
		CAN_TX: 03 22 ab 39 00 00 00 00         .".9....
		CAN_RX: 11 02 62 ab 39 01 07 0b         ..b.9...
		*/
		ecode |= (rxmsg->data[2] != txmsg->data[1] + 0x40);
		//ecode |= (rxmsg->data[3] != txmsg->data[2]);
	}
	return ecode;
}

void ecu_decrypt_CalculateKey(unsigned char access_level, unsigned char seed[8], unsigned char key[8])
{
	unsigned char idx, i;
	unsigned char mask, x;
	unsigned char kSecB2[2] = {0x00, 0x80};
	unsigned char kSecC2[2] = {0x00, 0x10};
	unsigned char kSecC1[2] = {0x00, 0x90};
	unsigned char kSecC0[2] = {0x00, 0x28};
	unsigned char secKey[3] = {0xA9, 0x41, 0xC5};
	// Select the five fixed bytes
	switch(access_level) {
		case 0x01:
			seed[3] = 0x52;
			seed[4] = 0x6F;
			seed[5] = 0x77;
			seed[6] = 0x61;
			seed[7] = 0x6E;
			break;
		case 0x03:
			seed[3] = 0x5A;
			seed[4] = 0x89;
			seed[5] = 0xE4;
			seed[6] = 0x41;
			seed[7] = 0x72;
			break;
		case 0x61:
			seed[3] = 0x4D;
			seed[4] = 0x5A;
			seed[5] = 0x04;
			seed[6] = 0x68;
			seed[7] = 0x38;
			break;
	}
	// Key calculation
	mask = 0x01;
	x=0;
	for(i=0; i<64;i++) {
		if((mask & seed[x])!=0) { // lsb != 0
			idx = 0x01;
		}
		else {
			idx = 0x00;
		}
		idx ^= (secKey[0] & 0x01);
		if(mask == 0x80) {
			mask = 0x01;
			x++;
		}
		else {
			mask <<= 1;
		}
		secKey[0]>>=1;
		if((secKey[1] & 0x01) != 0) {
			secKey[0] |= 0x80;
		}
		secKey[1]>>=1;
		if((secKey[2] & 0x01) != 0) {
			secKey[1] |= 0x80;
		}
		secKey[2]= ((secKey[2]>>=1) | kSecB2[idx]);
		secKey[0] = (secKey[0] ^ kSecC0[idx]);
		secKey[1] = (secKey[1] ^ kSecC1[idx]);
		secKey[2] = (secKey[2] ^ kSecC2[idx]);
	}
	key[0] = (secKey[0] >> 4);
	key[0] |= (secKey[1] << 4);
	key[1] = (secKey[2] >> 4);
	key[1] |= (secKey[1] & 0xF0);
	key[2] = (secKey[2] & 0x0F);
	key[2] |= (secKey[0] << 4);
}

void ecu_decrypt_239A(unsigned char access_level, unsigned char seed[8], unsigned char key[8])
{
	int result = seed[0] ^ seed[1];
	result ^= 0x23;
	result ^= 0x9a;
	result += 0x239a;
	key[0] = (result >> 8) & 0xff;
	key[1] = (result >> 0) & 0xff;
}

void ecu_decrypt_34AB(unsigned char access_level, unsigned char seed[8], unsigned char key[8])
{
	int result = seed[0] ^ seed[1];
	result ^= 0x34;
	result ^= 0xab;
	result += 0x34ab;
	key[0] = (result >> 8) & 0xff;
	key[1] = (result >> 0) & 0xff;
}

void ecu_decrypt_B1234C12(unsigned char access_level, unsigned char seed[8], unsigned char key[8])
{
	int result = 0;
	result |= (seed[0] << 24);
	result |= (seed[1] << 16);
	result |= (seed[2] << 8);
	result |= (seed[3] << 0);
	result += 0xB1234C12;
	key[0] = (result >> 24) & 0xff;
	key[1] = (result >> 16) & 0xff;
	key[2] = (result >> 8) & 0xff;
	key[3] = (result >> 0) & 0xff;
}

//return bytes filled into the pbuf
__weak int ecu_read(int addr, void *pbuf, int szbuf, char sid, char alen)
{
	ecu_start_session();
	ecu_tx_msg.id = can_id_tx;
	ecu_tx_msg.dlc = 8;
	memcpy(txdata, "\x03\x22\xff\xff\x00\x00\x00\x00", 8);

	txdata[0] = 1 + alen;
	txdata[1] = sid;

	switch(alen) {
	case 2:
		txdata[2] = (char)((addr >> 8) & 0xff);
		txdata[3] = (char)((addr >> 0) & 0xff);
		break;
	case 3:
		txdata[2] = (char)((addr >> 16) & 0xff);
		txdata[3] = (char)((addr >> 8) & 0xff);
		txdata[4] = (char)((addr >> 0) & 0xff);
		break;
	case 4:
		txdata[2] = (char)((addr >> 24) & 0xff);
		txdata[3] = (char)((addr >> 16) & 0xff);
		txdata[4] = (char)((addr >> 8) & 0xff);
		txdata[5] = (char)((addr >> 0) & 0xff);
		break;
	default:
		sys_assert(1 == 0); //invalid para alen
	}

	int ecode = ecu_transceive(&ecu_tx_msg, &ecu_rx_msg);
	if(ecode) {
		return 0;
	}

	char *data = (char *) pbuf;
	int nleft, nread = 0;
	int nbyte; //n bytes to read

	if(rxdata[0] < 0x10) {
		/*single frame situation, such as "did read 0xabce" return 2B + 3
		CAN_TX: 03 22 ab ce 00 00 00 00         ."......
		CAN_RX: 05 62 ab ce 75 0a ff ff         .b..u...
		*/
		nleft = rxdata[0] - 3;
		nbyte = (szbuf < nleft) ? szbuf : nleft;
		memcpy(data, &rxdata[4], nbyte);
		nread += nbyte;
		nleft -= nbyte;
		return nread;
	}

	/*multiframe situation, such as "did read 0xab39" return 255B + 3
	CAN_TX: 03 22 ab 39 00 00 00 00         .".9....
	CAN_RX: 11 02 62 ab 39 01 07 0b         ..b.9...
	*/
	nleft = (rxdata[0] & 0x0f);
	nleft = (nleft << 8) + rxdata[1] - 3; //3 = 62 ab 39
	memcpy(data, &rxdata[5], 3); // 3 = 01 07 0b
	nread += 3;
	nleft -= 3;
	req_flow_msg.id = can_id_tx;
	pdi_can_tx(&req_flow_msg);
	while((nleft > 0) && (nread < szbuf)) {
		ecode = pdi_can_rx(&ecu_rx_msg);
		if(ecode) break;

		nbyte = nleft;
		nbyte = (nbyte > szbuf - nread) ? szbuf - nread : nbyte;
		nbyte = (nbyte > 7) ? 7 : nbyte;
		memcpy(data + nread, &rxdata[1], nbyte);
		nread += nbyte;
		nleft -= nbyte;
	}

	return nread;
}

//return bytes filled into the pbuf
__weak int ecu_did_read(int addr, void *pbuf, int szbuf)
{
	return ecu_read(addr, pbuf, szbuf, 0x22, 2);
}

//return bytes filled into the pbuf
__weak int ecu_dtc_read(int addr, void *pbuf, int szbuf)
{
	return ecu_read(addr, pbuf, szbuf, 0x19, 2);
}

__weak int ecu_did_write(int addr, const void *pbuf, int szbuf)
{
}

__weak int ecu_reset(void)
{
	ecu_start_session();
	ecu_tx_msg.id = can_id_tx;
	ecu_tx_msg.dlc = 8;
	memcpy(txdata, "\x02\x11\x01\x00\x00\x00\x00\x00", 8);
	return ecu_transceive(&ecu_tx_msg, &ecu_rx_msg);
}

__weak int ecu_dtc_clear(void)
{
	ecu_start_session();
	ecu_tx_msg.id = can_id_tx;
	ecu_tx_msg.dlc = 8;
	memcpy(txdata, "\x04\x14\xff\xff\xff\x00\x00\x00", 8);
	return ecu_transceive(&ecu_tx_msg, &ecu_rx_msg);
}

int ecu_probe_id(int txid, int rxid)
{
	ecu_can_init(txid, rxid);
	can_msg_t start_session_msg = {
		.id = can_id_tx, .dlc = 8, .data = {0x02, 0x10, 0x01, 0, 0, 0, 0, 0}
	};
	return ecu_transceive(&start_session_msg, &ecu_rx_msg);
}

typedef struct {
	int txid;
	int rxid;
	const char *desc;
} id_cfg_t;

static int cmd_probe_id(int txid, int rxid)
{
	const id_cfg_t id_cfg_lists[] = {
		{0x247, 0x547, "SDM10"}, //0X647
		{0x607, 0x608, "BA, DM_DMA, HC, TL"},
		{0x737, 0x73F, "B515, CCP2"},
		//{0x746, "M16"},
		{0x752, 0x772, "B12L, HZH, RP"},
		//{0x76A, "RCLE"},
		{0x780, 0x781, "CHB041, CHB121"},
		{0x794, 0x79C, "J04N"},
		{0x7A2, 0x7C2, "C131"},
		{0x7D2, 0x7DA, "BA, DM_DMA, HC"},
		//{0x7E5, "RCLE"},
	};

	const id_cfg_t *pcfg = id_cfg_lists;
	int ncfg = sizeof(id_cfg_lists) / sizeof(id_cfg_t);
	id_cfg_t cfg_temp = {.txid = txid, .rxid = rxid, .desc = "no"};

	if(txid != 0) {
		if(rxid == 0) {
			for(int i = 0; i < ncfg; i ++) {
				if(pcfg[i].txid == txid) {
					cfg_temp.rxid = rxid = pcfg[i].rxid;
					cfg_temp.desc = pcfg[i].desc;
					break;
				}
			}
		}

		if(rxid == 0) {
			printf("probe: aborted! rxid is unknown\n");
			return -1;
		}

		pcfg = &cfg_temp;
		ncfg = 1;
	}

	for(int i = 0; i < ncfg; i ++) {
		if(!ecu_probe_id(pcfg[i].txid, pcfg[i].rxid)) {
			printf("probe: txid = 0x%03X, rxid = 0x%03X\n", pcfg[i].txid, pcfg[i].rxid);
			printf("probe: %s pdi has used it\n", pcfg[i].desc);
			return 0;
		}
	}

	printf("probe: failed!");
	return -2;
}

int ecu_probe_session(int session)
{
	can_msg_t start_session_msg = {
		.id = can_id_tx, .dlc = 8, .data = {0x02, 0x10, 0x01, 0, 0, 0, 0, 0}
	};

	ecu_transceive(&start_session_msg, &ecu_rx_msg); //reset first
	start_session_msg.data[2] = session & 0xff;
	pdi_mdelay(10);
	return ecu_transceive(&start_session_msg, &ecu_rx_msg);
}

int ecu_probe_algo(int session, ecu_algo_t decrypt_algo_func)
{
	can_msg_t reqseed_msg = {.id = can_id_tx, .dlc = 8};
	can_msg_t sendkey_msg = {.id = can_id_tx, .dlc = 8};

	if(decrypt_algo_func == ecu_decrypt_CalculateKey) {
		memcpy(reqseed_msg.data, "\x02\x27\x61\x00\x00\x00\x00\x00", 8);
		memcpy(sendkey_msg.data, "\x05\x27\x62\xff\xff\xff\x00\x00", 8);
	}
	else if(decrypt_algo_func == ecu_decrypt_239A) {
		memcpy(reqseed_msg.data, "\x02\x27\x7d\x00\x00\x00\x00\x00", 8);
		memcpy(sendkey_msg.data, "\x04\x27\x7e\xff\xff\x00\x00\x00", 8);
	}
	else if(decrypt_algo_func == ecu_decrypt_34AB) {
		memcpy(reqseed_msg.data, "\x02\x27\x61\x00\x00\x00\x00\x00", 8);
		memcpy(sendkey_msg.data, "\x04\x27\x62\xff\xff\x00\x00\x00", 8);
	}
	else if(decrypt_algo_func == ecu_decrypt_B1234C12) {
		memcpy(reqseed_msg.data, "\x02\x27\xc1\x00\x00\x00\x00\x00", 8);
		memcpy(sendkey_msg.data, "\x06\x27\xc2\xff\xff\xff\xff\x00", 8);
	}
	else {
		return -1; //algo not identified
	}

	if(ecu_probe_session(session)) {
		return -2; //start session failed
	}

	int ecode = ecu_transceive(&reqseed_msg, &ecu_rx_msg);
	if(ecode) {
		return -3;
	}

	unsigned char access_level, seed[8], key[8];
	memset(seed, 0x00, 8);
	memset(key, 0x00, 8);

	access_level = rxdata[2];
	seed[0] = rxdata[3];
	seed[1] = rxdata[4];
	seed[2] = rxdata[5];
	seed[3] = rxdata[6];
	decrypt_algo_func(access_level, seed, key);
	sendkey_msg.data[3] = key[0];
	sendkey_msg.data[4] = key[1];
	sendkey_msg.data[5] = key[2];
	sendkey_msg.data[6] = key[3];
	ecode = ecu_transceive(&sendkey_msg, &ecu_rx_msg);
	if(ecode) {
		return -4;
	}

	ecu_session = session;
	ecu_decrypt_algo = decrypt_algo_func;
	return 0;
}

typedef struct {
	ecu_algo_t algo;
	const char *desc;
} algo_item_t;

static int cmd_probe_algo(int session)
{
	const algo_item_t algo_lists[] = {
		{ecu_decrypt_CalculateKey, "CalculateKey"},
		{ecu_decrypt_239A, "239A"},
		{ecu_decrypt_34AB, "34AB"},
		{ecu_decrypt_B1234C12, "B1234C12"},
	};

	const algo_item_t *pitem = algo_lists;
	int nitem = sizeof(algo_lists) / sizeof(algo_item_t);
	for(int i = 0; i < nitem; i ++) {
		if(!ecu_probe_algo(session, pitem[i].algo)) {
			printf("probe: successfully decrypted!\n");
			printf("probe: algo = %s\n", pitem[i].desc);
			printf("probe: session = 0x%02x\n", session);
			return 0;
		}
	}

	printf("probe: decrypt failed!\n");
	printf("probe: session = 0x%02x\n", session);
	return -1;
}

static int cmd_probe_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"probe id [txid rxid]		auto probe ecu can id\n"
		"probe id\n"
		"probe id 0x752\n"
		"probe id 0x752 0x772\n"
		"probe session			list all supported DiagSession\n"
		"probe algo [0x03]		probe decrypt algo with specified session\n"
		"probe algo				default session = 0x03\n"
		"probe algo 0x61\n"
	};

	if (argc < 2) {
		printf("%s", usage);
		return 0;
	}

	if (!strcmp(argv[1], "id")) {
		int txid = 0;
		int rxid = 0;

		if(argc >= 3) {
			if(!strncmp(argv[2], "0x", 2)) sscanf(argv[2], "0x%x", &txid);
			else sscanf(argv[2], "%d", &txid);
		}

		if(argc >= 4) {
			if(!strncmp(argv[3], "0x", 2)) sscanf(argv[3], "0x%x", &rxid);
			else sscanf(argv[3], "%d", &rxid);
		}

		cmd_probe_id(txid, rxid);
	}

	if (!strncmp(argv[1], "session", 3)) {
		if(can_id_tx == 0) {
			if(cmd_probe_id(0, 0)) {
				return 0;
			}
		}

		int j = 0;
		char session_list[16];

		for(int i = 0; i < 256; i ++) {
			if(!ecu_probe_session(i)) {
				printf("probe: diag session 0x%02x is detected!\n", i);
				session_list[j] = (char) i;
				j ++;
			}
		}

		if(j > 0) {
			hexdump("probe: session = ", session_list, j);
		}
		else {
			printf("probe: session = none\n");
		}
	}

	if (!strcmp(argv[1], "algo")) {
		int diagsession = 0x03;

		if(argc >= 3) {
			if(!strncmp(argv[2], "0x", 2)) sscanf(argv[2], "0x%x", &diagsession);
			else sscanf(argv[2], "%d", &diagsession);
		}

		if(can_id_tx == 0) {
			if(cmd_probe_id(0, 0)) {
				return 0;
			}
		}

		cmd_probe_algo(diagsession);
	}

	return 0;
}

const cmd_t cmd_probe = {"probe", cmd_probe_func, "ecu probe cmds"};
DECLARE_SHELL_CMD(cmd_probe)

static int cmd_did_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"did read [addr]		read did of addr(dec or hex), such as:\n"
		"did read 0xff92	 	default addr = 0xff92, sn\n"
		"did read 65426\n"
	};

	if (argc < 2) {
		printf("%s", usage);
		return 0;
	}

	if (!strcmp(argv[1], "read")) {
		int addr = 0xff92; //serial number

		if(argc >= 3) {
			if(!strncmp(argv[2], "0x", 2)) sscanf(argv[2], "0x%x", &addr);
			else sscanf(argv[2], "%d", &addr);
		}

		char *text = (char *) sys_malloc(512);
		int nread = ecu_did_read(addr, text, 512);
		hexdump("DID: ", text, nread);
		sys_free(text);
	}

	return 0;
}

const cmd_t cmd_did = {"did", cmd_did_func, "ecu did mem access"};
DECLARE_SHELL_CMD(cmd_did)

static int cmd_dtc_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"dtc read [addr]		read did of addr(dec or hex), such as:\n"
		"dtc read 0x028b	 	default addr = 0x028b\n"
		"dtc read 651\n"
		"dtc clear			clear all dtc info\n"
	};

	if (argc < 2) {
		printf("%s", usage);
		return 0;
	}

	if (!strcmp(argv[1], "read")) {
		int addr = 0x028b;

		if(argc >= 3) {
			if(!strncmp(argv[2], "0x", 2)) sscanf(argv[2], "0x%x", &addr);
			else sscanf(argv[2], "%d", &addr);
		}

		char *text = (char *) sys_malloc(512);
		int nread = ecu_dtc_read(addr, text, 512);
		hexdump("DTC: ", text, nread);
		sys_free(text);
	}

	if (!strcmp(argv[1], "clear")) {
		ecu_start_session();
		ecu_dtc_clear();
	}

	return 0;
}

const cmd_t cmd_dtc = {"dtc", cmd_dtc_func, "ecu dtc mem access"};
DECLARE_SHELL_CMD(cmd_dtc)

static int cmd_ecu_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"ecu reset		reset ecu\n"
	};

	if (argc < 2) {
		printf("%s", usage);
		return 0;
	}

	if (!strcmp(argv[1], "reset")) {
		ecu_reset();
	}

	return 0;
}

const cmd_t cmd_ecu = {"ecu", cmd_ecu_func, "general ecu cmds"};
DECLARE_SHELL_CMD(cmd_ecu)

