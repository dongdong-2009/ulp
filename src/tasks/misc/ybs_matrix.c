/*
 * 	miaofng@2012 initial version
 *
 */

#include "ulp/sys.h"
#include "instr/instr.h"
#include "instr/matrix.h"
#include "common/bitops.h"
#include "crc.h"
#include "led.h"
#include "priv/mcamos.h"
#include "spi.h"
#include "stm32f10x.h"

static const char instr_name[] = "YBS MATRIX v1.0";
static unsigned char instr_mode;
static unsigned char instr_ecode;
static const can_bus_t *instr_can = &can1;
static mcamos_srv_t instr_server;
static time_t instr_timer;

static union instr_uuid matrix_uuid;
static unsigned char matrix_bus = 8;
static unsigned char matrix_row = 8;
static unsigned char matrix_image[8];
#define matrix_image_bytes 8
static const spi_bus_t *matrix_spi = &spi1;
#define pin_le SPI_CS_PB0 /*high level effective*/
#define pin_oe SPI_1_NSS /*low level effective*/

static void __matrix_set_image(void)
{
	/*msb, and byte0 will be sent first, so byte[0].bit0 is S00(IN 0 + BUS 0)
		BIT			MATRIX 16X8	MATRIX 8X8
	byte[0].bit0..3	S140..S143	IN14, BUS 0-3	IN6, BUS 4-7
	byte[0].bit4..7	S150..S153	IN15, BUS 0-3	IN7, BUS 4-7
	byte[1].bit0..3	S120..S123	IN12, BUS 0-3	IN4, BUS 4-7
	byte[1].bit4..7	S130..S133	IN13, BUS 0-3	IN5, BUS 4-7
	byte[2].bit0..3	S100..S103	IN10, BUS 0-3	IN2, BUS 4-7
	byte[2].bit4..7	S110..S113	IN11, BUS 0-3	IN3, BUS 4-7
	byte[3].bit0..3	S80..S83	IN8, BUS 0-3	IN0, BUS 4-7
	byte[3].bit4..7	S90..S93	IN9, BUS 0-3	IN1, BUS 4-7
	byte[4].bit0..3	S60..S63	IN6, BUS 0-3	IN6, BUS 0-3
	byte[4].bit4..7	S70..S73	IN7, BUS 0-3	IN7, BUS 0-3
	byte[5].bit0..3	S40..S43	IN4, BUS 0-3	IN4, BUS 0-3
	byte[5].bit4..7	S50..S53	IN5, BUS 0-3	IN5, BUS 0-3
	byte[6].bit0..3	S20..S23	IN2, BUS 0-3	IN2, BUS 0-3
	byte[6].bit4..7	S30..S33	IN3, BUS 0-3	IN3, BUS 0-3
	byte[7].bit0..3	S00..S03	IN0, BUS 0-3	IN0, BUS 0-3
	byte[7].bit4..7	S10..S13	IN1, BUS 0-3	IN1, BUS 0-3
	*/
	unsigned char row, row_min, row_max, bus, bus_min, bus_max, byte, mask;
	matrix_spi->csel(pin_le, 0);
	for(int i = 0; i < matrix_image_bytes; i ++) {
		byte = 0;
		mask = 0x01;
		row_min = 8 - ((i % 4) << 1) - 2;
		row_max = 8 - ((i % 4) << 1);
		for(row = row_min; row < row_max; row ++) {
			bus_min = (i < 4) ? 4 : 0;
			bus_max = (i < 4) ? 8 : 4;
			for(bus = bus_min; bus < bus_max; bus ++) {
				int index = row * matrix_bus + bus;
				if(bit_get(index, matrix_image)) {
					byte |= mask;
				}
				mask <<= 1;
			}
		}
		matrix_spi->wreg(0, byte);
	}
	matrix_spi->csel(pin_le, 1);
	matrix_spi->csel(pin_le, 0);
}

static void com_init(void)
{
	//can init
	can_cfg_t cfg = CAN_CFG_DEF;
	cfg.baud = INSTR_BAUD;
	instr_can->init(&cfg);

	time_t deadline = time_get(0);
	time_t timer = time_get(0);
	can_msg_t msg;
	struct instr_nreq_s *nreq = (struct instr_nreq_s *) msg.data;
	struct instr_echo_s *echo = (struct instr_echo_s *) msg.data;
	while(1) {
		sys_update();
		if(time_left(deadline) < 0) {
			deadline = time_get(100);
			/*broadcast newbie request*/
			nreq->uuid = matrix_uuid.value;
			nreq->ecode = instr_ecode;
			msg.id = INSTR_ID_BROADCAST;
			msg.dlc = sizeof(struct instr_nreq_s);
			msg.flag = 0;
			instr_can->send(&msg);
		}

		#if 0
		if(time_left(timer) < 0) {
			timer = time_get(5000);
			unsigned char val = matrix_image[0];
			val = (val == 0x00) ? 0xff : 0x00;
			memset(matrix_image, val, matrix_image_bytes);
			__matrix_set_image();
		}
		#endif

		/*recv*/
		if(!instr_can->recv(&msg)) {
			if((msg.id == INSTR_ID_BROADCAST) && (echo->uuid == matrix_uuid.value)) {
				led_inv(LED_GREEN);
				break;
			}
		}
	}

	//init mcamos server
	instr_server.can = &can1;
	instr_server.id_cmd = echo->id_cmd;
	instr_server.id_dat = echo->id_dat;
	instr_server.baud = INSTR_BAUD;
	instr_server.timeout = 8;
	instr_server.inbox_addr = INSTR_INBOX_ADDR;
	instr_server.outbox_addr = INSTR_OUTBOX_ADDR;
	mcamos_srv_init(&instr_server);
	instr_timer = time_get(INSTR_UPDATE_MS + 1000);
	led_off(LED_RED);
}

static void com_update(void)
{
	char ret = 0;
	unsigned char *inbox = instr_server.inbox;
	unsigned char *outbox = instr_server.outbox + 2;
	unsigned short crc16;

	static time_t led_timer;
	mcamos_srv_update(&instr_server);
	if(inbox[0] != 0) {
		led_timer = time_get(100);
	}

	if(time_left(led_timer) > 0) {
		led_on(LED_GREEN);
	}
	else {
		led_off(LED_GREEN);
	}

	switch(inbox[0]) {
	case INSTR_CMD_GET_NAME:
		memset(outbox, 0x00, INSTR_NAME_LEN_MAX + 1);
		strcpy(outbox, instr_name);
		break;

	case INSTR_CMD_UPDATE:
		instr_timer = time_get(INSTR_UPDATE_MS + 1000);
		break;

	case MATRIX_CMD_GET_SIZE:
		outbox[0] = matrix_bus;
		outbox[1] = matrix_row;
		break;

	case MATRIX_CMD_SET_IMAGE:
		ret = 1;
		crc16 = inbox[2];
		crc16 = (crc16 << 8) | inbox[3];
		if(cyg_crc16(&inbox[4], inbox[1]) == crc16) {
			if(inbox[1] <= matrix_image_bytes) {
				memcpy(matrix_image, &inbox[4], inbox[1]);
				__matrix_set_image();
				ret = 0;
			}
		}
		break;

	case 0:
	default:
		return;
	}

	instr_server.outbox[0] = inbox[0];
	instr_server.outbox[1] = ret;
	inbox[0] = 0; //clear inbox testid indicate cmd ops finished!
}

static void matrix_init(void)
{
	matrix_uuid.class = INSTR_CLASS_MATRIX;
	matrix_uuid.index = 0;
	matrix_uuid.crc_name = cyg_crc16((unsigned char *)instr_name, strlen(instr_name));
	instr_ecode = 0;
	instr_mode = INSTR_MODE_INIT;
	led_off(LED_GREEN);
	led_flash(LED_RED);

	spi_cfg_t cfg_74hc595 = SPI_CFG_DEF;
	cfg_74hc595.cpol = 0; /*ck idle low*/
	cfg_74hc595.cpha = 0; /*ck 1st edge active*/
	cfg_74hc595.bits = 8; /*8bit per chip*/
	cfg_74hc595.bseq = 1; /*msb*/
	cfg_74hc595.csel = 1; /*manual ctrl cs*/
	cfg_74hc595.freq = 25000000;
	matrix_spi->init(&cfg_74hc595);

	matrix_spi->csel(pin_oe, 1);
	memset(matrix_image, 0x00, matrix_image_bytes);
	__matrix_set_image();
	matrix_spi->csel(pin_oe, 0);

	//debug
#if 0
	while(1) {
		for(int i = 0; i < 64; i ++) {
			memset(matrix_image, 0x00, 8);
			__matrix_set_image();
			sys_mdelay(10);
			bit_set(i, matrix_image);
			__matrix_set_image();
			sys_mdelay(10);
		}
		sys_mdelay(5000);
	}
#endif
	com_init();
}

static void matrix_update(void)
{
	com_update();
}

void main(void)
{
	sys_init();
	matrix_init();
	while(1) {
		sys_update();
		matrix_update();
		if(time_left(instr_timer) < 0) {
			//reset
			__set_FAULTMASK(1); //close all irq
			NVIC_SystemReset();
		}
	}
}
