/*
 * authors:
 * 	junjun@2011 initial version
 * 	feng.ni@2012 the concentration is the essence
 */
#include "sys/task.h"
#include "ybs_gpcon.h"
#include "ulp_time.h"
#include <shell/cmd.h>
#include <string.h>
#include "spi.h"
#include "ulp/dac.h"
#include "stm32f10x.h"


static const struct ad5663_cfg_s ad5663_cfg = {
	.spi = &spi1,
	.gpio_cs = SPI_1_NSS,
	.gpio_ldac = SPI_CS_PA2,
	.gpio_clr = SPI_CS_PA3,
	.ch = 0,
};

int ybs_Init(void)
{
	gpcon_init();
	adc_init();
	dev_register("ad5663", &ad5663_cfg);
	return 0;
}

static time_t timer = 0;
int ybs_Update(void)
{
        if(time_left(timer) < 0) {
              gpcon_signal(SENSOR,TOGGLE);
              timer = time_get(10);
        }
	return 0;
}

//ybs shell command
static int cmd_ybs_func(int argc, char *argv[])
{
	const char *usage = {
		"ybs help            usage of ybs commond\n"
		"ybs pmos            simulator resistor button,usage:pmos on/off\n"
		"ybs adc             adc single convert cmd,usage:adc on\n"
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
		if(!strcmp(argv[1], "adc")) {
			if(argc == 3){
				if(!strcmp(argv[2], "on")){
					printf("ADC convert result:%d\n",ADC_GetConversionValue(ADC1));
					ADC_SoftwareStartConvCmd(ADC1, ENABLE);
				}
				return 0;
			}
		}
	}
	printf("%s", usage);
	return 0;
}

const static cmd_t cmd_ybs = {"ybs", cmd_ybs_func, "ybs debug command"};
DECLARE_SHELL_CMD(cmd_ybs)
