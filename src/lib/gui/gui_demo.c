/*
 * 	miaofng@2012 initial version
 */

#include "ulp/sys.h"
#include "gui/gui.h"
#include "instr/instr.h"
#include "instr/matrix.h"
#include "instr/dmm.h"
#include <string.h>

static gwidget *main_window;
static gwidget *fixed;
static gwidget *button_lines;
static gwidget *button_ground;
static gwidget *button_mode_id;
static gwidget *button_mode_dg;
static gwidget *button_start;
static gwidget *button_keycode;
static gwidget *button_typecode;
static gwidget *button_ecode0;
static gwidget *button_ecode1;
static gwidget *button_ecode2;

static unsigned cmap_ecode[] = GUI_COLORMAP_NEW(RED, WHITE);
static unsigned cmap_tcode[] = GUI_COLORMAP_NEW(GREEN, WHITE);
static unsigned cmap_kcode[] = GUI_COLORMAP_NEW(GRAY, WHITE);

static struct {
	unsigned start : 1;
	unsigned lines : 3;
	unsigned seconds : 9;
	unsigned mode : 8;
	const char *ground;
} oid_config;

enum {
	READY,
	BUSY
};

static struct {
	unsigned stm : 4;
	unsigned counter : 9;
} oid_status;

/*matrix bus connection*/
enum {
	BUS_CSP, /*BUS 0*/
	BUS_VMP,
	BUS_VMN,
	BUS_CSN,
};

/*matrix row connection*/
enum {
	PIN_SHELL, /*default to pin 0*/
	PIN_OSIGN, /*default to pin 1, gray line*/
	PIN_OSIGP, /*default to pin 2, black line*/
	PIN_HEAT1, /*default to pin 3, white line*/
	PIN_HEAT2, /*default to pin 4, white line*/

	PIN_RCAL1, /*calibration resistor*/
	PIN_RCAL2,
	PIN_VCAL1, /*calibration voltage source*/
	PIN_VCAL2,

	NR_OF_PINS,
};

static struct {
	int R[5][5];
	int mv;
} oid_result;

static inline void oid_result_init(void)
{
	memset(&oid_result, 0xff, sizeof(oid_result));
}

static int main_identify_on_event(gwidget *widget, gevent *event)
{
	static char str[5];
	if(event->type == GUI_TOUCH_BEGIN) {
		printf("x = %d, y = %d\n", event->ts.x, event->ts.y);
		if(gui_widget_touched(widget, event)) {
			if(widget == button_lines) {
				if(oid_status.stm == READY) {
					oid_config.lines = (oid_config.lines < 4) ? (oid_config.lines + 1) : 1;
					sprintf(str, "%d", oid_config.lines);
					gui_widget_set_text(widget, str);
				}
			}
			else if(widget == button_ground) {
				if(oid_status.stm == READY) {
					switch(oid_config.ground[0]) {
					case '?':
						oid_config.ground = "N";
						break;
					case 'N':
						oid_config.ground = "Y";
						break;
					default:
						oid_config.ground = "?";
						break;
					}
					gui_widget_set_text(widget, oid_config.ground);
				}
			}
			else if(widget == button_mode_dg) {
				if(oid_status.stm == READY) {
					oid_config.mode = 'd';
					gui_widget_set_text(button_mode_id, " ");
					gui_widget_set_text(button_mode_dg, "<");
				}
				else {
					if(oid_config.mode == 'd') {
						oid_status.counter += 30;
					}
				}
			}
			else if(widget == button_mode_id) {
				if(oid_status.stm == READY) {
					oid_config.mode = 'i';
					gui_widget_set_text(button_mode_id, "<");
					gui_widget_set_text(button_mode_dg, " ");
				}
				else {
					if(oid_config.mode == 'i') {
						oid_status.counter += 30;
					}
				}
			}
			else if(widget == button_start) {
				oid_config.start = 1;
			}
			else {
			/*
				i = (i < 19) ? (i + 1) : 0;
				sprintf(str, "%d", i);
				gui_widget_set_text(widget, str);
			*/
			}
		}
	}
	return -1;
}

void main_window_init(void)
{
	extern const unsigned char image_main_window[];
	main_window = gui_window_new(GUI_WINDOW_TOPLEVEL);
	gui_widget_set_image(main_window, image_main_window);
	fixed = gui_fixed_new();
	gui_widget_add(main_window, fixed);

	button_lines = gui_button_new_with_label("4");
	button_ground = gui_button_new_with_label("?");
	button_mode_id = gui_button_new_with_label("<");
	button_mode_dg = gui_button_new_with_label(" ");
	button_start = gui_button_new();
	button_keycode = gui_button_new_with_label("--");
	button_typecode = gui_button_new_with_label("--");
	button_ecode0 = gui_button_new_with_label("--");
	button_ecode1 = gui_button_new_with_label("--");
	button_ecode2 = gui_button_new_with_label("--");

	gui_widget_set_colormap(button_keycode, cmap_kcode);
	gui_widget_set_colormap(button_typecode, cmap_tcode);
	gui_widget_set_colormap(button_ecode0, cmap_ecode);
	gui_widget_set_colormap(button_ecode1, cmap_ecode);
	gui_widget_set_colormap(button_ecode2, cmap_ecode);

	gui_widget_set_size_request(button_lines, 60, 40-4);
	gui_widget_set_size_request(button_ground, 60, 40-4);
	gui_widget_set_size_request(button_mode_id, 60, 40-4);
	gui_widget_set_size_request(button_mode_dg, 60, 40-4);
	gui_widget_set_size_request(button_start, 120, 40-4);
	gui_widget_set_size_request(button_keycode, 116, 40-4);
	gui_widget_set_size_request(button_typecode, 116, 40-4);
	gui_widget_set_size_request(button_ecode0, 116, 40-4);
	gui_widget_set_size_request(button_ecode1, 116, 40-4);
	gui_widget_set_size_request(button_ecode2, 116, 40-4);
	gui_fixed_put(fixed, button_lines, 260, 40*1 + 2);
	gui_fixed_put(fixed, button_ground, 260, 40*2 + 2);
	gui_fixed_put(fixed, button_mode_dg, 260, 40*3 + 2);
	gui_fixed_put(fixed, button_mode_id, 260, 40*4 + 2);
	gui_fixed_put(fixed, button_start, 200, 40*5 + 2);
	gui_fixed_put(fixed, button_keycode, 80, 40*1 + 2);
	gui_fixed_put(fixed, button_typecode, 80, 40*2 + 2);
	gui_fixed_put(fixed, button_ecode0, 80, 40*3 + 2);
	gui_fixed_put(fixed, button_ecode1, 80, 40*4 + 2);
	gui_fixed_put(fixed, button_ecode2, 80, 40*5 + 2);
	gui_event_connect(button_lines, main_identify_on_event);
	gui_event_connect(button_ground, main_identify_on_event);
	gui_event_connect(button_mode_id, main_identify_on_event);
	gui_event_connect(button_mode_dg, main_identify_on_event);
	gui_event_connect(button_start, main_identify_on_event);
	gui_event_connect(button_keycode, main_identify_on_event);
	gui_event_connect(button_typecode, main_identify_on_event);
	gui_event_connect(button_ecode0, main_identify_on_event);
	gui_event_connect(button_ecode1, main_identify_on_event);
	gui_event_connect(button_ecode2, main_identify_on_event);
	gui_window_show(main_window);
}

static enum oid_error_type {
	E_OK,
	E_SYS_ERR,
	E_DMM_INIT,
	E_MATRIX_INIT,
	E_RES_CAL,
	E_VOL_CAL,
} oid_ecode[3], oid_ecode_sys;

static inline void oid_error_init(void)
{
	oid_ecode_sys = E_OK;
	oid_ecode[0] = E_OK;
	oid_ecode[1] = E_OK;
	oid_ecode[2] = E_OK;
}

static void oid_error(enum oid_error_type ecode)
{
	char str[8];
	gwidget *widget;
	if(ecode > E_SYS_ERR) {
		widget = button_ecode0;
		oid_ecode_sys = ecode;
	}

	switch(ecode) {
	case E_DMM_INIT:
		snprintf(str, 7, "DMMI%02d", ecode);
		gui_widget_set_text(widget, str);
		break;

	case E_MATRIX_INIT:
		snprintf(str, 7, "MATI%02d", ecode);
		gui_widget_set_text(widget, str);
		break;

	case E_RES_CAL:
		snprintf(str, 7, "RCAL%02d", ecode);
		gui_widget_set_text(widget, str);
		break;

	case E_VOL_CAL:
		snprintf(str, 7, "VCAL%02d", ecode);
		gui_widget_set_text(widget, str);
		break;

	default:
		;
	}
}

static void oid_error_flash(void)
{
	char str[] = "      ";
	gwidget *widget = button_ecode0;

	for(int i = 0; i < 3; i ++) {
		gui_widget_set_text(widget, str);
		sys_mdelay(200);
		oid_error(oid_ecode_sys);
		sys_mdelay(100);
	}
}

static int oid_measure_resistor(int pin0, int pin1)
{
	int mohm = -1;
	matrix_reset();
	matrix_connect(BUS_CSP, pin0);
	matrix_connect(BUS_VMP, pin0);
	matrix_connect(BUS_VMN, pin1);
	matrix_connect(BUS_CSN, pin1);
	if(!matrix_execute()) {
		sys_mdelay(1000);
		mohm = dmm_read(0);
	}
	return mohm;
}

void oid_instr_cal(void)
{
	struct dmm_s *oid_dmm = NULL;
	struct matrix_s *oid_matrix = NULL;

	time_t deadline = time_get(3000);
	do {
		sys_update();
		oid_dmm = dmm_open(NULL);
	} while(oid_dmm == NULL && time_left(deadline) > 0);

	do {
		sys_update();
		oid_matrix = matrix_open(NULL);
	} while(oid_matrix == NULL && time_left(deadline) > 0);

	if(oid_dmm == NULL) {
		oid_error(E_DMM_INIT);
		return;
	}

	if(oid_matrix == NULL) {
		oid_error(E_MATRIX_INIT);
		return;
	}

#define RCAL_MIN ((int)(1000 * (1 + 0.1)))
#define RCAL_MAX ((int)(1000 * (1 - 0.1)))
#define VCAL_MIN ((int)(1250 * (1 + 0.1)))
#define VCAL_MAX ((int)(1250 * (1 - 0.1)))

	int mohm = oid_measure_resistor(PIN_RCAL1, PIN_RCAL2);
	if((mohm > RCAL_MAX) || (mohm < RCAL_MIN)) {
		oid_error(E_RES_CAL);
		return;
	}

	/*
	int mv = oid_measure_voltage(PIN_VCAL1, PIN_VCAL2);
	if((mv > VCAL_MAX) || (mv > VCAL_MAX)) {
	}
	*/
}

void oid_cold_test()
{
	int i, j;
	/*ground test, to identify gray line*/
	for(i = 1; i <= oid_config.lines; i ++) {
		oid_result.R[0][i] = oid_measure_resistor(0, i);
	}

	/*heat wire test*/
	for(j = 1; j <= oid_config.lines; j ++) {
		for(i = j + 1; i <= oid_config.lines; i ++) {
			oid_result.R[j][i] = oid_measure_resistor(j, i);
		}
	}
}

void oid_hot_test()
{
}

void oid_init(void)
{
	oid_config.lines = 4;
	oid_config.ground = "?";
	oid_config.mode = 'i';
	oid_config.seconds = 60;
	oid_config.start = 0;
	oid_status.stm = READY;
	oid_status.counter = 0;
	oid_error_init();
}

void oid_update(void)
{
	static time_t oid_second_timer = 0;
	gwidget *widget;
	char str[4];

	if(oid_status.counter > 0) {
		if(time_left(oid_second_timer) < 0) {
			oid_second_timer = time_get(1000);
			oid_status.counter --;
			widget = (oid_config.mode == 'i') ? button_mode_id : button_mode_dg;
			sprintf(str, "%d", oid_status.counter);
			gui_widget_set_text(widget, str);
		}
	}
}

void __sys_update(void)
{
	instr_update();
	oid_update();
	gui_update();
}

void oid_start(void)
{
	oid_config.start = 0;
	oid_status.stm = BUSY;
	oid_status.counter = oid_config.seconds;
	oid_result_init();
	oid_cold_test();
	while(oid_status.counter > 0) {
		sys_update();
		if(oid_config.start == 1) {
			//cancel ...
			oid_config.start = 0;
			break;
		}
	}
	oid_hot_test();
	oid_status.stm = READY;
	oid_status.counter = 0;
	gwidget *widget = (oid_config.mode == 'i') ? button_mode_id : button_mode_dg;
	gui_widget_set_text(widget, "<");
}

void main(void)
{
	sys_init();
	instr_init();
	oid_init();
	gui_init(NULL);
	main_window_init();
	gui_update();
	oid_instr_cal();
	while(1) {
		sys_update();
		if(oid_config.start == 1) {
			if(oid_ecode_sys) {
				oid_error_flash();
				oid_config.start = 0;
			}
			else {
				oid_start();
			}
		}
	}
}
