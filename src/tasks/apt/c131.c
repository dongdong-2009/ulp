/*
 * david@2011 initial version
 */
#include <string.h>
#include "config.h"
#include "stm32f10x.h"
#include "sys/task.h"
#include "osd/osd.h"
#include "nvm.h"
#include "key.h"
#include "time.h"
#include "c131_driver.h"
#include "c131_diag.h"
#include "c131.h"

#define C131_STAGE1_MS		1000
#define C131_STAGE2_MS		1000
#define C131_GETDTC_MS		30000

//local key may
static const int keymap[] = {
	KEY_UP,
	KEY_DOWN,
	KEY_ENTER,
	KEY_RIGHT,
	KEY_LEFT,
	KEY_RESET,
	KEY_NONE
};

//for application state machine
static int c131_test_status;
static int c131_stage_status;
static time_t stage_timer;
static time_t dtc_timer;

const static char stage1_msg[8] = {
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
};

const static char stage2_msg[8] = {
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
};

//for load config variant nvm
static c131_load_t c131_load[NUM_OF_LOAD] __nvm;
static int c131_current_load __nvm;
static char c131_mode = C131_MODE_NORMAL;
static char c131_link_status;


static void c131_Init(void)
{
	int i;
	char name_temp[16];
	int hdlg;

	//init config which has been confirmed
	nvm_init();
	c131_current_load = 0;
	memset(name_temp, 0xff, 16);
	for (i = 0; i < NUM_OF_LOAD; i++) {
		if (memcmp(c131_load[i].load_name, name_temp, 16) == 0) {
			c131_load[i].load_bExist = 0;
		}
		//nvm_save();
	}

#ifdef CONFIG_TASK_OSD
	//init dialogue
	hdlg = osd_ConstructDialog(&c131_dlg);
	osd_SetActiveDialog(hdlg);
	key_SetLocalKeymap(keymap);
#endif

	//lowlevel app init
	c131_driver_Init();
	c131_diag_Init();

	c131_link_status = 0;

	//for sdm test status
	c131_test_status = C131_TEST_NOTYET;
	c131_stage_status = C131_STAGE1_RELAY;
}

static void c131_Update(void)
{
	can_msg_t msg;

	//for device under testing check
	if (Get_SDMStatus()) {
		c131_link_status = SDM_UNEXIST;
	} else {
		c131_link_status = SDM_EXIST;
	}

	//for sdm testing status machine
	switch (c131_test_status) {
		case C131_TEST_ONGOING:
			c131_can_SendOtherECUMsg();

			//embedded test flow state machine
			switch (c131_stage_status) {
				case C131_STAGE1_RELAY:
					sw_SetRelayStatus(C131_SW2_C1 | C131_SW5_C1, RELAY_ON);
					sw_SetRelayStatus(C131_SW2_C2 | C131_SW3 | C131_SW4 | C131_SW5_C2, RELAY_OFF);
					c131_relay_Update();
					stage_timer = time_get(C131_STAGE1_MS);
					dtc_timer = time_get(C131_GETDTC_MS);
					c131_stage_status = C131_STAGE1_CANMSG;
					break;

				case C131_STAGE1_CANMSG:
					if (time_left(stage_timer) <= 0) {
						if (c131_can_GetAirbagMsg(&msg) == 0) {
							if (memcmp(stage1_msg, msg.data, 8) == 0)
								c131_stage_status = C131_STAGE2_RELAY;
							else
								c131_test_status = C131_TEST_FAIL;
						}
					}
					break;

				case C131_STAGE2_RELAY:
					sw_SetRelayStatus(C131_SW2_C1 | C131_SW5_C1, RELAY_ON);
					sw_SetRelayStatus(C131_SW2_C2 | C131_SW3 | C131_SW4 | C131_SW5_C2, RELAY_ON);
					c131_relay_Update();
					stage_timer = time_get(C131_STAGE2_MS);
					c131_stage_status = C131_STAGE2_CANMSG;
					break;

				case C131_STAGE2_CANMSG:
					if (time_left(stage_timer) <= 0) {
						if (c131_can_GetAirbagMsg(&msg) == 0) {
							if (memcmp(stage2_msg, msg.data, 8) == 0)
								c131_stage_status = C131_GET_DTCMSG;
							else
								c131_test_status = C131_TEST_FAIL;
						}
					}
					break;

				case C131_GET_DTCMSG:
					//determine the dtc data
					if (time_left(dtc_timer) <= 0) {
						c131_can_GetDTC();
						c131_test_status = C131_TEST_NOTYET;
						c131_stage_status = C131_STAGE1_RELAY;
						c131_can_ClearHistoryDTC();
						Disable_SDMPWR();
						c131_test_status = C131_TEST_SUCCESSFUL;
					}
					break;

				default :
					break;
			}
			break;
		
		case C131_TEST_FAIL:
			break;
		
		case C131_TEST_SUCCESSFUL:
			break;
		
		case C131_TEST_NOTYET:
		default:
			break;
	}
}

int c131_SetMode(int workmode)
{
	c131_mode = workmode;
	return 0;
}

int c131_GetMode(void)
{
	return c131_mode;
}

int c131_GetLoad(c131_load_t ** pload, int index_load)
{
	if ((index_load >= 0) & (index_load < NUM_OF_LOAD)) {
		*pload = &c131_load[index_load];
		return 0;
	} else
		return -1;
}

int c131_AddLoad(c131_load_t * pload)
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
			return 0;
		}
	}

	return 1;
}

int c131_ConfirmLoad(int index_load)
{
	c131_current_load = index_load;
	c131_SetRelayRAM(c131_load[c131_current_load].load_ram);
	return 0;
}

int c131_GetCurrentLoadIndex(void)
{
	return c131_current_load;
}

int c131_GetSDMType(int index_load, char *pname)
{
	c131_load_t * pload;
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

int c131_GetLinkInfo(void)
{
	return c131_link_status;
}

int c131_GetTestStatus(void)
{
	return c131_test_status;
}

int c131_SetTeststatus(int status)
{
	c131_test_status = status;

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
