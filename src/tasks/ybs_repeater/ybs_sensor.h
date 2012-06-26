/*
 * 	king@2012 initial version
 */

#ifndef __YBS_SENSOR_H_
#define __YBS_SENSOR_H_

#define NO_RESPONSE ((unsigned short)0x8000)

#define SENSOR_NUMBER (6 << 1)
#define EVENT_NUMBER (1 << 8)

typedef struct {
	struct {
		unsigned int time;
		unsigned char error;
	} event[EVENT_NUMBER];
	unsigned short average;
	unsigned short high_average;
	unsigned short low_average;
	unsigned short high_alarm;
	unsigned short low_alarm;
	unsigned short high_cut;
	unsigned short low_cut;
	int alarm_cnt;
	int cut_cnt;
	int no_response_cnt;
	union {
		struct {
			unsigned int init_flag : 1;
			unsigned int alarm_flag : 1;
			unsigned int cut_flag : 1;
			unsigned int no_response_flag : 1;
			unsigned int new_data_flag : 1;
			unsigned int sensor_status : 2;	// 0:start 1:run 2:end
		};
		unsigned int value;
	} status;
} ybs_sensor_t;

void sensor_init();
void sensor_update();
void read_sensor(unsigned short *bank0, unsigned short *bank1);
void write_sensor(unsigned int sensor, unsigned char cmd);

#endif /*__YBS_SENSOR_H_*/