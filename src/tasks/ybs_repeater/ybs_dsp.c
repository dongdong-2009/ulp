/*
 * 	king@2012 initial version
 */

#include "ybs_sensor.h"
#include "ybs_dc.h"

extern ybs_sensor_t sensor[SENSOR_NUMBER];
extern int average_point_number;
extern int alarm_cnt_max;
extern int cut_cnt_max;
extern int no_response_cnt_max;


static unsigned short max_dsp(unsigned short old_data, unsigned short new_data)
{
	return (new_data > old_data ? new_data : old_data);
}

static unsigned short min_dsp(unsigned short old_data, unsigned short new_data)
{
	return (new_data < old_data ? new_data : old_data);
}

static unsigned short average_dsp(unsigned short new_data, unsigned short old_data, unsigned short average, unsigned int size)
{
	unsigned int temp1;
	temp1 = (unsigned int)average * size;
	temp1 = temp1 - (unsigned int)old_data + (unsigned int)new_data;
	return (unsigned short)(temp1 / size);
}

unsigned short ybs_dsp(int sensor_now, unsigned short new_data)
{
	unsigned short temp1, temp2, ret;

	if(new_data >= NO_RESPONSE) {		//no response
		sensor[sensor_now].no_response_cnt++;
		ret = sensor[sensor_now].average | NO_RESPONSE;
		temp1 = sensor[sensor_now].average;
		if(sensor[sensor_now].no_response_cnt >= no_response_cnt_max) {		//no response is timeout
			sensor[sensor_now].status.no_response_flag = 1;
		}
	}
	else {
		sensor[sensor_now].no_response_cnt = sensor[sensor_now].no_response_cnt == 0 ? 0 : sensor[sensor_now].no_response_cnt - 1;
		ret = new_data;
		temp1 = new_data;

		if(new_data >= sensor[sensor_now].high_alarm | new_data <= sensor[sensor_now].low_alarm) {
			sensor[sensor_now].alarm_cnt++;
			if(sensor[sensor_now].alarm_cnt >= alarm_cnt_max) {
				sensor[sensor_now].status.alarm_flag = 1;
			}
		}
		else {
			sensor[sensor_now].alarm_cnt = sensor[sensor_now].alarm_cnt == 0 ? 0 : sensor[sensor_now].alarm_cnt - 1;
		}

		if(new_data >= sensor[sensor_now].high_cut | new_data <= sensor[sensor_now].low_cut) {
			sensor[sensor_now].cut_cnt++;
			if(sensor[sensor_now].cut_cnt >= cut_cnt_max) {
				sensor[sensor_now].status.cut_flag = 1;
			}
		}
		else {
			sensor[sensor_now].cut_cnt = sensor[sensor_now].cut_cnt == 0 ? 0 : sensor[sensor_now].cut_cnt - 1;
		}
	}

	temp2 = (unsigned short)dc_read(0 - average_point_number);
	temp2 = temp2 & (~NO_RESPONSE);

	//calculate the average
	sensor[sensor_now].average = average_dsp(temp1, temp2, sensor[sensor_now].average, average_point_number);
	sensor[sensor_now].high_average = max_dsp(sensor[sensor_now].high_average, sensor[sensor_now].average);
	sensor[sensor_now].low_average = min_dsp(sensor[sensor_now].low_average, sensor[sensor_now].average);

	return ret;
}
