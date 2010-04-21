/*
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "shell/cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "motor/motor.h"
#include "normalize.h"

static int cmd_speed_func(int argc, char *argv[])
{
	int speed;

	if(argc < 2) {
		printf("uasge:\n");
		printf(" speed 100\n");
		return 0;
	}

	speed = (short)atoi(argv[1]);
	printf("setting motor speed to %dHz\n", speed);
	speed = NOR_SPEED(speed);
	motor_SetSpeed((short)speed);
	motor_Start();
	return 0;
}
const cmd_t cmd_speed = {"speed", cmd_speed_func, "set motor speed in Hz"};
DECLARE_SHELL_CMD(cmd_speed)

static int cmd_rpm_func(int argc, char *argv[])
{
	int rpm, speed;

	if(argc < 2) {
		printf("uasge:\n");
		printf(" rpm 100\n");
		return 0;
	}

	rpm = (short)atoi(argv[1]);
	printf("setting motor speed to %dRPM\n", rpm);
	speed = RPM_TO_SPEED(rpm);
	speed = NOR_SPEED(speed);
	motor_SetSpeed((short)speed);
	motor_Start();
	return 0;
}
const cmd_t cmd_rpm = {"rpm", cmd_rpm_func, "set motor speed in rpm"};
DECLARE_SHELL_CMD(cmd_rpm)

static int cmd_motor_func(int argc, char *argv[])
{
	float rs, ls;
	int pn;
	if(argc < 4) {
		printf( \
			"uasge:\n" \
			" motor rs(Ohm) ls(mH) pn\n" \
			" motor 68 100 8\n" \
		);
		return 0;
	}

	rs = atof(argv[1]);
	ls = atof(argv[2]);
	pn = atoi(argv[3]);

	printf("rs = %fOhm\nls = %fmH\npn = %d\n", rs, ls, pn);
	motor->rs = (short) NOR_RES(rs);
	motor->ld = (short) NOR_IND(ls);
	motor->lq = (short) NOR_IND(ls);
	motor->pn = pn;
	return 0;
}
const cmd_t cmd_motor = {"motor", cmd_motor_func, "set motor paras"};
DECLARE_SHELL_CMD(cmd_motor)

static int cmd_ramp_func(int argc, char *argv[])
{
	short ms, rpm;
	int speed, mv;
	
	if((argc < 3) && (argc != 0)) {
		ms = motor->start_time;
		speed = _SPEED(motor->start_speed);
		rpm = (short)SPEED_TO_RPM(speed);
		mv = _VOL(Vramp);
		printf("uasge:\n");
		printf(" ramp %d %d		ms rpm\n", ms, rpm);
		printf(" ramp %d %d %d	ms rpm mv\n", ms, rpm, mv);
		return 0;
	}

	if(argc >= 3) {
		ms = (short)atoi(argv[1]);
		rpm = (short)atoi(argv[2]);

		speed = RPM_TO_SPEED(rpm);
		printf("setting motor ramp speed to %dHz\n", speed);

		speed = NOR_SPEED(speed);
		motor->start_speed = (short)speed;
		motor->start_time = ms;
	
		//try to restart motor
		motor_SetSpeed(0);
	}

	if(argc >= 4) {
		mv = atoi(argv[3]);
		Vramp = (short) NOR_VOL(mv);
	}
	
	if(motor_GetStatus() != MOTOR_IDLE)
		return 1;
	
	motor_SetSpeed(1);
	motor_Start();
	return 0;
}
const cmd_t cmd_ramp = {"ramp", cmd_ramp_func, "set motor ramp up paras"};
DECLARE_SHELL_CMD(cmd_ramp)

static void cmd_pid_usage(void)
{
	printf( \
			"uasge:\n" \
			" pid type kp ki\n" \
			" type = speed/flux/torque\n" \
			" pid speed 150 25\n" \
	);
}

static int cmd_pid_func(int argc, char *argv[])
{
	float fp,fi;
        short kp,ki;

	if(argc < 4) {
		cmd_pid_usage();
		return 0;
	}

	fp = atof(argv[2]);
	fi = atof(argv[3]);

        kp = (short) NOR_PID_GAIN(fp);
        ki = (short) NOR_PID_GAIN(fi);

        printf("normalized kp = %d, ki = %d\n", kp, ki);

	if(!strcmp(argv[1], "speed")) {
		pid_Config(pid_speed, kp, ki, 0);
	}
	else if(!strcmp(argv[1], "flux")) {
		pid_Config(pid_flux, kp, ki, 0);
	}
	else if(!strcmp(argv[1], "torque")) {
		pid_Config(pid_torque, kp, ki, 0);
	}
	else {
		cmd_pid_usage();
	}
	return 0;
}
const cmd_t cmd_pid = {"pid", cmd_pid_func, "set pid paras"};
DECLARE_SHELL_CMD(cmd_pid)

