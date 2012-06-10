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

#define DC_BASE	0xA0000000
#define DC_SIZE 0x01000000 /*32MB*/
#define DC_SECT_SZ 1024 /*< 1536, better for ethernet, unit: bytes*/
#define DC_SECT_NR (DC_SIZE/DC_SECT_SZ)
#define DC_CH_NR 12 /*nr of ybs sensor channel*/
#define DC_FIFO_SZ 512 /*note: this is the fifo size for ybs_dsp usage not +/-10S , unit: words*/
#define DC_SECT_EMPTY_TH ((int)(DC_SECT_NR * 30%))
#define DC_SAVE_MS 10 /*sampling period*/
#define DC_TEMP_MS 20000 /*reclaim it if timeout*/

struct dc_sect_s {
	int ybs; //which sensor? 0 ~ 15
	int offset; //[0, DC_SECT]
	time_t time;

	/*for fast empty seach purpose, used when write operation*/
	struct list_head list_empty;
	/*the whole keeped data sector can be viewed as an circbuf, used when update operation to remove obsoleted data*/
	struct list_head list_keep;
	/*temp means it's used but not keeped,  free it if its old enough by update*/
	struct list_head list_temp;
	/*the whole keeped data sector of a specified ybs sensor, used when keep operation*/
	struct list_head list_ybs;
	/*a chained hash table, used when random time search operation*/
	struct list_head list_hash;
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

/* to read the waveform data to pbuf directly
 */
struct pbuf* dc_read_pbuf(time_t time, int n);

#endif
