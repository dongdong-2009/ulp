/*
 *	miaofng@2011 initial version
 */
#include "config.h"
#include "debug.h"
#include "time.h"
#include "dbs.h"
#include "sis_card.h"
#include <string.h>

static enum {
	DBS_STM_INIT,
	DBS_STM_BLACK,	/*comm blackout pedriod, DBS_BLK_MS*/
	DBS_STM_TRACE1,	/*1st group of 8 trace message + DBS_IMG_MS*/
	DBS_STM_TRACE2,	/*2nd group of 8 trace message + DBS_IMG_MS*/
	DBS_STM_OK,	/*2 ok message + DBS_OKD_MS*/
	DBS_STM_FAULT,	/*continous fault message*/
	DBS_STM_ACC,	/*continous acc message*/
	DBS_STM_ERROR,	/*dbs itself hardware error*/
} dbs_stm = DBS_STM_INIT;
static time_t dbs_timer;
#define DBS_FIFO_SIZE (4096) //10mS buffer * 8000msg/1S * (19 + 2)bit/msg * 2edge/bit = 3360
static unsigned short dbs_fifo[DBS_FIFO_SIZE];
static unsigned short dbs_size = 0; //fifo fize
static struct dbs_sensor_s dbs_sensor;

/*polynom = x^5 + x^2 + 1, 100101 = 0x25
31bits = 26bits + 5bits*/
static unsigned crc5(unsigned v)
{
	int i, mask, p = 0x25;
	v <<= 5; //26 + 5 = 31
	for(i = 25; i >= 0; i--) {
		mask = 1 << (i + 5);
		if(v & mask) {
			v ^= (p << i);
			//printf("v = 0x%04x, p = 0x%04x i = %02d\n", v, p, i);
		}
	}
	return v;
}

/*note:
 1, bits[20, 0] is effective input bits, and send out msb first
 2, encoder size output will fold back to 0, if dbs_fifo overflow
 3, encoder time output will fold back to 0, if time bigger than 65535
 4, give out a time point to dbs_fifo when an edge occurs
 */
int dbs_encode(int bits)
{
	int i, mask, bit_new, bit_old = -1;
	unsigned short time = dbs_fifo[dbs_size];
	for(i = 20; i >= 0; i --) {
		mask = 1 << i;
		bit_new = (bits & mask) ? 1 : 0;

		time = time + 0;
		if((bit_new != bit_old) && (bit_old != -1)) {
			dbs_fifo[dbs_size] = time;
			dbs_size = (dbs_size == DBS_FIFO_SIZE) ? (dbs_size + 1) : 0;
		}

		time = time + 1;
		dbs_fifo[dbs_size] = time;

		time = time + 1;
		bit_old = bit_new;
	}

	//add two idle bits at the msg tail
	time = time + 0;
	if(bit_old == 1) {
		dbs_fifo[dbs_size] = time;
		dbs_size = (dbs_size == DBS_FIFO_SIZE) ? (dbs_size + 1) : 0;
	}

	time = time + 4;
	dbs_fifo[dbs_size] = time;
	//note: do not increase dbs_size here!!!

	return 0;
}

int dbs_decode(union dbs_msg_s *msg)
{
	return 0;
}

void dbs_init(struct dbs_sensor_s *sensor, void *cfg)
{
	int div = sensor->speed;
	div *= 205; //72MHz / 205 / 2 = 5.694uS
	card_player_init(5, 25, div);
	memcpy(&dbs_sensor, sensor, sizeof(dbs_sensor));
	memset(dbs_fifo, 0, DBS_FIFO_SIZE);
	dbs_size = 0;
	dbs_timer = 0;
}

void dbs_update(void)
{
	union dbs_msg_s msg;
	int i;

	//wait?
	if(time_left(dbs_timer) > 0)
		return;

	switch(dbs_stm) {
	case DBS_STM_INIT:
		dbs_stm = DBS_STM_BLACK;
		dbs_timer = time_get(DBS_BLK_MS);
		break;
	case DBS_STM_BLACK:
		dbs_stm = DBS_STM_TRACE1;
		dbs_timer = time_get(DBS_IMG_MS);
		break;
	case DBS_STM_TRACE1:
	case DBS_STM_TRACE2:
		if(dbs_size == 0) {
			for(i = 0; i < 8; i ++) {
				msg.value = 0;
				msg.trace.start = 0x00; //0b00;
				msg.trace.type = 0x00; //0b00;
				msg.trace.addr = dbs_sensor.addr & 0x03;
				msg.trace.data = dbs_sensor.trace[i];
				msg.trace.crc = crc5(msg.value >> 8);
				dbs_encode(msg.value >> 3);
			}
			card_player_start(dbs_fifo, dbs_size, 0);
		}
		else {
			if(card_player_left() == 0) {
				dbs_size = 0;
				memset(dbs_fifo, 0, DBS_FIFO_SIZE);
				dbs_stm = DBS_STM_TRACE2;
				dbs_timer = time_get(DBS_IMG_MS);
			}
		}
		break;
	case DBS_STM_OK: //note: stm will stay at this state!!!
		if(dbs_size == 0) {
			msg.value = 0;
			msg.soh.start = 0x00; //0b00;
			msg.soh.type = 0x00; //0b00;
			msg.soh.d98 = 0x00; //0b00;
			msg.soh.addr = dbs_sensor.addr & 0x03;
			msg.soh.data = dbs_stm & 0x3f;
			msg.soh.crc = crc5(msg.value >> 8);
			dbs_encode(msg.value >> 3);
			card_player_start(dbs_fifo, dbs_size, 0);
		}
		else {
			if(card_player_left() == 0) {
				dbs_size = 0;
				memset(dbs_fifo, 0, DBS_FIFO_SIZE);
				dbs_stm = DBS_STM_OK;
				dbs_timer = time_get(DBS_OKD_MS);
			}
		}
		break;
	case DBS_STM_FAULT:
	case DBS_STM_ACC:
	case DBS_STM_ERROR:
	default:
		break;
	}
}

void dbs_poweroff(void)
{
	card_player_stop();
}

void dbs_learn_init(void)
{
	memset(dbs_fifo, 0, DBS_FIFO_SIZE);
	dbs_size = 0;
	card_recorder_init(NULL);
	card_recorder_start(dbs_fifo, DBS_FIFO_SIZE, 1);
}

void dbs_learn_update(void)
{
}

int dbs_learn_finish(void)
{
	return 0;
}

int dbs_learn_result(struct dbs_sensor_s *sensor)
{
	return 0;
}
