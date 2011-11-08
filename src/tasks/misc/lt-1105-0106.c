#include "config.h"
#include "debug.h"
#include "ulp_time.h"
#include "nvm.h"
#include "key.h"
#include "osd/osd.h"
#include "sys/task.h"
#include "sys/sys.h"
#include "led.h"
#include "stm32f10x_gpio.h"

#define STATUS_OFF		Bit_RESET
#define STATUS_ON		Bit_SET

#define PRODUCT_OFF		Bit_SET
#define PRODUCT_ON		Bit_RESET

#define DELAY_MS 1000
#define REPEAT_MS 10
#define FLASH_MS 500

//for valve head demo
#define VH_OVERFLOW_STAGE_ONE			1500*60 //1.5min
#define VH_OVERFLOW_STAGE_TWO			10000*60 //10min
#define VH_OVERFLOW_FR_STAGEONE			1500*60 //1.5min
#define VH_OVERFLOW_FR_STAGETWO			10000*60 //10min
#define VH_PRODUCT_RESET_TIME			3
#define VH_LOOPS_OVERFLOW				12000

#define VH_ALARM_BUZZER_PIN		GPIO_Pin_9
#define VH_PRODUCT_CTRL_PIN		GPIO_Pin_4
#define VH_PRODUCT_FB_PIN		GPIO_Pin_5

#define PUT_ALARM_BUZZER(ba)	GPIO_WriteBit(GPIOC,VH_ALARM_BUZZER_PIN,ba)
#define PUT_PRODUCT_CTRL(ba)	GPIO_WriteBit(GPIOC,VH_PRODUCT_CTRL_PIN,ba)
#define GET_PRODUCT_FB()		GPIO_ReadInputDataBit(GPIOC,VH_PRODUCT_FB_PIN)

enum {
	VHD_STATUS_IDLE,
	VHD_STATUS_FIRSTROUND,
	VHD_STATUS_START,
	VHD_STATUS_CHECK,
	VHD_STATUS_ALARM,
	VHD_STATUS_TOPLIMIT
} vhd_status;

enum {
	VHD_FR_ONE,
	VHD_FR_TWO
} vhd_fr_status;
static int start_flag;
time_t vhd_overflow_timer;

//for osd state
static int loops_up_limit __nvm;
static int loops_dn_limit __nvm;
static int loops_counter __nvm;;

static int vhd_status_flash;
static time_t vhd_status_timer;
static time_t vhd_loops_timer;

const char str_up_limit[] = "1";
const char str_dn_limit[] = "3";
const char str_loops[] = "2";

//display
static int get_up_limit(void);
static int get_dn_limit(void);
static int get_loops(void);
static int get_status(void);

//modify settings
static int sel_group(const osd_command_t *cmd);
static int set_up_limit(const osd_command_t *cmd);
static int set_dn_limit(const osd_command_t *cmd);
static int vhd_play(const osd_command_t *cmd);
static int vhd_goto_dnlimit(const osd_command_t *cmd);

const osd_item_t items_up_limit[] = {
	{0, 2, 1, 1, (int)str_up_limit, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{1, 2, 8, 1, (int)get_up_limit, ITEM_DRAW_INT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_ALWAYS, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_dn_limit[] = {
	{0, 1, 1, 1, (int)str_dn_limit, ITEM_DRAW_TXT, ITEM_ALIGN_LEFT, ITEM_UPDATE_NEVER, ITEM_RUNTIME_NONE},
	{1, 1, 8, 1, (int)get_dn_limit, ITEM_DRAW_INT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_ALWAYS, ITEM_RUNTIME_V},
	NULL,
};

const osd_item_t items_loops[] = {
	{0, 0, 1, 1, (int)get_status, ITEM_DRAW_INT, ITEM_ALIGN_LEFT, ITEM_UPDATE_ALWAYS, ITEM_RUNTIME_V},
	{1, 0, 8, 1, (int)get_loops, ITEM_DRAW_INT, ITEM_ALIGN_RIGHT, ITEM_UPDATE_ALWAYS, ITEM_RUNTIME_V},
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
	{.event = KEY_PLAY, .func = vhd_play},
	{.event = KEY_ENTER, .func = vhd_goto_dnlimit},
	NULL,
};

const osd_group_t grps[] = {
	{.items = items_up_limit, .cmds = cmds_up_limit, .order = 2, .option = GROUP_DRAW_FULLSCREEN},
	{.items = items_loops, .cmds = cmds_loops, .order = 0, .option = GROUP_DRAW_FULLSCREEN},
	{.items = items_dn_limit, .cmds = cmds_dn_limit, .order = 1, .option = GROUP_DRAW_FULLSCREEN},
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
	nvm_init();

	if(cmd->event == KEY_DOWN)
		result = osd_SelectPrevGroup();
	else
		result = osd_SelectNextGroup();

	vhd_loops_timer = time_get(VH_LOOPS_OVERFLOW);

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

static int get_status(void)
{
	if ((vhd_status != VHD_STATUS_ALARM) && (vhd_status != VHD_STATUS_TOPLIMIT)) {
		return 2;
	}

	if(time_left(vhd_status_timer) < 0) {
		vhd_status_timer = FLASH_MS;
		vhd_status_flash ^= 1;
	}
	return (vhd_status_flash * 2);
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

	vhd_loops_timer = time_get(VH_LOOPS_OVERFLOW);

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
		loops_counter = loops_dn_limit;
		nvm_save();
		break;
	default:
		loops = key_SetEntryAndGetDigit();
		loops_dn_limit = loops;
		break;
	}

	vhd_loops_timer = time_get(VH_LOOPS_OVERFLOW);

	return 0;
}

static int vhd_play(const osd_command_t *cmd)
{
	if (vhd_status == VHD_STATUS_IDLE) {
		start_flag = STATUS_ON;
	} else {
		start_flag = STATUS_OFF;
	}

	return 0;
}

static int vhd_goto_dnlimit(const osd_command_t *cmd)
{
	loops_counter = loops_dn_limit;
	nvm_save();

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

	GPIO_InitStructure.GPIO_Pin = VH_ALARM_BUZZER_PIN | VH_PRODUCT_CTRL_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
  
	GPIO_InitStructure.GPIO_Pin = VH_PRODUCT_FB_PIN;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	//for init state
	PUT_ALARM_BUZZER(STATUS_OFF);
	PUT_PRODUCT_CTRL(PRODUCT_OFF);
	led_off(LED_GREEN);
	led_off(LED_RED);
	led_off(LED_YELLOW);

	//for related data init
	vhd_status = VHD_STATUS_IDLE;
	vhd_fr_status = VHD_FR_ONE;
	start_flag = STATUS_OFF;
	vhd_loops_timer = time_get(VH_LOOPS_OVERFLOW);
	sdelay(8);
	osd_SelectGroup(&grps[1]);
}

void vhd_Update(void)
{
	switch (vhd_status) {
		case VHD_STATUS_IDLE:
			if (start_flag) {
				PUT_PRODUCT_CTRL(PRODUCT_ON);
				vhd_overflow_timer = time_get(VH_OVERFLOW_FR_STAGEONE);
				led_flash(LED_GREEN);
				vhd_status = VHD_STATUS_FIRSTROUND;
			}
			break;
		case VHD_STATUS_FIRSTROUND:
			if (vhd_fr_status == VHD_FR_ONE) {
				if(time_left(vhd_overflow_timer) < 0) {
					//for delay
					PUT_PRODUCT_CTRL(PRODUCT_OFF);
					sdelay(VH_PRODUCT_RESET_TIME);
					PUT_PRODUCT_CTRL(PRODUCT_ON);
					vhd_fr_status = VHD_FR_TWO;
					vhd_overflow_timer = time_get(VH_OVERFLOW_FR_STAGETWO);
				}
			} else {
				if(time_left(vhd_overflow_timer) < 0) {
					//for alarm
					PUT_PRODUCT_CTRL(PRODUCT_OFF);
					PUT_ALARM_BUZZER(STATUS_ON);
					led_on(LED_RED);
					led_off(LED_GREEN);
					vhd_status = VHD_STATUS_ALARM;
					break;
				}
			}
			if (GET_PRODUCT_FB() == 0) {
				loops_counter ++;
				nvm_save();
				PUT_PRODUCT_CTRL(PRODUCT_OFF);
				sdelay(VH_PRODUCT_RESET_TIME);
				if (loops_counter >= loops_up_limit) {
					led_flash(LED_YELLOW);
					led_off(LED_GREEN);
					vhd_status = VHD_STATUS_TOPLIMIT;
				} else {
					PUT_PRODUCT_CTRL(PRODUCT_ON);
					vhd_overflow_timer = time_get(VH_OVERFLOW_STAGE_ONE);
					vhd_status = VHD_STATUS_START;
				}
			}
			if (start_flag == STATUS_OFF) {
				led_off(LED_GREEN);
				PUT_PRODUCT_CTRL(PRODUCT_OFF);
				vhd_status = VHD_STATUS_IDLE;
			}
			break;
		case VHD_STATUS_START:
			if(time_left(vhd_overflow_timer) < 0) {
				PUT_PRODUCT_CTRL(PRODUCT_OFF);
				sdelay(VH_PRODUCT_RESET_TIME);
				vhd_overflow_timer = time_get(VH_OVERFLOW_STAGE_TWO);
				PUT_PRODUCT_CTRL(PRODUCT_ON);
				vhd_status = VHD_STATUS_CHECK;
			}
			if (start_flag == STATUS_OFF) {
				led_off(LED_GREEN);
				PUT_PRODUCT_CTRL(PRODUCT_OFF);
				vhd_status = VHD_STATUS_IDLE;
			}
			break;
		case VHD_STATUS_CHECK:
			if (GET_PRODUCT_FB() == 0) {
				loops_counter ++;
				nvm_save();
				PUT_PRODUCT_CTRL(PRODUCT_OFF);
				sdelay(VH_PRODUCT_RESET_TIME);
				if (loops_counter >= loops_up_limit) {
					led_flash(LED_YELLOW);
					led_off(LED_GREEN);
					vhd_status = VHD_STATUS_TOPLIMIT;
				} else {
					PUT_PRODUCT_CTRL(PRODUCT_ON);
					vhd_overflow_timer = time_get(VH_OVERFLOW_STAGE_ONE);
					vhd_status = VHD_STATUS_START;
				}
			}
			if (time_left(vhd_overflow_timer) <0) {
				//for alarm
				PUT_PRODUCT_CTRL(PRODUCT_OFF);
				PUT_ALARM_BUZZER(STATUS_ON);
				led_on(LED_RED);
				led_off(LED_GREEN);
				vhd_status = VHD_STATUS_ALARM;
			}
			if (start_flag == STATUS_OFF) {
				led_off(LED_GREEN);
				PUT_PRODUCT_CTRL(PRODUCT_OFF);
				vhd_status = VHD_STATUS_IDLE;
			}
			break;
		case VHD_STATUS_ALARM:
			if (start_flag == STATUS_OFF) {
				PUT_ALARM_BUZZER(STATUS_OFF);
				led_off(LED_RED);
				vhd_status = VHD_STATUS_IDLE;
			}
			break;
		case VHD_STATUS_TOPLIMIT:
			if (start_flag == STATUS_OFF) {
				led_off(LED_YELLOW);
				vhd_status = VHD_STATUS_IDLE;
			}
			break;
		default:
			break;
	}
	if (time_left(vhd_loops_timer) < 0) {
		vhd_loops_timer = time_get(VH_LOOPS_OVERFLOW);
		osd_SelectGroup(&grps[1]);
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