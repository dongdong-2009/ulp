/*
*
* miaofng@2018-01-31 routine for ford online hub tester
* 1, fdsig includes: mux, audio and dmm <optional, in case of external dmm not used>
* 2, mux is like "hotkey" of cmd cmd gpio
*
* miaofng@2018-04-10 update to support fdsig v1.1
*
*/
#include "ulp/sys.h"
#include "led.h"
#include <string.h>
#include <ctype.h>
#include "shell/shell.h"
#include "common/fifo.h"
#include "stm32f10x.h"
#include "gpio.h"
#include "pwm.h"
#include "tm770x.h"
#include "nvm.h"
#include "fdsig.h"

int sys_ecode = 0;
const char *sys_emsg = "";

void __sys_init(void)
{
	//GPIO_BIND(GPIO_OD1, PA08, HUBW_RST)
	//GPIO_BIND(GPIO_OD1, PA11, HUBL_RST)
#if CONFIG_FDPLC_V1P0 == 1
	GPIO_BIND(GPIO_PP0, PA00, DIAG_VEH)
#endif
	GPIO_BIND(GPIO_PP0, PA01, DIAG_DET)
	GPIO_BIND(GPIO_PP0, PA02, DIAG_LOP)
	GPIO_BIND(GPIO_PP0, PA03, DIAG_SLR)

	GPIO_BIND(GPIO_PP1, PA08, VPM_EN)
	GPIO_BIND(GPIO_PP0, PA11, USB1_S1)
	GPIO_BIND(GPIO_PP0, PA12, USB1_S0)

	//PB
	GPIO_BIND(GPIO_OD1, PB05, HUBH_RST#)
#if CONFIG_FDPLC_V1P0 == 1
	GPIO_BIND(GPIO_PP0, PB12, DG409_S0)
	GPIO_BIND(GPIO_PP0, PB13, DG409_S1)
#endif
	GPIO_BIND(GPIO_PP0, PB14, DG408_S4)
	GPIO_BIND(GPIO_PP0, PB15, DG408_S3)

	//PC
	GPIO_BIND(GPIO_PP0, PC06, USB3_S1)
	GPIO_BIND(GPIO_PP0, PC07, USB3_S0)
	GPIO_BIND(GPIO_PP0, PC08, USB2_S1)
	GPIO_BIND(GPIO_PP0, PC09, USB2_S0)
	GPIO_BIND(GPIO_PP1, PC12, USBx_EN#)
	GPIO_BIND(GPIO_PP0, PC13, DIAG_S2)
	GPIO_BIND(GPIO_PP0, PC14, DIAG_S1)
	GPIO_BIND(GPIO_PP0, PC15, DIAG_S0)

	//PD
	GPIO_BIND(GPIO_PP0, PD00, USB2C_CDP)
	GPIO_BIND(GPIO_PP1, PD01, USB2C_EN#)
	GPIO_BIND(GPIO_PP0, PD08, DG408_S2)
	GPIO_BIND(GPIO_PP0, PD09, DG408_S1)
	GPIO_BIND(GPIO_PP0, PD10, DG408_S0)
	GPIO_BIND(GPIO_PP0, PD11, VCDP_S2)
	GPIO_BIND(GPIO_PP0, PD12, VCDP_S1)
	GPIO_BIND(GPIO_PP0, PD13, VCDP_S0)
	GPIO_BIND(GPIO_PP0, PD14, USB3C_CDP)
	GPIO_BIND(GPIO_PP0, PD15, USB3C_EN#)

	//PE
	GPIO_BIND(GPIO_PP0, PE01, VUSB0_EN)
	GPIO_BIND(GPIO_OD1, PE02, HUBL_RST#)
	GPIO_BIND(GPIO_PP0, PE03, HOST_SEL)
	GPIO_BIND(GPIO_PP1, PE04, USB0_EN#)
	GPIO_BIND(GPIO_OD1, PE05, HUBW_RST#)

	GPIO_BIND(GPIO_PP0, PE08, EVBATN)
	GPIO_BIND(GPIO_PP0, PE09, EVBATP)
	GPIO_BIND(GPIO_PP0, PE10, EVUSB0)
	GPIO_BIND(GPIO_PP0, PE11, ELOAD1)
	GPIO_BIND(GPIO_PP0, PE12, ELOAD2)
	GPIO_BIND(GPIO_PP0, PE13, ELOAD3)
	GPIO_BIND(GPIO_PP0, PE14, ELOAD2C)
	GPIO_BIND(GPIO_PP0, PE15, ELOAD3C)

	//LED(on board)
	GPIO_BIND(GPIO_PP0, PC00, LED_R)
	GPIO_BIND(GPIO_PP0, PC01, LED_G)
}

void sig_init(void)
{
	dmm_init();
	time_t now = time_get(0);
	printf("system booted in %.3f S\n", (float) now / 1000.0);
	printf("sys_ecode = %08x\n", sys_ecode);
}

void sig_update(void)
{
}

void main()
{
	sys_init();
	shell_mute(NULL);
	printf("fdsig v1.x, build: %s %s\n\r", __DATE__, __TIME__);
	sig_init();
	while(1){
		sys_update();
		sig_update();
	}
}

int cmd_xxx_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"*IDN?		to read identification string\n"
		"*RST		instrument reset\n"
		"*UUID?		query the stm32 uuid\n"
	};

	if(!strcmp(argv[0], "*?")) {
		printf("%s", usage);
		return 0;
	}
	else if(!strcmp(argv[0], "*IDN?")) {
		printf("<+0, fdsig v1.x, build: %s %s\n\r", __DATE__, __TIME__);
		return 0;
	}
	else if(!strcmp(argv[0], "*RST")) {
		printf("<+0, No Error\n\r");
		mdelay(50);
		NVIC_SystemReset();
	}
	else if(!strcmp(argv[0], "*UUID?")) {
		unsigned uuid = *(unsigned *)(0X1FFFF7E8);
		printf("<%+d\n", uuid);
		return 0;
	}
	else if(!strcmp(argv[0], "ERR?")) {
		printf("<%+d, %s\n", sys_ecode, sys_emsg);
		return 0;
	}
	else {
		printf("<-1, Unknown Command\n\r");
		return 0;
	}
	return 0;
}

typedef struct {
	const char *name;
	struct {
		unsigned char EVBATN: 1;
		unsigned char EVBATP: 1;
		unsigned char EVUSB0: 1;
		unsigned char ELOAD1: 1;
		unsigned char ELOAD2: 1;
		unsigned char ELOAD3: 1;
		unsigned char ELOAD2C: 1;
		unsigned char ELOAD3C: 1;
	} rly;
	struct {
		unsigned char upx_ven : 1; //high effective
		unsigned char upx_en : 1; //high effective
		unsigned char usbx_en : 1; //high effective
		unsigned char ds2c_en : 1; //high effective
		unsigned char ds3c_en : 1; //high effective
		unsigned char usb1 : 2; //mux sel @ds#1
		unsigned char usb2 : 2; //mux sel @ds#2
		unsigned char usb3 : 2; //mux sel @ds#3
		unsigned char ds2c : 1; //mux sel @ds#c2-
		unsigned char ds3c : 1; //mux sel @ds#c3-
	} mux;
} mode_t;

#define RLY_OFF  {.EVBATN=0, .EVBATP=0, .EVUSB0=0, .ELOAD1=0, .ELOAD2=0, .ELOAD3=0, .ELOAD2C=0, .ELOAD3C=0}
#define RLY_BYP  {.EVBATN=0, .EVBATP=1, .EVUSB0=0, .ELOAD1=0, .ELOAD2=0, .ELOAD3=0, .ELOAD2C=0, .ELOAD3C=0}
#define RLY_PMK  {.EVBATN=1, .EVBATP=1, .EVUSB0=0, .ELOAD1=1, .ELOAD2=1, .ELOAD3=1, .ELOAD2C=0, .ELOAD3C=0}
#define RLY_PMKC {.EVBATN=1, .EVBATP=1, .EVUSB0=0, .ELOAD1=0, .ELOAD2=0, .ELOAD3=0, .ELOAD2C=1, .ELOAD3C=1}

#define MUX_OFF  {.upx_ven=0, .upx_en=0, .usbx_en=0, .usb1=0, .usb2=0, .usb3=0, .ds2c_en=0, .ds2c=0, .ds3c_en=0, .ds3c=0}
#define MUX_IDL  {.upx_ven=1, .upx_en=1, .usbx_en=0, .usb1=0, .usb2=0, .usb3=0, .ds2c_en=0, .ds2c=0, .ds3c_en=0, .ds3c=0}
#define MUX_PMK  {.upx_ven=1, .upx_en=1, .usbx_en=1, .usb1=0, .usb2=0, .usb3=0, .ds2c_en=0, .ds2c=0, .ds3c_en=0, .ds3c=0}
#define MUX_PMK1 {.upx_ven=1, .upx_en=1, .usbx_en=1, .usb1=0, .usb2=3, .usb3=3, .ds2c_en=0, .ds2c=0, .ds3c_en=0, .ds3c=0}
#define MUX_PMK2 {.upx_ven=1, .upx_en=1, .usbx_en=1, .usb1=3, .usb2=0, .usb3=3, .ds2c_en=0, .ds2c=0, .ds3c_en=0, .ds3c=0}
#define MUX_PMK3 {.upx_ven=1, .upx_en=1, .usbx_en=1, .usb1=3, .usb2=3, .usb3=0, .ds2c_en=0, .ds2c=0, .ds3c_en=0, .ds3c=0}
#define MUX_CDP  {.upx_ven=1, .upx_en=1, .usbx_en=1, .usb1=3, .usb2=3, .usb3=3, .ds2c_en=1, .ds2c=1, .ds3c_en=1, .ds3c=1}
#define MUX_H21  {.upx_ven=1, .upx_en=1, .usbx_en=1, .usb1=2, .usb2=2, .usb3=0, .ds2c_en=0, .ds2c=0, .ds3c_en=0, .ds3c=0}
#define MUX_H12  {.upx_ven=1, .upx_en=1, .usbx_en=1, .usb1=1, .usb2=1, .usb3=0, .ds2c_en=0, .ds2c=0, .ds3c_en=0, .ds3c=0}
#define MUX_H13  {.upx_ven=1, .upx_en=1, .usbx_en=1, .usb1=1, .usb2=0, .usb3=1, .ds2c_en=0, .ds2c=0, .ds3c_en=0, .ds3c=0}
#define MUX_H12C {.upx_ven=1, .upx_en=1, .usbx_en=1, .usb1=1, .usb2=0, .usb3=0, .ds2c_en=1, .ds2c=0, .ds3c_en=0, .ds3c=0}
#define MUX_H13C {.upx_ven=1, .upx_en=1, .usbx_en=1, .usb1=1, .usb2=0, .usb3=0, .ds2c_en=0, .ds2c=0, .ds3c_en=1, .ds3c=0}

const mode_t mode_list[] = {
	{.name = "off" , .rly = RLY_OFF , .mux = MUX_OFF },
	{.name = "rev" , .rly = RLY_BYP , .mux = MUX_OFF },
	{.name = "iqz" , .rly = RLY_BYP , .mux = MUX_OFF },
	{.name = "idl" , .rly = RLY_BYP , .mux = MUX_IDL },
	{.name = "pmk" , .rly = RLY_PMK , .mux = MUX_PMK },
	{.name = "pmk1", .rly = RLY_PMK , .mux = MUX_PMK1 },
	{.name = "pmk2", .rly = RLY_PMK , .mux = MUX_PMK2 },
	{.name = "pmk3", .rly = RLY_PMK , .mux = MUX_PMK3 },
	{.name = "pmkc", .rly = RLY_PMKC, .mux = MUX_PMK },
	{.name = "cdp" , .rly = RLY_PMK , .mux = MUX_CDP },
	{.name = "h21" , .rly = RLY_PMK , .mux = MUX_H21 },
	{.name = "h12" , .rly = RLY_PMK , .mux = MUX_H12 },
	{.name = "h13" , .rly = RLY_PMK , .mux = MUX_H13 },
	{.name = "h12c", .rly = RLY_PMK , .mux = MUX_H12C},
	{.name = "h13c", .rly = RLY_PMK , .mux = MUX_H13C},
};

const mode_t *mode_search(const char *name)
{
	const mode_t *mode = NULL;
	int n = sizeof(mode_list) / sizeof(mode_t);
	for(int i = 0; i < n; i ++) {
		if(!strcmp(name, mode_list[i].name)) {
			mode = &mode_list[i];
		}
	}
	return mode;
}

static int cmd_mode_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"mode off		all disconnect\n"
		"mode rev		reverse voltage applied\n"
		"mode iqz		quiescent current mode, only vbat applied\n"
		"mode idl		idle current mode, only vbat and upstream usb connected\n"
		"mode pmk		all downstream = passmark mode\n"
		"mode pmkc		the same as pmk mode, except eload = type-c\n"
		"mode cdp		all downstream = cdp mode\n"
		"mode h12		host-to-host mode, 1->2\n"
		"mode h12c		host-to-host mode, 1->c2-\n"
		"mode h13		host-to-host mode, 1->3\n"
		"mode h13c		host-to-host mode, 1->c3-\n"
		"mode h21		host-to-host mode, 2->1\n"
	};

	if(argc >= 2) {
		const mode_t *mode = mode_search(argv[1]);
		if(mode != NULL) {
			//handle relay settings
			gpio_set("EVBATN", mode->rly.EVBATN);
			gpio_set("EVBATP", mode->rly.EVBATP);
			//gpio_set("EVUSB0", mode->rly.EVUSB0);
			gpio_set("ELOAD1", mode->rly.ELOAD1);
			gpio_set("ELOAD2", mode->rly.ELOAD2);
			gpio_set("ELOAD3", mode->rly.ELOAD3);
			gpio_set("ELOAD2C", mode->rly.ELOAD2C);
			gpio_set("ELOAD3C", mode->rly.ELOAD3C);

			//handle mux settings
			gpio_set("VUSB0_EN", mode->mux.upx_ven);
			gpio_set("USB0_EN#", !mode->mux.upx_en);
			gpio_set("USBx_EN#", !mode->mux.usbx_en);
			gpio_set("USB2C_EN#", !mode->mux.ds2c_en);
			gpio_set("USB3C_EN#", !mode->mux.ds3c_en);
			gpio_set("USB1_S1", mode->mux.usb1 & 0x02);
			gpio_set("USB1_S0", mode->mux.usb1 & 0x01);
			gpio_set("USB2_S1", mode->mux.usb2 & 0x02);
			gpio_set("USB2_S0", mode->mux.usb2 & 0x01);
			gpio_set("USB3_S1", mode->mux.usb3 & 0x02);
			gpio_set("USB3_S0", mode->mux.usb3 & 0x01);
			gpio_set("USB2C_CDP", mode->mux.ds2c & 0x01);
			gpio_set("USB3C_CDP", mode->mux.ds3c & 0x01);

			printf("<+0, OK!\n");
			return 0;
		}
	}

	printf("<-1, operation no effect!\n");
	printf("%s", usage);
	return 0;
}

cmd_t cmd_mode = {"mode", cmd_mode_func, "mode i/f cmds"};
DECLARE_SHELL_CMD(cmd_mode)

static int cmd_host_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"host win		host = win, pdi mode\n"
		"host linux		host = lnx, fct mode\n"
	};

	if(argc >= 2) {
		int win = !strcmp(argv[1], "win");
		int lnx = !strcmp(argv[1], "linux");
		if(win | lnx) {
			gpio_set("HOST_SEL", lnx);
			printf("<+0, OK!\n");
			return 0;
		}
	}

	printf("<-1, operation no effect!\n");
	printf("%s", usage);
	return 0;
}

cmd_t cmd_host = {"host", cmd_host_func, "host i/f cmds"};
DECLARE_SHELL_CMD(cmd_host)

static int cmd_aux_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"aux test L|R		enter into playback mode, mic = L|R\n"
		"aux loop L|R		enter into loopback mode, mic = L|R\n"
		"aux diag [det=gnd]		enter into diagnose mode\n"
	};

	int test = 0;
	int loop = 0;
	int diag = 0;
	int gdet = 0; //aux_det = gnd
	int micr = 0; //mic_in = R
	if(argc >= 2) {
		test = !strcmp(argv[1], "test");
		loop = !strcmp(argv[1], "loop");
		diag = !strcmp(argv[1], "diag");
		if(argc >= 3) {
			if(diag) gdet = !strcmp(argv[2], "det=gnd");
			if(test|loop) micr = !strcmp(argv[2], "R");
		}
	}

	if(test|loop|diag) {
		gpio_set("DIAG_LOP", loop);
		gpio_set("DIAG_SLR", micr);
		gpio_set("DIAG_DET", gdet);
#if CONFIG_FDPLC_V1P0 == 1
		gpio_set("DIAG_VEH", diag);
#endif
		printf("<+0, OK!\n");
		return 0;
	}

	printf("<-1, operation no effect!\n");
	printf("%s", usage);
	return 0;
}

cmd_t cmd_aux = {"aux", cmd_aux_func, "aux i/f cmds"};
DECLARE_SHELL_CMD(cmd_aux)
