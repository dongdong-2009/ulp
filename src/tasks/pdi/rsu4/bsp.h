#ifndef __PDI_BSP_H_
#define __PDI_BSP_H_

extern const can_bus_t *bsp_can_bus;
#define bsp_host_bus uart3

enum {
	//U6
	K_B0, K_B1, K_B2, K_B3, //00-03
	K_C0, K_C1, K_C2, K_C3, //04-07
	K_D4, K_C4, K_B4, K_A4, //08-11
	K_D3, K_D2, K_D1, K_D0, //12-15
	//U7
	K_A0, K_A1, K_A2, K_A3, //16-19
	LED_RG0, LED_RG1, LED_RG2, LED_RG3, //20-23
	LED_LG3, LED_LG2, LED_LG1, LED_LG0, //24-27
	LED_LR3, LED_LR2, LED_LR1, LED_LR0, //28-31
	//U8
	NC0, NC1, NC2, NC3, //32-35
	LED_RY0, LED_RY1, LED_RY2, LED_RY3, //36-39
	LED_RR3, LED_RR2, LED_RR1, LED_RR0, //40-43
	LED_LY0, LED_LY1, LED_LY2, LED_LY3, //44-47
};

//void mxc_write_image(void);
//void mxc_switch(int signal, int on);

#define LED_ALL		0x00FFFFFF
#define LED_LA		0x000F0F0F //ALL LEFT
#define LED_RA		0x00F0F0F0 //ALL RIGHT
#define LED_LY		0x000F0000 //ALL LEFT YELLOW
#define LED_RY		0x00F00000 //ALL RIGHT YELLOW
void bsp_led(int mask, int status);
void bsp_select_grp(int grp); //ecu grp = 0..4

//o ctrl
void bsp_swbat(int yes);
void bsp_beep(int yes);
void bsp_select_rsu(int pos);
void bsp_select_can(int ecu);

//i sense
int bsp_rdy_status(int pos);
int bsp_rsu_status(int pos, int *mV);
int bsp_vbat(void);

void bsp_init(void);
void bsp_reset(void);

#endif
