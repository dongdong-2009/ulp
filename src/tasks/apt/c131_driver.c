/*
 * David@2011 init version
 */
#include <stdio.h>
#include <string.h>
#include "linux/list.h"
#include "sys/sys.h"
#include "spi.h"
#include "config.h"
#include "time.h"
#include "stm32f10x.h"
#include "mbi5025.h"
#include "can.h"
#include "c131_driver.h"

//Local Static RAM define
static unsigned char LoadRAM[8];
static unsigned short * pLoadRAM;
static unsigned char SRImage[6];

struct can_queue_s {
	int ms;
	time_t timer;
	can_msg_t msg;
	struct list_head list;
};

//EMS_3 MSG
static struct can_queue_s c131_ems_3 = {
	.ms = 10,
	.msg = {0x11c, 4, {0x00, 0x00, 0x00, 0x00}, 0},
};

static struct can_queue_s c131_ems_4 = {
	.ms = 10,
	.msg = {0x122, 8, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
};

static struct can_queue_s c131_abs_1 = {
	.ms = 10,
	.msg = {0xb4, 8, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
};

static struct can_queue_s c131_abs_2 = {
	.ms = 10,
	.msg = {0xb6, 8, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
};

static struct can_queue_s c131_abs_3 = {
	.ms = 20,
	.msg = {0x12c, 4, {0x00, 0x00, 0x00, 0x00}, 0},
};

static struct can_queue_s c131_sas_1 = {
	.ms = 10,
	.msg = {0x96, 8, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
};

static struct can_queue_s c131_tcu_3 = {
	.ms = 100,
	.msg = {0x248, 8, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, 0},
};

static const can_msg_t req_dtc_msg = {
	.id = C131_DIAG_REQ_ID,
	.dlc = 8,
	.data = {0x04, 0x18, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00},
};

static const can_msg_t req_clrdtc_msg = {
	.id = C131_DIAG_REQ_ID,
	.dlc = 8,
	.data = {0x01, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
};

static const can_msg_t req_start_msg = {
	.id = C131_DIAG_REQ_ID,
	.dlc = 8,
	.data = {0x02, 0x10, 0x81, 0x00, 0x00, 0x00, 0x00, 0x00},
};

static const can_msg_t req_flow_msg = {
	.id = C131_DIAG_REQ_ID,
	.dlc = 8,
	.data = {0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
};

static LIST_HEAD(can_queue);
static const can_bus_t *can_bus;
static void c131_can_StartDiagnosis(void);

//Local device define
static mbi5025_t sr = {
	.bus = &spi1,
	.load_pin = SPI_CS_PC3,
	.oe_pin = SPI_CS_PC4,
};

void c131_driver_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	can_cfg_t cfg = CAN_CFG_DEF;

	//PD Port Init
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOD,  &GPIO_InitStructure);

	//PC Port Init
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_2 | GPIO_Pin_5 | GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	//for sdm detect input pin
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	//power set
	Disable_SDMPWR();
	Disable_EXTPWR();
	Disable_LEDPWR();

	//spi device init
	mbi5025_Init(&sr);
	mbi5025_DisableLoad(&sr);
	mbi5025_EnableOE(&sr);

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

	//ram pointer init
	pLoadRAM = (unsigned short *)LoadRAM;
}

int c131_SetRelayRAM(unsigned char * pload)
{
	memcpy(LoadRAM, pload, 8);
	return 0;
}

int c131_relay_Update(void)
{
	memset(SRImage, 0x00, sizeof(SRImage));

	SRImage[1] = LoadRAM[2];
	SRImage[2] = LoadRAM[4];
	SRImage[3] = LoadRAM[6];
	SRImage[4] = LoadRAM[1];
	SRImage[5] = LoadRAM[0];

	mbi5025_WriteBytes(&sr, SRImage, sizeof(SRImage));

	return 0;
}

int loop_SetRelayStatus(unsigned short loop_relays, int act)
{
	if (act == RELAY_OFF) {
		pLoadRAM[0] &= ~loop_relays;
	} else {
		pLoadRAM[0] |= loop_relays;
	}
	return 0;
}

int loop_GetRelayStatus(unsigned short * ploop_status)
{
	(* ploop_status) = pLoadRAM[0];
	return 0;
}

int ess_SetRelayStatus(unsigned short ess_relays, int act)
{
	if (act == RELAY_OFF) {
		pLoadRAM[1] &= ~ess_relays;
	} else {
		pLoadRAM[1] |= ess_relays;
	}
	return 0;
}

int ess_GetRelayStatus(unsigned short * pess_status)
{
	(* pess_status) = pLoadRAM[1];
	return 0;
}

int led_SetRelayStatus(unsigned short led_relays, int act)
{
	if (act == RELAY_OFF) {
		pLoadRAM[2] &= ~led_relays;
	} else {
		pLoadRAM[2] |= led_relays;
	}
	return 0;
}

int led_GetRelayStatus(unsigned short * pled_status)
{
	(* pled_status) = pLoadRAM[2];
	return 0;
}

int sw_SetRelayStatus(unsigned short sw_relays, int act)
{
	if (act == RELAY_OFF) {
		pLoadRAM[3] &= ~sw_relays;
	} else {
		pLoadRAM[3] |= sw_relays;
	}
	return 0;
}

int sw_GetRelayStatus(unsigned short * psw_status)
{
	(* psw_status) = pLoadRAM[3];
	return 0;
}

//can related function
void c131_can_SendOtherECUMsg(void)
{
	struct list_head *pos;
	struct can_queue_s *q;

	list_for_each(pos, &can_queue) {
		q = list_entry(pos, can_queue_s, list);
		if(q -> timer == 0 || time_left(q -> timer) < 0) {
			q -> timer = time_get(q -> ms);
			can_bus -> send(&q -> msg);
		}
	}
}

int c131_can_GetAirbagMsg(can_msg_t *pmsg)
{
	if (!can_bus -> recv(pmsg)) {
		if ((pmsg->id == AIRBAG_CAN_ID) && (pmsg->dlc == 8))
			return 0;
	}

	return 1;
}

int c131_can_ClearHistoryDTC(void)
{
	return can_bus -> send(&req_clrdtc_msg);
}

static void c131_can_StartDiagnosis(void)
{
	while(!can_bus -> send(&req_start_msg));

	return;
}

int c131_can_GetDTC(void)
{
	can_msg_t msg;
	char *p;

	//start session
	c131_can_StartDiagnosis();

	//send dtc request
	while(!can_bus -> send(&req_dtc_msg));

	//receive dtc
	while (!can_bus -> recv(&msg)) {
		if (msg.data[2] == 0x58)
			return 0;
	}

	return 0;
}
