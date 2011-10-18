/*
 * david@2011 initial version
 */
#include <string.h>
#include <stdio.h>
#include "stm32f10x.h"
#include "sys/task.h"
#include "osd/osd.h"
#include "linux/list.h"
#include "sys/sys.h"
#include "config.h"
#include "nvm.h"
#include "key.h"
#include "ulp_time.h"
#include "c131_driver.h"
#include "c131_diag.h"
#include "c131.h"

typedef enum {
	DIAGNOSIS_NOTYET,
	DIAGNOSIS_FAILED,
	DIAGNOSIS_SUCCESSFUL,
}c131_diagnosis_t;

typedef enum {
	SDM_UNEXIST,
	SDM_EXIST,
}c131_exist_t;

typedef enum {
	C131_STAGE1_RELAY,
	C131_STAGE1_CANMSG,
	C131_STAGE2_RELAY,
	C131_STAGE2_CANMSG,
	C131_GET_DTCMSG,
	C131_STAGE_FAILED,
	C131_STAGE_OVER,
}c131_stage_t;

typedef enum {
	C131_TEST_NOTYET,
	C131_TEST_ONGOING,
	C131_TEST_FAIL,
	C131_TEST_SUCCESSFUL,
}c131_test_t;

struct can_queue_s {
	int ms;
	time_t timer;
	can_msg_t msg;
	struct list_head list;
};

#define C131_STAGE1_MS		5000
#define C131_STAGE2_MS		5000
#define C131_GETDTC_MS		30000

//for osd
static const int keymap[] = {
	KEY_UP,
	KEY_DOWN,
	KEY_ENTER,
	KEY_RIGHT,
	KEY_LEFT,
	KEY_RESET,
	KEY_NONE
};

static const char selected = 0x7f;
static const char unselected = 0x20;

static int c131_index_load;
static char sdmtype_ram[16];	//the last char for EOC

static const char str_internal[] = "Internal";
static const char str_external[] = "External";
static char indicator_pwr;
static char sdm_pwr;
static char led_pwr;

static const char str_sdmon[]  = "SDM Con   ";
static const char str_sdmoff[] = "SDM Discon";
static char status_link = 0;

static const char str_startenter[]     = "Start -> Enter ";
static const char str_diagfailed[]     = "Diag Failed    ";
static const char str_diagsuccessful[] = "Diag Successful";
static char status_diag;

static const char str_testongoing[]    = "Test  Ongoing  ";
static const char str_testfailed[]     = "Test   Failed  ";
static const char str_testsuccessful[] = "Test Successful";
static int c131_test_status;
static int c131_stage_status;
static time_t stage_timer;
static time_t dtc_timer;

static const char str_nodtcinfo[] = "No   DTC   Info ";
static char dtc_info[16];
static char dtc_index;
static dtc_t dtc_buffer[32];
static c131_dtc_t c131_dtc;

//for load config variant nvm
static apt_load_t c131_load[NUM_OF_LOAD] __nvm;
static int c131_current_load __nvm;
static int num_load;

//for const can frame which will not change
static struct can_queue_s c131_ems_3 = {
	.ms = 10,
	.msg = {0x11c, 4, {0x00, 0x00, 0x00, 0x00}, 0},
};

static struct can_queue_s c131_ems_4 = {
	.ms = 10,
	.msg = {0x122, 8, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
};

static struct can_queue_s c131_abs_2 = {
	.ms = 10,
	.msg = {0xb6, 8, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
};

static struct can_queue_s c131_abs_3 = {
	.ms = 20,
	.msg = {0x12c, 4, {0x00, 0x00, 0x00, 0x00}, 0},
};

static struct can_queue_s c131_tcu_3 = {
	.ms = 100,
	.msg = {0x248, 8, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
};

//for static can frame which need change in livecounter and checksum field
static struct can_queue_s c131_abs_1 = {
	.ms = 10,
	.msg = {0xb4, 8, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
};

static struct can_queue_s c131_sas_1 = {
	.ms = 10,
	.msg = {0x96, 8, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
};

//for diagnosis can msg
static const can_msg_t req_dtc_msg = {
	.id = C131_DIAG_REQ_ID,
	.dlc = 8,
	.data = {0x03, 0x19, 0x02, 0x09, 0x00, 0x00, 0x00, 0x00},
};

static const can_msg_t req_clrdtc_msg = {
	.id = C131_DIAG_REQ_ID,
	.dlc = 8,
	.data = {0x04, 0x14, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00},
};

static const can_msg_t req_start_msg = {
	.id = C131_DIAG_REQ_ID,
	.dlc = 8,
	.data = {0x02, 0x10, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00},
};

static const can_msg_t req_flow_msg = {
	.id = C131_DIAG_REQ_ID,
	.dlc = 8,
	.data = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
};

static LIST_HEAD(can_queue);
static const can_bus_t *can_bus;

//for local functions
static int c131_GetSDMType(int index_load, char *pname);
static int c131_GetLoad(apt_load_t ** pload, int index_load);
static int c131_ConfirmLoad(int index_load);
static int c131_GetCurrentLoadIndex(void);
static void c131_CanMSGInit(void);
static void c131_can_SendOtherECUMsg(void);
static int c131_can_StartDiagnosis(void);
static int c131_can_GetAirbagMsg(can_msg_t *pmsg);

static void c131_Init(void)
{
	int i;
	char name_temp[16];
	int hdlg;

	//init config which has been confirmed
	c131_current_load = 0;
	memset(name_temp, 0xff, 16);
	for (i = 0; i < NUM_OF_LOAD; i++) {
		if (memcmp(c131_load[i].load_name, name_temp, 16) == 0) {
			c131_load[i].load_bExist = 0;
		}
		if (c131_load[i].load_bExist)
			num_load ++;
		//nvm_save();
	}

#ifdef CONFIG_TASK_OSD
	hdlg = osd_ConstructDialog(&apt_dlg);
	osd_SetActiveDialog(hdlg);
	key_SetLocalKeymap(keymap);
#endif

	//for dlg support members init
	c131_index_load = 0;
	if (num_load == 0)
		strcpy(sdmtype_ram, "No Invalid Cfg ");
	else {
		while(c131_GetSDMType(c131_index_load, sdmtype_ram))
			c131_index_load ++;
	}

	indicator_pwr = 0;
	sdm_pwr = 0;
	led_pwr = 0;
	status_link = 0;
	status_diag = DIAGNOSIS_NOTYET;
	c131_test_status = C131_TEST_NOTYET;
	c131_stage_status = C131_STAGE1_RELAY;
	dtc_index = 0;

	//lowlevel app init
	c131_driver_Init();
	c131_diag_Init();
	c131_CanMSGInit();

	//for led pwr and states init
	Enable_LEDPWR();
	led_SetRelayStatus(C131_LED1_C1, RELAY_ON);
	led_SetRelayStatus(C131_LED1_C2, RELAY_ON);
	led_SetRelayStatus(C131_LED2, RELAY_ON);
	led_SetRelayStatus(C131_LED3, RELAY_ON);
	led_SetRelayStatus(C131_LED4_C2, RELAY_ON);
	led_SetRelayStatus(C131_LED5, RELAY_ON);
	c131_relay_Update();

	//for dtc related varible init
	c131_dtc.pdtc = dtc_buffer;
}

static void c131_Update(void)
{
	can_msg_t msg;

	//for device under testing check
	if (Get_SDMStatus()) {
		status_link = SDM_UNEXIST;
	} else {
		status_link = SDM_EXIST;
	}

	if (Get_SDMPWRStatus())
		c131_can_SendOtherECUMsg();

	//for sdm testing status machine
	if (c131_test_status == C131_TEST_ONGOING) {
		//embedded test flow state machine
		switch (c131_stage_status) {
			case C131_STAGE1_RELAY:
				sw_SetRelayStatus(C131_SW2_C1 | C131_SW5_C1, RELAY_ON);
				sw_SetRelayStatus(C131_SW2_C2 | C131_SW5_C2, RELAY_OFF);
				sw_SetRelayStatus(C131_SW3 | C131_SW4, RELAY_ON);
				c131_relay_Update();
				stage_timer = time_get(C131_STAGE1_MS);
				dtc_timer = time_get(C131_GETDTC_MS);
				c131_stage_status = C131_STAGE1_CANMSG;
				break;
			case C131_STAGE1_CANMSG:
				if (time_left(stage_timer) <= 0) {
					if (c131_can_GetAirbagMsg(&msg) == 0) {
						can_msg_print(&msg, "\n");
						//for DSB and PSB and PAD and SBR
						if (((msg.data[2]&0x0f) == 0x05) && (msg.data[5]&0x01) && ((msg.data[7]&0x04) == 0x0))
							c131_stage_status = C131_STAGE2_RELAY;
						else
							c131_stage_status = C131_STAGE_FAILED;
					}
				}
				break;
			case C131_STAGE2_RELAY:
				sw_SetRelayStatus(C131_SW2_C1 | C131_SW5_C1, RELAY_ON);
				sw_SetRelayStatus(C131_SW2_C2 | C131_SW5_C2, RELAY_ON);
				sw_SetRelayStatus(C131_SW3 | C131_SW4, RELAY_OFF);
				c131_relay_Update();
				stage_timer = time_get(C131_STAGE2_MS);
				c131_stage_status = C131_STAGE2_CANMSG;
				break;
			case C131_STAGE2_CANMSG:
				if (time_left(stage_timer) <= 0) {
					if (c131_can_GetAirbagMsg(&msg) == 0) {
						can_msg_print(&msg, "\n");
						//for DSB and PSB and PAD and SBR
						if (((msg.data[2]&0x0f) == 0x0) && ((msg.data[5]&0x01) == 0x0) && (msg.data[7]&0x04))
							c131_stage_status = C131_GET_DTCMSG;
						else
							c131_stage_status = C131_STAGE_FAILED;
					}
				}
				break;
			case C131_GET_DTCMSG:
				//determine the dtc data
				if (time_left(dtc_timer) <= 0) {
					//warning lamp detect
					if (c131_can_GetAirbagMsg(&msg) == 0) {
						if (msg.data[5]&0x02)
							c131_stage_status = C131_STAGE_FAILED;
					}
					//DTC detect
					c131_can_GetDTC(&c131_dtc);
					if (c131_dtc.dtc_bExist)
						c131_stage_status = C131_STAGE_FAILED;

					if (c131_stage_status != C131_STAGE_FAILED)
						c131_stage_status = C131_STAGE_OVER;
				}
				break;
			case C131_STAGE_FAILED:
				FailLed_On();
				c131_test_status = C131_TEST_FAIL;
				Disable_SDMPWR();
				c131_stage_status = C131_STAGE1_RELAY;
				break;
			case C131_STAGE_OVER:
				FailLed_Off();
				c131_test_status = C131_TEST_SUCCESSFUL;
				Disable_SDMPWR();
				c131_stage_status = C131_STAGE1_RELAY;
				break;
			default :
				break;
		}
	}
}

int apt_AddLoad(apt_load_t * pload)
{
	int i;

	//for edit existing config
	for (i = 0; i < NUM_OF_LOAD; i++) {
		if (strcmp(c131_load[i].load_name, pload->load_name) == 0) {
			c131_load[i] = (*pload);
			c131_load[i].load_bExist =1;
			nvm_save();
			return 0;
		}
	}

	//for adding new config
	for (i = 0; i < NUM_OF_LOAD; i++) {
		if (c131_load[i].load_bExist == 0) {
			c131_load[i] = (*pload);
			c131_load[i].load_bExist =1;
			nvm_save();
			num_load ++;
			return 0;
		}
	}

	return 1;
}

int apt_SetAPTRAM(unsigned char *p)
{
	c131_SetRelayRAM(p);
	return c131_relay_Update();
}

//for item group APT SDM TYPE SELECT
int apt_GetSDMTypeName(void)
{
	return (int)sdmtype_ram;
}

int apt_GetSDMTypeSelect(void)
{
	if (c131_GetCurrentLoadIndex() == c131_index_load)
		return (int)&selected;
	else
		return (int)&unselected;
}

int apt_SelectSDMType(int keytype)
{
	if (num_load == 0) {
		strcpy(sdmtype_ram, "No Invalid Cfg ");
		return 0;
	}

	if (keytype == KEY_LEFT) {
		do {
			c131_index_load --;
			if (c131_index_load < 0)
				c131_index_load = NUM_OF_LOAD - 1;
		} while(c131_GetSDMType(c131_index_load, sdmtype_ram));
	} else if(keytype == KEY_RIGHT) {
		do {
			c131_index_load ++;
			if (c131_index_load >= NUM_OF_LOAD) 
				c131_index_load = 0;
		} while(c131_GetSDMType(c131_index_load, sdmtype_ram));
	} else if(keytype == KEY_ENTER) {
		c131_ConfirmLoad(c131_index_load);
	}

	return 0;
}

//for item group APT POWER SELECT
int apt_GetSDMPWRName(void)
{
	if (sdm_pwr)
		return (int)str_external;
	else
		return (int)str_internal;
}

int apt_GetSDMPWRIndicator(void)
{
	if (indicator_pwr == 0)
		return (int)&selected;
	else
		return (int)&unselected;
}

int apt_GetLEDPWRName(void)
{
	if (led_pwr)
		return (int)str_external;
	else
		return (int)str_internal;
}

int apt_GetLEDPWRIndicator(void)
{
	if (indicator_pwr == 1)
		return (int)&selected;
	else
		return (int)&unselected;
}

int apt_SelectPWR(int keytype)
{
	if (keytype == KEY_LEFT) {
		indicator_pwr = 0;
	} else if(keytype == KEY_RIGHT) {
		indicator_pwr = 1;
	} else if(keytype == KEY_ENTER) {
		if (indicator_pwr) {
			led_pwr = ~led_pwr;
			if (led_pwr)
				Enable_LEDEXTPWR();
			else
				Disable_LEDEXTPWR();
		} else {
			sdm_pwr = ~sdm_pwr;
			if (sdm_pwr)
				Enable_SDMEXTPWR();
			else
				Disable_SDMEXTPWR();
		}
	}

	return 0;
}

int apt_GetLinkInfo(void)
{
	if (status_link == SDM_UNEXIST)
		return (int)str_sdmoff;
	else
		return (int)str_sdmon;
}

int apt_GetDiagInfo(void)
{
	switch (status_diag) {
	case DIAGNOSIS_NOTYET:
		return (int)str_startenter;
	case DIAGNOSIS_FAILED:
		return (int)str_diagfailed;
	case DIAGNOSIS_SUCCESSFUL:
		return (int)str_diagsuccessful;
	default :
		break;
	}

	return 0;
}

int apt_SelectAPTDiag(int keytype)
{
	int ret = 0;
	if (keytype == KEY_ENTER) {
		ret = c131_DiagSW();
		ret += c131_DiagLOOP();
		if (ret)
			status_diag = DIAGNOSIS_FAILED;
		else
			status_diag = DIAGNOSIS_SUCCESSFUL;
	}

	return 0;
}

int apt_GetTestInfo(void)
{
	switch (c131_test_status) {
		case C131_TEST_NOTYET:
			return (int)str_startenter;
		case C131_TEST_ONGOING:
			return (int)str_testongoing;
		case C131_TEST_FAIL:
			return (int)str_testfailed;
		case C131_TEST_SUCCESSFUL:
			return (int)str_testsuccessful;
		default:
			break;
	}

	return (int)"ERROR";
}

int apt_SelectSDMTest(int keytype)
{
	if ((keytype == KEY_ENTER)) {// && (status_link == SDM_EXIST)){
		if (c131_test_status == C131_TEST_NOTYET) {
			c131_test_status = C131_TEST_ONGOING;
			// for initializing the status
			c131_ConfirmLoad(c131_GetCurrentLoadIndex());
			c131_relay_Update();
			Enable_SDMPWR();
		}
	} else if (keytype == KEY_RESET) {
		if ((c131_test_status == C131_TEST_FAIL) || (c131_test_status == C131_TEST_SUCCESSFUL)) {
			c131_test_status = C131_TEST_NOTYET;
			FailLed_Off();
			//for led pwr and states init
			Enable_LEDPWR();
			led_SetRelayStatus(C131_LED1_C1, RELAY_ON);
			led_SetRelayStatus(C131_LED1_C2, RELAY_ON);
			led_SetRelayStatus(C131_LED2, RELAY_ON);
			led_SetRelayStatus(C131_LED3, RELAY_ON);
			led_SetRelayStatus(C131_LED4_C2, RELAY_ON);
			led_SetRelayStatus(C131_LED5, RELAY_ON);
			c131_relay_Update();
		}
	}

	return 0;
}

int apt_GetDTCInfo(void)
{
	if (c131_test_status == C131_TEST_FAIL) {
		sprintf(dtc_info, "%02x ", (c131_dtc.pdtc + dtc_index)->dtc_hb);
		sprintf(dtc_info + 3, "%02x ", (c131_dtc.pdtc + dtc_index)->dtc_mb);
		sprintf(dtc_info + 6, "%02x ", (c131_dtc.pdtc + dtc_index)->dtc_lb);
		sprintf(dtc_info + 9, "%02x ", (c131_dtc.pdtc + dtc_index)->dtc_status);
		return (int)dtc_info;
	} else {
		return (int)str_nodtcinfo;
	}
}

int apt_SelectSDMDTC(int keytype)
{
	if (keytype == KEY_LEFT){
		dtc_index --;
		if (dtc_index < 0)
			dtc_index = 0;
	} else if (keytype == KEY_RIGHT){
		dtc_index ++;
		if (dtc_index >= c131_dtc.dtc_len)
			dtc_index = c131_dtc.dtc_len - 1;
	}

	return 0;
}

int c131_can_ClearHistoryDTC(void)
{
	time_t over_time;
	can_msg_t msg;

	can_bus -> send(&req_clrdtc_msg);
	over_time = time_get(50);
	while (time_left(over_time)) {
		if (!can_bus -> recv(&msg)) {
			if (msg.data[1] == 0x54)
				return 0;
			if (msg.data[1] == 0x7f)
				return 1;
		}
	}

	return 1;
}

int c131_can_GetDTC(c131_dtc_t *pc131_dtc)
{
	can_msg_t msg;
	time_t over_time;
	int num_frame;
	int i = 0;
	int data_len;
	unsigned char *p;

	p = (unsigned char *)(pc131_dtc->pdtc);
	c131_can_StartDiagnosis();

	//send dtc request
	can_bus -> send(&req_dtc_msg);

	//recv reponse
	over_time = time_get(50);
	while (time_left(over_time)) {
		if (!can_bus -> recv(&msg) && (msg.id == 0x7c2)) {
			can_msg_print(&msg, "\n");
			//single frame
			if (msg.data[0]>>4 == 0) {
				if (msg.data[1] == 0x59) {
					pc131_dtc->dtc_bExist = 0;
					pc131_dtc->dtc_bPositive = 1;
				} else {
					pc131_dtc->dtc_bPositive = 0;
				}
				return 0;
			} else {
			//multi frames
				pc131_dtc->dtc_bExist = 1;
				pc131_dtc->dtc_bPositive = 1;
				//calculate the number of can frames and dtc
				data_len = msg.data[0] & 0x0f;
				data_len <<= 8;
				data_len |= msg.data[1];
				num_frame = (data_len - 6)/7;
				if ((data_len - 6) % 7)
					num_frame ++;
				pc131_dtc->dtc_len = (data_len - 3) / 4;
				memcpy(p, msg.data + 5, 3);
				p += 3;

				//send flow control msgstrncpy
				can_bus -> send(&req_flow_msg);
				over_time = time_get(50);
				while (time_left(over_time) && (i < num_frame)) {
					if (!can_bus -> recv(&msg) && (msg.id == 0x7c2)) {
						can_msg_print(&msg, "\n");
						if (i < num_frame) {
							memcpy(p, msg.data + 1, 7);
							p += 7;
						}
						i ++;
						if (i == num_frame)
							return 0;
						over_time = time_get(50);
					}
				}
			}
		}
	}

	return 1;
}

void main(void)
{
	task_Init();
	c131_Init();

	while(1) {
		task_Update();
		c131_Update();
	}
}

//local functions
static int c131_GetLoad(apt_load_t ** pload, int index_load)
{
	if ((index_load >= 0) & (index_load < NUM_OF_LOAD)) {
		*pload = &c131_load[index_load];
		return 0;
	} else
		return -1;
}

static int c131_ConfirmLoad(int index_load)
{
	c131_current_load = index_load;
	c131_SetRelayRAM(c131_load[c131_current_load].load_ram);
	return 0;
}

static int c131_GetCurrentLoadIndex(void)
{
	return c131_current_load;
}

static int c131_GetSDMType(int index_load, char *pname)
{
	apt_load_t * pload;
	int len;

	if (c131_GetLoad(&pload, index_load) == 0) {
		if(!pload->load_bExist)
			return 1;
		else {
			memcpy(pname, pload->load_name, 15);
			pname[15] = 0x00;
			len = strlen(pname);
			memset(&pname[len], ' ', 15 - len);
		}
	}

	return 0;
}

static void c131_CanMSGInit(void)
{
	struct list_head *pos;
	struct can_queue_s *q;
	int i = 0;
	can_cfg_t cfg = CAN_CFG_DEF;
	cfg.baud = 500000;
	can_bus = &can1;
	can_bus -> init(&cfg);


	INIT_LIST_HEAD(&c131_ems_3.list);
	list_add(&c131_ems_3.list, &can_queue);

	INIT_LIST_HEAD(&c131_ems_4.list);
	list_add(&c131_ems_4.list, &can_queue);

	INIT_LIST_HEAD(&c131_abs_1.list);
	list_add(&c131_abs_1.list, &can_queue);

	INIT_LIST_HEAD(&c131_abs_2.list);
	list_add(&c131_abs_2.list, &can_queue);

	INIT_LIST_HEAD(&c131_abs_3.list);
	list_add(&c131_abs_3.list, &can_queue);

	INIT_LIST_HEAD(&c131_sas_1.list);
	list_add(&c131_sas_1.list, &can_queue);

	INIT_LIST_HEAD(&c131_tcu_3.list);
	list_add(&c131_tcu_3.list, &can_queue);

	list_for_each(pos, &can_queue) {
		q = list_entry(pos, can_queue_s, list);
		if(q -> timer == 0 || time_left(q -> timer) < 0) {
			q -> timer = time_get(0 + i);
			i += 6;
		}
	}
}

//can related function
static void c131_can_SendOtherECUMsg(void)
{
	struct list_head *pos;
	struct can_queue_s *q;
	time_t over_time;

	list_for_each(pos, &can_queue) {
		q = list_entry(pos, can_queue_s, list);
		if(q -> timer == 0 || time_left(q -> timer) < 0) {
			over_time = time_get(10);
			q -> timer = time_get(q -> ms);
			if ((q->msg).id == 0xb4) {
				(q->msg).data[7] ++;
				(q->msg).data[7] &= 0x0f;
				(q->msg).data[0] = (q->msg).data[7];
			}
			if ((q->msg).id == 0x96) {
				(q->msg).data[6] ++;
				(q->msg).data[0] = (q->msg).data[6];
			}
			while (can_bus -> send(&q -> msg)) {
				if (time_left(over_time) < 0)
					return;
			}
		}
	}
}

static int c131_can_GetAirbagMsg(can_msg_t *pmsg)
{
	if (!can_bus -> recv(pmsg)) {
		if ((pmsg->id == AIRBAG_CAN_ID) && (pmsg->dlc == 8))
			return 0;
	}

	return 1;
}

static int c131_can_StartDiagnosis(void)
{
	time_t over_time;
	can_msg_t msg;

	can_bus -> send(&req_start_msg);

	over_time = time_get(50);
	while (time_left(over_time)) {
		if (!can_bus -> recv(&msg)) {
			if (msg.data[1] == 0x50)
				return 0;
			if (msg.data[1] == 0x7f)
				return 1;
		}
	}

	return 1;
}
