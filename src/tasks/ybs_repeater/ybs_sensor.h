/*
 * 	king@2012 initial version
 */

#ifndef __YBS_SENSOR_H_
#define __YBS_SENSOR_H_
#include "ybs_sensor.h"
#include "ybs_repeater.h"

#define NORMALDATASIZESHIFT 8			//please change this para if you want to change the storage
#define NORMALDATASIZE 1 << NORMALDATASIZESHIFT	//the number of points to store	2Bytes/point

#define ALARMDATASIZESHIFT 3
#define ALARMDATASIZE 1 << ALARMDATASIZESHIFT

#define ALARMDATANUMBERSHIFT 3
#define ALARMDATANUMBER 1 << ALARMDATANUMBERSHIFT

#define ALARMCNT 5
#define CUTCNT 5
#define NORESPONSECNT 5

#define SENSORNUMBER 12

typedef struct {
	uint32_t data_size;
	uint32_t data_now;
	struct {
		RTC_TIME_Type time;
		uint32_t backward_data_now;
		uint32_t forward_data_now;
		uint16_t data[ALARMDATASIZE];
	} data[ALARMDATANUMBER];
} ybs_sensor_alarmdata_t;

typedef struct {
	uint16_t data[NORMALDATASIZE];
	uint32_t data_size;
	uint32_t record_data_now;
	uint32_t data_now;
} ybs_sensor_normaldata_t;


typedef struct {
	ybs_sensor_alarmdata_t alarmdata;
	ybs_sensor_normaldata_t normaldata;
	uint16_t average;
	uint16_t low_average;
	uint16_t high_average;
	uint16_t high_value;
	uint16_t low_value;
	uint16_t high_alarm;				//high value of alarm
	uint16_t low_alarm;				//low value of alarm
	uint16_t high_cut;				//high value of cut
	uint16_t low_cut;				//low value of cut
	uint16_t alarm_cnt;
	uint16_t cut_cnt;
	uint32_t alarm;
	uint32_t cut;
	uint16_t noresponse_cnt;
	/* 
	 * 	bit0: initialized flag	0:no		1:yes
	 * 	bit1: alarm flag	0:disable	1:enable
	 * 	bit2: backward record flag		0:disable	1:enable
	 * 	bit3: forward record flag		0:disable	1:enable
	 * 	bit4: no response flag			0:response	1:no response
	 * 	bit5: para error flag			0:no error	1:error		average¼Ó¼õÆ«ÒÆÒç³ö´íÎó
	 */
	uint32_t status;
} ybs_sensor_t;

void sensor_init();
void sensor_update();
void read_sensor(uint32_t com, uint16_t *bank0, uint16_t *bank1);
void write_sensor(uint32_t com, uint8_t cmd);
void set_sensor();
uint16_t get_sensor_average(uint32_t com);
void analize_sensor(uint32_t com, uint16_t data);
void record_sensor(uint32_t com);

#endif /*__YBS_SENSOR_H_*/