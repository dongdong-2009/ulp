/*
 * 	junjun@2011 initial version
 */
#include "sys/task.h"
#include "ybs_gpcon.h"
#include "ulp_time.h"
#include <shell/cmd.h>
#include <string.h>
#include "spi.h"
#include "ad5663.h"

static const struct ad5663_chip_s ad5663_cfg = {
	.spi = &spi1,
	.gpio_cs = SPI_1_NSS,
	.gpio_ldac = SPI_CS_PA2,
	.gpio_clr = SPI_CS_PA3,
};

int ybs_Init(void)
{
	gpcon_init();
	dev_register("ad5663", &ad5663_cfg);
	return 0;
}

int ybs_Update(void)
{
	return 0;
}

int ybs_mdelay(int ms)
{
	int left;
	time_t deadline = time_get(ms);
	do {
		left = time_left(deadline);
		if(left >= 10) { //system update period is expected less than 10ms
			ybs_Update();
		}
	} while(left > 0);
	return 0;
}

static int cmd_ad5663(int dv)
{
	int fd = dev_open("dac", 0);
	dev_ioctl(fd, DAC_RESET);
	dev_ioctl(fd, DAC_CHOOSE_CHANNEL1);
	dev_write(fd,NULL,0);
	ybs_mdelay(1);
	dev_ioctl(fd, DAC_WRIN_UPDN,dv);
	dev_write(fd,NULL,0);
	dev_close(fd);
	return 0;
}




//ybs shell command
static int cmd_ybs_func(int argc, char *argv[])
{
	int dv;
	const char *usage[] = {
		{"ybs help            usage of ybs commond\n"},
		{"ybs pmos            simulator resistor button,usage:pmos on/off\n"},
		{"ybs ad5663          cmd of dac IC ad5663,usag:ad5663 xxx\n"}
	};

	if(argc > 1) {
		if(!strcmp(argv[1], "pmos")) {
			if(argc == 3){
				if(!strcmp(argv[2], "on"))
					gpcon_signal(SENSOR,SENPR_ON);
				if(!strcmp(argv[2], "off"))
					gpcon_signal(SENSOR,SENPR_OFF);
				return 0;
			}
			else
				return -1;
		}
		if(!strcmp(argv[1], "ad5663")){
			if(argc == 3){
				dv = atoi(argv[2]);
				printf("dv = %4d",dv);
				return cmd_ad5663(dv);
			}
			else
			printf("%s",usage[2]);
		}
	}
	//printf("%s", usage);
	return 0;
}

const static cmd_t cmd_ybs = {"ybs", cmd_ybs_func, "ybs debug command"};
DECLARE_SHELL_CMD(cmd_ybs)
