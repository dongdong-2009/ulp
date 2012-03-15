/*
 *	king@2011 initial version
 */

#include "psi5.h"
#include "sis_card.h"
#include <string.h>
#include "config.h"
#include "ulp/debug.h"
#include "ulp_time.h"
#include "stm32f10x.h"

#define START_TIME 30

static enum {
	PSI5_STM_INIT,
	PSI5_STM_RUN,

	PSI5_LEARN_START,
	PSI5_LEARN_BUSY,
	PSI5_LEARN_PASS,
	PSI5_LEARN_FAIL,
} psi5_stm = PSI5_STM_INIT;

const unsigned short ID[16] = {0x200, 0x201, 0x202, 0x203, 0x204, 0x205, 0x206, 0x207,
 0x208, 0x209, 0x20A, 0x20B, 0x20C, 0x20D, 0x20E, 0x20F};

#if 1
const unsigned short init_status[16] = {0x01E8, 0x01E8, 0x01E7, 0x01E7};
#endif

static enum {
	PSI5_DECODE_INIT,//before start
	PSI5_DECODE_START,//start
} psi5_decode = PSI5_DECODE_INIT;

static int msg_read = 0;

static struct psi5_sensor_s psi5_sensor;

static time_t psi5_timer;
static int fifo_size_used = 0;
#define PSI5_FIFO_SIZE (15360) //(15K)  (2 * 8 * 32 + 1(min)) * 13bit/msg * 2edge/bit
static unsigned short psi5_start;
static unsigned short psi5_end;
static unsigned short psi5_size;
static unsigned short psi5_fifo[PSI5_FIFO_SIZE];
static unsigned int msg[1024];
static char msg_bits[1024];
static char systick_task_flag;
int msg_to_send;
static unsigned int init_msg_num;

static unsigned int even_parity(unsigned int v) //偶校验
{
	unsigned int i, mask, n;
	for(i = 0, n = 0; i <31; i++) {
		mask = 1 << i;
		if(v & mask) {
			if(++n == 2)
				n = 0;
		}
	}
	return n;
}

/*
* polynom = x^3 + x + 1, 1011 = 11
* 3 bits CRC
*/
static unsigned int crc3(unsigned int v)
{
	int i, mask, p = 11;
	v <<= 3;
	for(i = 27; i >= 0; i--) {
		mask = 1 << i + 3;
		if(v & mask)
			v ^= (p << i);
	}
	return v;
}

static unsigned int psi5_msg_process(unsigned int data,unsigned char length ,unsigned char parity) //length不包括起始位
{
	int j;
	unsigned int m, n;
	if(parity == PSI5_EVENPARITY) {
		if(data & 0x0001) {
			for(j = 10; j < length - 1; j++) {
				data <<= 1;
				data ++;
			}
		}
		else
			data <<= (length - 11);
		data += (even_parity(data) << (length - 1));
	}
	else if(parity == PSI5_3CRC){
		if(data & 0x0001) {
			for(j = 10; j < length - 3; j++) {
				data <<= 1;
				data ++;
			}
		}
		else
			data <<= (length - 13);
		m = crc3(data);
		for(j = 2; j >= 0; j--) {
			if(m & (1 << j))
				n++;
			n <<= j;
		}
		data += (n << (length - 1));
	}
	return data;
}

static void psi5_encode(unsigned short m)
{
	int k;
	m = psi5_msg_process(m, psi5_sensor.data_bits, psi5_sensor.parity);
	if(!msg_to_send) {
		psi5_fifo[psi5_start] = START_TIME + 1;
		psi5_start++;
		psi5_fifo[psi5_start] = psi5_fifo[psi5_start - 1] + 1;
		psi5_start++;
	}
	else {
		psi5_fifo[psi5_start & 0x0fff] = psi5_fifo[(psi5_start - 1) & 0x0fff] + 1;
		psi5_start++;
		psi5_fifo[psi5_start & 0x0fff] = psi5_fifo[(psi5_start - 1) & 0x0fff] + 1;
		psi5_start++;
	}
	if(msg_to_send < init_msg_num)
		msg_to_send++;
	for(k = 0; k < psi5_sensor.data_bits; k++) {
		if(k) {
			if(!(m & (1 << (k - 1))) == !(m & (1 << k))) {
				psi5_fifo[psi5_start & 0x0fff] = psi5_fifo[(psi5_start - 1) & 0x0fff] + 1;
				psi5_start++;
				psi5_fifo[psi5_start & 0x0fff] = psi5_fifo[(psi5_start - 1) & 0x0fff] + 1;
				psi5_start++;
			}
			else {
				psi5_fifo[psi5_start & 0x0fff] = psi5_fifo[(psi5_start - 1) & 0x0fff] + 2;
				psi5_start++;
			}
		}
		else {
			if(m & (1 << k)) {
				psi5_fifo[psi5_start & 0x0fff] = psi5_fifo[(psi5_start - 1) & 0x0fff] + 2;
				psi5_start++;
			}
			else {
				psi5_fifo[psi5_start & 0x0fff] = psi5_fifo[(psi5_start - 1) & 0x0fff] + 1;
				psi5_start++;
				psi5_fifo[psi5_start & 0x0fff] = psi5_fifo[(psi5_start - 1) & 0x0fff] + 1;
				psi5_start++;
			}
		}
		if((k == (psi5_sensor.data_bits - 1)) && ((m & (1 << k)) == 0)) {
			psi5_fifo[psi5_start & 0x0fff] = psi5_fifo[(psi5_start - 1) & 0x0fff] + 1;
			psi5_start++;
		}
	}
	psi5_fifo[psi5_start & 0x0fff] = psi5_fifo[(psi5_start & 0x0fff) - 1] + START_TIME;
	psi5_start++;
}

static unsigned short message_get(int m)
{
	int j;
	j = 2 * psi5_sensor.init_data_num * psi5_sensor.init_repeat;
	if(m < j) {
		j = m / 2 / psi5_sensor.init_repeat;
		if(m & 0x01)
			return (j & 0x01) ? ((psi5_sensor.init_data[j / 2] >> 4) | 0x0210) : ((psi5_sensor.init_data[j / 2] & 0x000f) | 0x0210);
		else
			return ID[j & 0x000f];
	}
	else {
		j += psi5_sensor.status_data_num;
		if(m < j) {
			j = m - 2 * psi5_sensor.init_data_num * psi5_sensor.init_repeat;
			return init_status[j];
		}
		else
			return 0x00;
	}
}

void psi5_init(struct psi5_sensor_s *sensor, void *cfg)
{
	int div;
	systick_task_flag = 0;
	msg_to_send = 0;
	psi5_start = 0;
	psi5_end = 0;
	memcpy(&psi5_sensor, sensor, sizeof(psi5_sensor));
	init_msg_num = 2 * psi5_sensor.init_data_num * psi5_sensor.init_repeat + psi5_sensor.status_data_num;
	if(psi5_sensor.speed == PSI5_SPEED_125KBPS)
		div = 288;// 288 / 72MHz = 8 uS / 2
	else if(psi5_sensor.speed == PSI5_SPEED_189KBPS)
		div = 191;// 191 / 72MHz = 5.3 uS / 2
	card_player_init(2, 40, div);
	while(psi5_start < (0xfff - 100))
		psi5_encode(message_get(msg_to_send));
	psi5_stm = PSI5_STM_INIT;
	psi5_timer = time_get(PSI5_BLK_MS);;
}

void psi5_update(void)
{
	switch(psi5_stm) {
	case PSI5_STM_INIT:
		if(time_left(psi5_timer) > 0)
			return;
		psi5_card_player_start(psi5_fifo, 1 << 12, START_TIME);
		systick_task_flag = 1;
		psi5_stm = PSI5_STM_RUN;
	case PSI5_STM_RUN:
		break;
	default:
		break;
	}
}

void psi5_poweroff(void)
{
	card_player_stop();
}

void task_tick(void)
{
	if(systick_task_flag) {
		int j;
		psi5_end = (1 << 12) - DMA_GetCurrDataCounter(DMA1_Channel7);
		j = psi5_end > (psi5_start & 0x0fff) ? psi5_end - (psi5_start & 0x0fff) : psi5_end + (1 << 12) - (psi5_start & 0x0fff);
		while(j > 200) {
			psi5_encode(message_get(msg_to_send));
			j = psi5_end > (psi5_start & 0x0fff) ? psi5_end - (psi5_start & 0x0fff) : psi5_end + (1 << 12) - (psi5_start & 0x0fff);
		}
	}
}

static const char psi5_us[3][2] = {
	{5, 9}, //125kbps-189kbps
	{7, 9}, //125kbps	T=7.6~8.4uS
	{5, 6}, //189kbps	T=5.0~5.6uS
};

static int psi5_learn_speed(struct psi5_sensor_s *sensor, int n)
{
	int i, us;
	int min, cnt = 0, us_min, us_max;
	char speed = PSI5_SPEED_INVALID;

	us_min = psi5_us[0][0];
	us_max = psi5_us[0][1];
	min = us_max;
	do {
		//find smallest interval
		for(i = 1; i < n; i ++) {
			us = psi5_fifo[i] - psi5_fifo[i - 1];
			if((us < us_min) || (us > us_max))
				continue;
			if(us < min) {
				min = us;
				cnt = 0;
			}

			cnt += (us == min) ? 1 : 0;
			if(cnt > 16)
				break;
		}
		//is it an approprite interval?
		if(cnt > 16) {
			for(i = 1; i < 3; i ++) {
				if((min >= psi5_us[i][0]) && (min <= psi5_us[i][1])) {
					speed = i;
					break;
				}
			}

			if(i < 3)
				break;
			else
				return -1;//us_min = min;
		}
	} while(us_min <= us_max);
	if(speed != PSI5_SPEED_INVALID) {
		sensor -> speed = speed;
		return 0;
	}
	return -1;
}

/*
*	return value:	0:
			-1:
*			-2:
*			-3:
*/

static int psi5_learn_decode(unsigned int *msg, char *n, int speed)
{
	int us, us_min, us_max;
	int i, w, bits = 0, bits_temp = 0, nbit = 0;
	*msg = 0;
	psi5_decode = PSI5_DECODE_INIT;
	do {
		w = -1;
		psi5_size ++;
		if(fifo_size_used < psi5_size + 1)
			return 0;
		us = psi5_fifo[psi5_size] - psi5_fifo[psi5_size - 1];
		us = (us < 0) ? (us + 0x10000) : us;

		switch (psi5_decode) {
		case PSI5_DECODE_INIT:
			us_min = psi5_us[speed][0];
			us_max = psi5_us[speed][1];
			if((us >= us_min) && (us <= us_max)) {//start bits received
				psi5_decode = PSI5_DECODE_START;
				bits <<= 2;
				bits |= 0x00;
				nbit += 2;
				break;
			}
			else
				return -1;//frame start bits error

		case PSI5_DECODE_START:
			us_min = psi5_us[speed][0];
			us_max = psi5_us[speed][1];
			if((us >= us_min) && (us <= us_max))// 1T
				w = 2;
			else {
				us_min = us_min + (us_min >> 1);
				us_max = us_max + ((us_max + 1) >> 1);
				if((us >= us_min) && (us <= us_max))// 1.5T
					w = 3;
				else {
					us_min = psi5_us[speed][0] << 1;
					us_max = psi5_us[speed][1] << 1;
					if((us >= us_min) && (us <= us_max))// 2T
						w = 4;
					else if(us > us_max)// >2T
						w = 5;
					else
						return -2;//Tbit error
				}
			}
			bits_temp = bits & 0x03;
			if(bits_temp == 0) {
				if(w == 2) {
					bits <<= 1;//0 received
					bits |= 0x00;
					nbit ++;
				}
				else if(w == 3) {//11 received
					bits <<= 2;
					bits |= 0x03;
					nbit += 2;
				}
				else if(w == 4) {//10 received
					bits <<= 2;
					bits |= 0x02;
					nbit += 2;
				}
				else if(w == 5)//frame is over
					break;
			}
			else if(bits_temp == 1)
				return -3;//state error,不可能出现01的状态
			else if(bits_temp == 2) {
				if(w == 2) {
					bits <<= 1;//0 received
					bits |= 0x00;
					nbit ++;
				}
				else if(w == 3) {//11 received
					bits <<= 2;
					bits |= 0x03;
					nbit += 2;
				}
				else if(w == 4) {//10 received
					bits <<= 2;
					bits |= 0x02;
					nbit += 2;
				}
				else if(w == 5)//frame is over
					break;
			}
			else if(bits_temp == 3) {
				if(w == 2) {
					bits <<= 1;//1 received
					bits |= 0x01;
					nbit ++;
				}
				else if(w == 3) {//0 received
					bits <<= 1;
					bits |= 0x00;
					nbit ++;
				}
				else if(w == 4)
					return -3;
				else if(w == 5)//frame is over
					break;
			}
		}
	} while (w < 5);
	*n = nbit - 2;
	for(i = 0; i < *n; i++) {
		*msg <<= 1;
		if((1 << i) & bits)
			*msg += 1;
	}
	return 1;
}

void psi5_learn_init(void)
{
	card_recorder_init(NULL);
	memset(psi5_fifo, 0, sizeof(psi5_fifo));
	psi5_size = 0;
	psi5_timer = time_get(PSI5_BLK_MS_MIN);
	psi5_stm = PSI5_LEARN_START;
	memset(&psi5_sensor, 0, sizeof(psi5_sensor));
}

void psi5_learn_update(void)
{
	int i, n;
	if(psi5_stm == PSI5_LEARN_START) {
		if(time_left(psi5_timer) < 0) {
			card_recorder_start(psi5_fifo, PSI5_FIFO_SIZE, 0);
			psi5_stm = PSI5_LEARN_BUSY;
			psi5_timer = time_get(500); //capture 500mS
		}
		return;
	}
	if(psi5_stm != PSI5_LEARN_BUSY) return;
	n = PSI5_FIFO_SIZE - card_recorder_left();
	if((time_left(psi5_timer) > 0) && (n < 10240)) {
		return;
	}

	card_recorder_stop();
	fifo_size_used = PSI5_FIFO_SIZE - card_recorder_left();
	if(n < 200) {
		psi5_stm = PSI5_LEARN_FAIL;
		return;
	}
	if(psi5_learn_speed(&psi5_sensor, n) < 0) {
		psi5_stm = PSI5_LEARN_FAIL;
		return;
	}

	int min = 10,max = 11, k;
	for(i = 0;;i++) {//读取数据，并检测帧的位数
		k = psi5_learn_decode(&msg[i], &msg_bits[i], psi5_sensor.speed);
		if(k > 0) {
			if(msg_bits[i] > max) {
				max = msg_bits[i];
				min = max - 1;
			}
			if(msg_bits[i] < min) {
				psi5_stm = PSI5_LEARN_FAIL;
				return;
			}
		}
		else if(k == 0) {
			msg_read = i;
			psi5_sensor.data_bits = max;
			break;
		}
		else {
			psi5_stm = PSI5_LEARN_FAIL;
			return;
		}
	}
	for(i = 0; i < msg_read; i++) {//补全最后一位
		if(msg_bits[i] == psi5_sensor.data_bits - 1) {
			msg[i] |= 1 << msg_bits[i];
		}
		else if(msg_bits[i] < psi5_sensor.data_bits - 1) {
			psi5_stm = PSI5_LEARN_FAIL;
			return;
		}
	}
	for(i = 0; i < msg_read; i++) {//判断校验规则：偶校验 or 3位CRC
		if((msg[i] >> (psi5_sensor.data_bits - 1)) == even_parity(msg[i] & (~(1 << (psi5_sensor.data_bits - 1)))))
			psi5_sensor.parity = PSI5_EVENPARITY;
		else
			psi5_sensor.parity = PSI5_3CRC;
	}
	for(i = 0, k = 1; i < 32; i++) {//判断重复的次数
		if(msg[2 * i] == msg[2 * i + 2])
			k++;
		else
			break;
	}
	psi5_sensor.init_repeat = k;
	int j;
	for(i = 0, j = 0, k = 0;; j++, i++) {//判断初始化数据的个数
		if(i == 16)
			i = 0;
		if(((msg[j * psi5_sensor.init_repeat * 2] >> (psi5_sensor.data_bits - 11)) & 0x3ff) == ID[i])
			k++;
		else
			break;
	}
	psi5_sensor.init_data_num = k;
	for(i = 0; i < psi5_sensor.init_data_num; i++) {
		if(i & 0x01)
			psi5_sensor.init_data[i / 2] |= (msg[i * psi5_sensor.init_repeat * 2 + 1] >> (psi5_sensor.data_bits - 11) & 0x0f) << 4;
		else
			psi5_sensor.init_data[i / 2] = msg[i * psi5_sensor.init_repeat * 2 + 1] >> (psi5_sensor.data_bits - 11) & 0x0f;
	}
	for(i = 0, k = 0;; i++) {
		unsigned int temp;
		temp = msg[psi5_sensor.init_data_num * psi5_sensor.init_repeat * 2 + i] >> (psi5_sensor.data_bits - 11) & 0x3ff;
		if(temp <= 511 && temp >= 481) {
#if 0
			psi5_sensor.status_data[i] = temp;
#endif
			k++;
		}
		else
			break;
	}
	psi5_sensor.status_data_num = k;
	psi5_stm = PSI5_LEARN_PASS;
}

int psi5_learn_finish(void)
{
	return ((psi5_stm == PSI5_LEARN_PASS) || (psi5_stm == PSI5_LEARN_FAIL));
}

int psi5_learn_result(struct psi5_sensor_s *sensor)
{
	int ret = -1;
	if(psi5_stm == PSI5_LEARN_PASS) {
		memcpy(sensor, &psi5_sensor, sizeof(psi5_sensor));
		ret = 0;
	}
	return ret;
}
