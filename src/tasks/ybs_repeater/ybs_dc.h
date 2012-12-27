/*
 * ybs repeater data center
 * the data maintained by repeater dc(data center) is something like a tension-time curve.
 * it's non-continuous because we are only interested in the time fragment when an event occurs.
 * requirements:
 * 1, non-continuous tension-time data strorage
 * 3, the whole storage space will be divided into sectors with the size of DC_SECT
 * 3, tension data is saved one-by-one, its time-critical!!!
 * 4, search empty must be fast!!! it's executed in isr
 * 5, random seach through time function
 *
 * miaofng@2012
 *
 */
#ifndef __YBS_DC_H_
#define __YBS_DC_H_

#include "config.h"
#include "linux/list.h"
#include "ulp/lwip.h"
#include "ulp_time.h"

#define DC_BASE	0xA0000000
#define DC_SIZE 0x01000000 /*32MB*/
#define DC_SECT_SZ 1024 /*< 1536, better for ethernet, unit: bytes*/
#define DC_SECT_NR (DC_SIZE/DC_SECT_SZ)
#define DC_CH_NR 12 /*nr of ybs sensor channel*/
#define DC_FIFO_NP 512 /*note: this is the fifo size for ybs_dsp usage not +/-10S , unit: points*/
#define DC_SECT_EMPTY_TH ((int)(DC_SECT_NR * 0.3))
#define DC_SAVE_MS 10 /*sampling period*/
#define DC_SECT_MS (DC_SAVE_MS * DC_SECT_NP)
#define DC_TEMP_MS 50000 /*reclaim it if timeout*/
#define DC_DATA_NA ((short)(1 << 15))
#define DC_SEND_TCP_LIMIT (TCP_SND_BUF)
#define DC_HASH_MS 1000 /*must be smaller than DC_SECT_MS*/

struct dc_sect_s {
	time_t time;
	int ybs; //which sensor? 0 ~ 15
	int offset; //[0, DC_SECT_NP]
	int rserved;

	/*sector status machine: empty -> temp -> keep */
	struct list_head list_empty;
	struct list_head list_keep;
	struct list_head list_temp;
	struct list_head list_ybs; /*temp or keep*/

	/*for fast search only*/
	struct list_head queue_hash;
	struct list_head list_hash;

	/*real payload*/
	#define DC_SECT_HEAD_LEN 64 /*bytes*/
	#define DC_SECT_NP ((DC_SECT_SZ - DC_SECT_HEAD_LEN) / sizeof(short))
	short data[DC_SECT_NP];
};

int dc_init(void);
int dc_update(void);

/*to select a ybs sensor as the current one
 * ybs: 0-15
 */
void dc_config_ybs(int ch);

/* to save the tension data to fifo one by one,
 * time = time_get() is implied
  */
int dc_save(short tension);

/* to read the tension data from fifo one by one
 * range: offset = [0, -(DC_FIFO_SZ - 1)]
  */
int dc_read(int offset);

/* to indicate an event happened, and the tension data must be keeped to sectors
 * effective range: [start, end], both of them must be negtive
 * note:
 * 1, start&end smaller than -DC_FIFO_SZ is acceptable
 * 2, start < end !!!
 */
int dc_keep(int start, int end);

/* to read the waveform data and send to lwip directly,
 * range: [start, end], return the bytes has been sent
 */
int dc_send_tcp(struct tcp_pcb *pcb, int ch, time_t start, time_t end);

#endif
