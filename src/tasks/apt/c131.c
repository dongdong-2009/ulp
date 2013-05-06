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
#include "priv/usdt.h"

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
static char typeinfo_ram[16];	//the last char for EOC

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

static const char str_nodtcinfo[] = "No   DTC   Info ";
static char dtc_info[16];
static char dtc_index;
static dtc_t dtc_buffer[64];
static c131_dtc_t c131_dtc;

//for load config variant nvm
static apt_load_t c131_load[NUM_OF_LOAD] __nvm;
static int c131_current_load __nvm;
static int num_load;

//for const can frame which will not change
static struct can_queue_s c131_ems_3 = {
	.ms = 10,
	.msg = {0x11c, 8, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
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
	.msg = {0x12c, 8, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
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

static const can_msg_t eeprom1_msg = {
	.id = C131_DIAG_REQ_ID,
	.dlc = 8,
	.data = {0x02, 0x10, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00},
};

static const can_msg_t eeprom2_msg = {
	.id = C131_DIAG_REQ_ID,
	.dlc = 8,
	.data = {0x02, 0x27, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00},
};

static can_msg_t eeprom3_msg = {
	.id = C131_DIAG_REQ_ID,
	.dlc = 8,
	.data = {0x04, 0x27, 0x02, 0xff, 0xff, 0x00, 0x00, 0x00},
};

static const can_msg_t eeprom4_msg = {
	.id = C131_DIAG_REQ_ID,
	.dlc = 8,
	.data = {0x21, 0x00, 0x00, 0x00, 0xfc, 0x00, 0x00, 0x00},
};
// static const can_msg_t req_flow_msg = {
	// .id = C131_DIAG_REQ_ID,
	// .dlc = 8,
	// .data = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
// };

static LIST_HEAD(can_queue);
static const can_bus_t *can_bus;

//for local functions
static int c131_GetSDMType(int index_load, char *pname);
static int c131_ConfirmLoad(int index_load);
static int c131_DelCurrentLoad(void);
static void c131_InitMsg(void);
static int c131_SendOtherECUMsg(void);
static int c131_StartDiagnosis(void);
static int c131_GetAirbagMsg(can_msg_t *pmsg);
static void c131_Tim2Init(void);
static int c131_mdelay(int ms);
static void c131_InitLedConfig(void);
static void c131_Init(void)
{
	int i;
	char name_temp[16];
	int hdlg;

	can_cfg_t cfg = CAN_CFG_DEF;
	cfg.baud = 500000;
	can_bus = &can1;
	can_bus -> init(&cfg);
	usdt_Init(can_bus);

	//init config which has been confirmed
	c131_current_load = -1;
	strcpy(typeinfo_ram, "No Invalid Cfg ");
	memset(name_temp, 0xff, 16);
	for (i = 0; i < NUM_OF_LOAD; i++) {
		if (memcmp(c131_load[i].load_name, name_temp, 16) == 0) {
			c131_load[i].load_bExist = 0;
		}
		if (c131_load[i].load_bExist)
			num_load ++;
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
	dtc_index = 0;

	//lowlevel app init
	c131_driver_Init();
	c131_diag_Init();
	c131_InitMsg();
	c131_Tim2Init();

	//c131_InitLedConfig();					//for led pwr and states init
	c131_dtc.pdtc = dtc_buffer;				//for dtc related varible init
}

static void c131_Update(void)
{
	can_msg_t msg;

	//for device under testing check
	if (Get_SDMStatus()) {
		status_link = SDM_UNEXIST;
		Disable_SDMPWR();
		Disable_LEDPWR();
	} else {
		status_link = SDM_EXIST;
	}

	//for sdm testing sequence
	if (c131_test_status == C131_TEST_ONGOING) {
		//stage one, config the relays(loads)
		sw_SetRelayStatus(C131_SW2_C1 | C131_SW5_C1, RELAY_ON);
		sw_SetRelayStatus(C131_SW2_C2 | C131_SW5_C2, RELAY_OFF);
		sw_SetRelayStatus(C131_SW3 | C131_SW4, RELAY_ON);
		c131_relay_Update();
		c131_mdelay(12000);
#if 0   //added by david 2012,5,6
		while (c131_GetAirbagMsg(&msg));			//check the msg of sdm
		can_msg_print(&msg, "\n");
		//for DSB and PSB and PAD and SBR
		if (!(((msg.data[2]&0x0f) == 0x05) && (msg.data[5]&0x01) && ((msg.data[7]&0x04) == 0x0))) {
			c131_test_status = C131_TEST_FAIL;
			printf("Failed : DSB and PSB and PAD and SBR!\n");
		}

		//stage two, config the relays(loads)
		sw_SetRelayStatus(C131_SW2_C1 | C131_SW5_C1, RELAY_ON);
		sw_SetRelayStatus(C131_SW2_C2 | C131_SW5_C2, RELAY_ON);
		sw_SetRelayStatus(C131_SW3 | C131_SW4, RELAY_OFF);
		c131_relay_Update();
		c131_mdelay(2000);
		while (c131_GetAirbagMsg(&msg) == 0);			//check the msg of sdm
		can_msg_print(&msg, "\n");
		//for DSB and PSB and PAD and SBR
		if (!(((msg.data[2]&0x0f) == 0x0) && ((msg.data[5]&0x01) == 0x0) && (msg.data[7]&0x04))) {
			c131_test_status = C131_TEST_FAIL;
			printf("Failed : DSB and PSB and PAD and SBR!\n");
		}

		//check whether there is dtc data
		c131_mdelay(5000);
		while(c131_GetAirbagMsg(&msg));
		can_msg_print(&msg, "\n");
		if (msg.data[5] & 0x02) {							//airbag warning lamp detect
			c131_test_status = C131_TEST_FAIL;
			printf("Failed : airbag warning lamp detect!\n");
		}
#endif
		c131_GetDTC(&c131_dtc);								//DTC detect
		if (c131_dtc.dtc_bExist) {
			c131_test_status = C131_TEST_FAIL;
			printf("Failed : DTC detect!\n");
		}

		if (c131_test_status == C131_TEST_FAIL) {		//test sequence fail
			FailLed_On();
			Disable_SDMPWR();
			Disable_LEDPWR();
		} else {
			c131_test_status = C131_TEST_SUCCESSFUL;	//test sequence passed
			FailLed_Off();
			Disable_SDMPWR();
			Disable_LEDPWR();
		}
	}
}

int apt_AddLoad(apt_load_t * pload)
{
	int i;

	for (i = 0; i < NUM_OF_LOAD; i++) {					//for edit existing config
		if (strcmp(c131_load[i].load_name, pload->load_name) == 0) {
			c131_load[i] = (*pload);
			c131_load[i].load_bExist =1;
                        num_load ++;
			nvm_save();
			c131_index_load = i;  //display updated sdm model
			c131_GetSDMType(c131_index_load, sdmtype_ram);
			return 0;
		}
	}
	for (i = 0; i < NUM_OF_LOAD; i++) {					//for adding new config
		if (c131_load[i].load_bExist == 0) {
			c131_load[i] = (*pload);
			c131_load[i].load_bExist =1;
			num_load ++;
			nvm_save();
			c131_index_load = i;  //display new sdm model
			c131_GetSDMType(c131_index_load, sdmtype_ram);
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

int apt_GetTypeInfo(void)
{
	return (int)typeinfo_ram;
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
		c131_GetSDMType(c131_index_load, typeinfo_ram);
	} else if(keytype == KEY_RESET) {
		if (c131_DelCurrentLoad() == 0) {
			if (num_load == 0) {
				strcpy(sdmtype_ram, "No Invalid Cfg ");
				return 0;
			}
			do {
				c131_index_load ++;
				if (c131_index_load >= NUM_OF_LOAD) 
					c131_index_load = 0;
			} while(c131_GetSDMType(c131_index_load, sdmtype_ram));
		}
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
//added by david, 2012,5,6
		c131_DiagSW();
		c131_DiagLOOP();
//ended
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
	if ((keytype == KEY_ENTER) && (status_link == SDM_EXIST) && (c131_current_load != -1)) {
		if (c131_test_status == C131_TEST_NOTYET) {
			c131_test_status = C131_TEST_ONGOING;
			// for initializing the status
			c131_ConfirmLoad(c131_GetCurrentLoadIndex());
			c131_relay_Update();
			c131_mdelay(20);
			Enable_SDMPWR();
			Enable_LEDPWR();
		}
	} else if (keytype == KEY_RESET) {
		if ((c131_test_status == C131_TEST_FAIL) || (c131_test_status == C131_TEST_SUCCESSFUL)) {
			c131_test_status = C131_TEST_NOTYET;
			FailLed_Off();
			//for led pwr and states init
			//c131_InitLedConfig();
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

int c131_ClearHistoryDTC(void)
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

int c131_GetDTC(c131_dtc_t *pc131_dtc)
{
	can_msg_t msg_res, *pRes;
	int i = 0, data_len, msg_len;
	unsigned char *p;

	p = (unsigned char *)(pc131_dtc->pdtc);
	c131_StartDiagnosis();

	if (usdt_GetDiagFirstFrame(&req_dtc_msg, 1, NULL, &msg_res, &msg_len))
		return 1;

	if (msg_len > 1) {
		pRes = (can_msg_t *) sys_malloc(msg_len * sizeof(can_msg_t));
		if (pRes == NULL)
			return 1;
		*pRes = msg_res;
		if(usdt_GetDiagLeftFrame(pRes, msg_len))
			return 1;

		//multi frames
		pc131_dtc->dtc_bExist = 1;
		pc131_dtc->dtc_bPositive = 1;
		data_len = msg_res.data[0] & 0x0f;		//calculate the number of can frames and dtc
		data_len <<= 8;
		data_len |= msg_res.data[1];
		pc131_dtc->dtc_len = (data_len - 3) / 4;
		memcpy(p, msg_res.data + 5, 3);
		p += 3;
		for (i = 1; i < msg_len; i++) {
			memcpy(p, ((pRes + i)->data) + 1, 7);
			p += 7;
		}
		sys_free(pRes);
	} else if (msg_len == 1) {
		if (msg_res.data[1] == 0x59)
			pc131_dtc->dtc_bPositive = 1;
		else {
			pc131_dtc->dtc_bPositive = 0;
			return 1;
		}
		//single frame
		if (msg_res.data[0] == 7) {
			pc131_dtc->dtc_bExist = 1;
			pc131_dtc->dtc_len = 1;
			memcpy(p, msg_res.data + 4, 4);
		} else
			pc131_dtc->dtc_bExist = 0;
		return 0;
	}

	return 0;
}

int c131_GetDiagInfo(can_msg_t *pReq, can_msg_t *pRes, int *plen)
{
	can_msg_t msg_res;
	int msg_len;

	if (usdt_GetDiagFirstFrame(pReq, 1, NULL, &msg_res, &msg_len))
		return 1;
	if (msg_len > 1) {
		*pRes = msg_res;
		if(usdt_GetDiagLeftFrame(pRes, msg_len))
			return 1;
		*plen = msg_len;
	} else if (msg_len == 1) {
		*pRes = msg_res;
		*plen = msg_len;
	}

	return 0;
}

int c131_GetEEPROMInfo(can_msg_t *pReq, can_msg_t *pRes, char data_len, int *plen)
{
	can_msg_t msg_res, msg_req[2];
	int msg_len, result = 0;

	result = usdt_GetDiagFirstFrame(&eeprom1_msg, 1, NULL, &msg_res, &msg_len);
	result += usdt_GetDiagFirstFrame(&eeprom2_msg, 1, NULL, &msg_res, &msg_len);
	eeprom3_msg.data[3] = ~msg_res.data[3];
	eeprom3_msg.data[4] = ~msg_res.data[4];
	result += usdt_GetDiagFirstFrame(&eeprom3_msg, 1, NULL, &msg_res, &msg_len);
	if (result)
		return 1;

	msg_req[0] = *pReq;
	msg_req[1] = eeprom4_msg;
	msg_req[1].data[4] = data_len;

	if (usdt_GetDiagFirstFrame(msg_req, 2, NULL, &msg_res, &msg_len))
		return 1;
	if (msg_len > 1) {
		*pRes = msg_res;
		if(usdt_GetDiagLeftFrame(pRes, msg_len))
			return 1;
		*plen = msg_len;
	} else if (msg_len == 1) {
		*pRes = msg_res;
		*plen = msg_len;
	}

	return 0;
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

void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET) {
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
		if (Get_SDMPWRStatus())
			c131_SendOtherECUMsg();
	}
}

//init tim2 interrupt config, 5ms interval
static void c131_Tim2Init(void)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	/* Time base configuration */
	TIM_TimeBaseStructure.TIM_Period = 800;		//5ms interrupt
	TIM_TimeBaseStructure.TIM_Prescaler = 720 - 1;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
	/* TIM IT enable */
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
	/* TIM2 enable counter */
	TIM_Cmd(TIM2, ENABLE);
	TIM_ClearFlag(TIM2, TIM_FLAG_Update);

	/* Enable the TIM2 gloabal Interrupt */
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
        NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);

	NVIC_Init(&NVIC_InitStructure);
}

int c131_mdelay(int ms)
{
	int left;
	time_t deadline = time_get(ms);
	do {
		left = time_left(deadline);
		if(left >= 10) { //system update period is expected less than 10ms
			task_Update();
		}
	} while(left > 0);

	return 0;
}
//local functions
int c131_GetLoad(apt_load_t ** pload, int index_load)
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

int c131_GetCurrentLoadIndex(void)
{
	return c131_current_load;
}

static int c131_DelCurrentLoad(void)
{
	if (c131_current_load == c131_index_load)
		return 1;
	else {
		c131_load[c131_index_load].load_bExist = 0;
		num_load --;
		nvm_save();
		return 0;
	}
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

static void c131_InitMsg(void)
{
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
}

static int c131_SendOtherECUMsg(void)
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
					return 1;
			}
		}
	}

	return 0;
}

static int c131_GetAirbagMsg(can_msg_t *pmsg)
{
	if (!can_bus -> recv(pmsg)) {
		if ((pmsg->id == AIRBAG_CAN_ID))
			return 0;
	}

	return 1;
}

static int c131_StartDiagnosis(void)
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

static void c131_InitLedConfig(void)
{
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
