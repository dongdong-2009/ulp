#include "config.h"
#include "debug.h"
#include "time.h"
#include "nvm.h"
#include "key.h"
#include "osd/osd.h"
#include "sys/task.h"
#include "sys/sys.h"
#include "led.h"

enum {
	STATUS_OFF = 0,
	STATUS_ON
};

#define DELAY_MS 1000
#define REPEAT_MS 10

//for valve head demo
#define VH_OVERFLOW_STAGE_ONE	1500*60	//1.5min
#define VH_OVERFLOW_STAGE_TWO	2000*60	//1.5min

#define VH_ALARM_BUZZER		GPIO_Pin_9
#define VH_PRODUCT_CTRL		GPIO_Pin_4
#define VH_PRODUCT_FB		GPIO_Pin_5

#define PUT_ALARM_BUZZER(ba)		GPIO_WriteBit(GPIOC, VH_ALARM_BUZZER, ba)
#define PUT_PRODUCT_CTRL(ba)		GPIO_WriteBit(GPIOC, VH_PRODUCT_CTRL, ba)
#define GET_PRODUCT_FB()			GPIO_ReadInputDataBit(GPIOC, VH_PRODUCT_FB)

enum {
	VHD_IDLE,
	VHD_START,
	VHD_MIDDLE,
	VHD_CHECK,
	VHD_ALARM,
};
static int vhd_sm;
static int start_flag;
time_t vhd_overflow_timer;

static int loops_up_limit __nvm;
static int loops_dn_limit __nvm;
static int loops_counter = 0;

const char str_up_limit[] = "1";
const char str_dn_limit[] = "4";
const char str_loops[] = "0";

//display
static int get_up_limit(void);
static int get_dn_limit(void);
static int get_loops(void);

//modify settings
static int sel_group(const osd_command_t *cmd);
static int set_up_limit(const osd_command_t *cmd);
static int set_dn_limit(const osd_command_t *cmd);

const osd_item_t items_up_limit[] = {
	{0, 2, 1, 1, (int)str_up_limit, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 2, 8, 1, (int)get_up_limit, ITEM_DRAW_INT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_ALWAYS, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_dn_limit[] = {
	{0, 1, 1, 1, (int)str_dn_limit, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 1, 8, 1, (int)get_dn_limit, ITEM_DRAW_INT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_ALWAYS, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_loops[] = {
	{0, 0, 1, 1, (int)str_loops, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{0, 0, 8, 1, (int)get_loops, ITEM_DRAW_INT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_ALWAYS, ITEM_RUNTIME_V},
	NULL,
};

const osd_command_t cmds_up_limit[] = {
	{.event = KEY_UP, .func = sel_group},
	{.event = KEY_DOWN, .func = sel_group},
	{.event = KEY_RIGHT, .func = set_up_limit},
	{.event = KEY_LEFT, .func = set_up_limit},
	{.event = KEY_RESET, .func = set_up_limit},
	{.event = KEY_ENTER, .func = set_up_limit},
	{.event = KEY_0, .func = set_up_limit},
	{.event = KEY_1, .func = set_up_limit},
	{.event = KEY_2, .func = set_up_limit},
	{.event = KEY_3, .func = set_up_limit},
	{.event = KEY_4, .func = set_up_limit},
	{.event = KEY_5, .func = set_up_limit},
	{.event = KEY_6, .func = set_up_limit},
	{.event = KEY_7, .func = set_up_limit},
	{.event = KEY_8, .func = set_up_limit},
	{.event = KEY_9, .func = set_up_limit},
	NULL,
};

const osd_command_t cmds_dn_limit[] = {
	{.event = KEY_UP, .func = sel_group},
	{.event = KEY_DOWN, .func = sel_group},
	{.event = KEY_RIGHT, .func = set_dn_limit},
	{.event = KEY_LEFT, .func = set_dn_limit},
	{.event = KEY_RESET, .func = set_dn_limit},
	{.event = KEY_ENTER, .func = set_dn_limit},
	{.event = KEY_0, .func = set_dn_limit},
	{.event = KEY_1, .func = set_dn_limit},
	{.event = KEY_2, .func = set_dn_limit},
	{.event = KEY_3, .func = set_dn_limit},
	{.event = KEY_4, .func = set_dn_limit},
	{.event = KEY_5, .func = set_dn_limit},
	{.event = KEY_6, .func = set_dn_limit},
	{.event = KEY_7, .func = set_dn_limit},
	{.event = KEY_8, .func = set_dn_limit},
	{.event = KEY_9, .func = set_dn_limit},
	NULL,
};

const osd_command_t cmds_loops[] = {
	{.event = KEY_UP, .func = sel_group},
	{.event = KEY_DOWN, .func = sel_group},
	NULL,
};

const osd_group_t grps[] = {
	{.items = items_up_limit, .cmds = cmds_up_limit, .order = 2, .option = GROUP_DRAW_FULLSCREEN},
	{.items = items_dn_limit, .cmds = cmds_dn_limit, .order = 1, .option = GROUP_DRAW_FULLSCREEN},
	{.items = items_loops, .cmds = cmds_loops, .order = 0, .option = GROUP_DRAW_FULLSCREEN},
	NULL,
};

osd_dialog_t dlg = {
	.grps = grps,
	.cmds = NULL,
	.func = NULL,
	.option = 0,
};

static int sel_group(const osd_command_t *cmd)
{
	int result = -1;

	if(cmd->event == KEY_UP)
		result = osd_SelectPrevGroup();
	else
		result = osd_SelectNextGroup();

	return result;
}

static int get_up_limit(void)
{
	return loops_up_limit;
}

static int get_dn_limit(void)
{
	return loops_dn_limit;
}

static int get_loops(void)
{
	return loops_counter;
}

static int set_up_limit(const osd_command_t *cmd)
{
	int loops = loops_up_limit;

	switch(cmd -> event){
	case KEY_LEFT:
		loops = (loops > 0) ? loops - 1 : 0;
		loops_up_limit = loops;
		key_SetKeyScenario(DELAY_MS, REPEAT_MS);
		break;
	case KEY_RIGHT:
		loops ++;
		loops_up_limit = loops;
		key_SetKeyScenario(DELAY_MS, REPEAT_MS);
		break;
	case KEY_RESET:
		break;
	case KEY_ENTER:
		nvm_save();
		break;
	default:
		loops = key_SetEntryAndGetDigit();
		loops_up_limit = loops;
		break;
	}

	return 0;
}

static int set_dn_limit(const osd_command_t *cmd)
{
	int loops = loops_dn_limit;

	switch(cmd -> event){
	case KEY_LEFT:
		loops = (loops > 0) ? loops - 1 : 0;
		loops_dn_limit = loops;
		key_SetKeyScenario(DELAY_MS, REPEAT_MS);
		break;
	case KEY_RIGHT:
		loops ++;
		loops_dn_limit = loops;
		key_SetKeyScenario(DELAY_MS, REPEAT_MS);
		break;
	case KEY_RESET:
		break;
	case KEY_ENTER:
		nvm_save();
		break;
	default:
		loops = key_SetEntryAndGetDigit();
		loops_dn_limit = loops;
		break;
	}

	return 0;
}

void vhd_Init(void)
{
	int hdlg;
	GPIO_InitTypeDef GPIO_InitStructure;

	//for dlg of osd
	hdlg = osd_ConstructDialog(&dlg);
	osd_SetActiveDialog(hdlg);
	
	//for gpio init
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

	GPIO_InitStructure.GPIO_Pin = VH_ALARM_BUZZER | VH_PRODUCT_CTRL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
  
	GPIO_InitStructure.GPIO_Pin = VH_PRODUCT_FB;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	//for init state
	PUT_ALARM_BUZZER(STATUS_OFF);
	PUT_PRODUCT_CTRL(STATUS_OFF);
	led_on(LED_GREEN);
	led_off(LED_RED);
	led_off(LED_YELLOW);

	//for related data init
	vhd_sm = VHD_IDLE;
	start_flag = STATUS_OFF;
}

void vhd_Update(void)
{
	switch (vhd_sm) {
		case VHD_IDLE:
			if (start_flag) {
				vhd_sm = VHD_START;
				PUT_PRODUCT_CTRL(STATUS_ON);
				vhd_overflow_timer = time_get();
			}
			else
				return;
			break;
		case VHD_START:
			PUT_PRODUCT_CTRL(STATUS_ON);
			
	
	
	}
	
	
	
}

void main(void)
{
	task_Init();
	vhd_Init();
	while(1) {
		task_Update();
		vhd_Update();
	}
}