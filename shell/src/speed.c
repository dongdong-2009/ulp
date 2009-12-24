/*
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "motor.h"

int cmd_speed_func(int argc, char *argv[])
{
	short speed;
	
	if(argc < 2) {
		printf("uasge:\n");
		printf(" speed 100\n");
		return 0;
	}
	
	speed = (short)atoi(argv[1]);
	printf("setting motor speed to %dHz\n", speed);
	motor_SetSpeed(speed);
	return 0;
}
cmd_t cmd_speed = {"speed", cmd_speed_func, "set motor speed in Hz"};

int cmd_rpm_func(int argc, char *argv[])
{
	short rpm;
	
	if(argc < 2) {
		printf("uasge:\n");
		printf(" rpm 100\n");
		return 0;
	}
	
	rpm = (short)atoi(argv[1]);
	printf("setting motor speed to %dRPM\n", rpm);
	motor_SetRPM(rpm);
	return 0;
}
cmd_t cmd_rpm = {"rpm", cmd_rpm_func, "set motor speed in rpm"};

