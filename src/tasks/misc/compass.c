/*
*
*  miaofng@2016-09-12 routine for usbhub ctrl board v2.x
*
*/
#include "ulp/sys.h"
#include "led.h"
#include <string.h>
#include <ctype.h>
#include "shell/shell.h"
#include "common/bitops.h"
#include "stm32f10x.h"
#include "gpio.h"
#include "spi.h"

#include "ak09915.h"

const spi_bus_t *cmps_spi = &spi1;

void cmps_init(void)
{
	GPIO_BIND(GPIO_PP0, PC0, LED_R)
	GPIO_BIND(GPIO_PP1, PC1, LED_G)

	//mux
	GPIO_BIND(GPIO_PP0, PB10, MUX_SEL)
	GPIO_BIND(GPIO_PP1, PB11, MUX_EN#)

	//uut
	GPIO_BIND(GPIO_PP0, PB14, A_VEN);
	GPIO_BIND(GPIO_PP0, PB15, B_VEN);

	GPIO_BIND(GPIO_DIN, PB13, A_RDY#);
	GPIO_BIND(GPIO_DIN, PB12, B_RDY#);

	//fixture i/o
	GPIO_BIND(GPIO_IPU, PB01, A_START);
	GPIO_BIND(GPIO_PP0, PB09, A_PASS);
	GPIO_BIND(GPIO_PP0, PB08, A_FAIL);

	GPIO_BIND(GPIO_IPU, PB00, B_START);
	GPIO_BIND(GPIO_PP0, PB06, B_PASS);
	GPIO_BIND(GPIO_PP0, PB05, B_FAIL);

	const spi_cfg_t ak_cfg = {
		.cpol = 1, //idle high
		.cpha = 1,
		.bits = 8,
		.bseq = 1, //msb 1st
		.csel = 1,
		.freq = 1000000,
	};

	cmps_spi->init(&ak_cfg);
}

void cmps_update(void)
{
}

void main()
{
	sys_init();
	//shell_mute(NULL);
	cmps_init();
	printf("compass tester v1.x, build: %s %s\n\r", __DATE__, __TIME__);
	while(1){
		sys_update();
		cmps_update();
	}
}

int rreg(char pos, int addr)
{
	GPIO_SET(MUX_SEL, (pos == 'A') ? 0 : 1)
	GPIO_SET(MUX_EN#, 0)

	//msb = 1, read
	addr &= 0x7f;
	addr |= 0x80;

	cmps_spi->csel(SPI_1_NSS, 0);
	cmps_spi->wreg(0, addr);
	int data = cmps_spi->rreg(0);
	cmps_spi->csel(SPI_1_NSS, 1);

	GPIO_SET(MUX_EN#, 1)
	return data;
}

int wreg(char pos, int addr, int value)
{
	GPIO_SET(MUX_SEL, (pos == 'A') ? 0 : 1)
	GPIO_SET(MUX_EN#, 0)

	//msb = 0, write
	addr &= 0x7f;

	cmps_spi->csel(SPI_1_NSS, 0);
	cmps_spi->wreg(0, addr);
	cmps_spi->wreg(0, value);
	cmps_spi->csel(SPI_1_NSS, 1);

	GPIO_SET(MUX_EN#, 1)
	return 0;
}

static char *trim(char *str)
{
	char *p = str;
	char *p1;
	if(p) {
		p1 = p + strlen(str) - 1;
		while(*p && isspace(*p)) p++;
		while(p1 > p && isspace(*p1)) *p1-- = '\0';
	}
	return p;
}

static int cmd_cmps_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"cmps A reg		read uut A or B reg\n"
		"cmps A reg=val		write reg 0x31=0x10\n"
		"cmps A test		self-test\n"
		"cmps A sens		real measurement\n"
	};

	if(argc != 3) {
		printf("%s", usage);
		return 0;
	}

	char *p = strchr(argv[2], '=');
	if(p != NULL) { //write
		*p = 0;
		char *str_reg = trim(argv[2]);
		char *str_val = trim(p + 1);

		int reg = -1;
		int val = -1;

		if(!strncmp(str_reg, "0x", 2)) sscanf(str_reg, "0x%x", &reg);
		else sscanf(str_reg, "%d", &reg);

		if(!strncmp(str_val, "0x", 2)) sscanf(str_val, "0x%x", &val);
		else sscanf(str_val, "%d", &val);

		if((reg >= 0) && (val >= 0)) {
			int ecode = wreg(argv[1][0], reg, val);
			printf("<%+d\n", ecode);
		}
	}
	else { //read
		if(!strcmp(argv[2], "test")) { //self-test
			char pos = argv[1][0];
			if(pos == 'A') GPIO_SET(A_VEN, 1)
			else GPIO_SET(B_VEN, 1)

			GPIO_SET(MUX_SEL, (pos == 'A') ? 0 : 1)
			GPIO_SET(MUX_EN#, 0)

			wreg(pos, AK_CNTL2, 0x00);
			wreg(pos, AK_CNTL2, 0X10);
			sys_mdelay(10);
			printf("ST1 = 0x%02x\n", rreg(pos, AK_ST1));
			short v = 0;

			v = rreg(pos, AK_HXH);
			v = (v << 8) | rreg(pos, AK_HXL);
			printf("HX = %d\n", v);

			v = rreg(pos, AK_HYH);
			v = (v << 8) | rreg(pos, AK_HYL);
			printf("HY = %d\n", v);

			v = rreg(pos, AK_HZH);
			v = (v << 8) | rreg(pos, AK_HZL);
			printf("HZ = %d\n", v);

			GPIO_SET(MUX_EN#, 1)
			return 0;
		}

		if(!strcmp(argv[2], "sens")) { //real measurement
			char pos = argv[1][0];
			if(pos == 'A') GPIO_SET(A_VEN, 1)
			else GPIO_SET(B_VEN, 1)

			GPIO_SET(MUX_SEL, (pos == 'A') ? 0 : 1)
			GPIO_SET(MUX_EN#, 0)

			wreg(pos, AK_CNTL2, 0x00);
			wreg(pos, AK_CNTL2, 0X01 | (1 << 6));
			sys_mdelay(10);
			//printf("ST1 = 0x%02x\n", rreg(pos, AK_ST1));
			short v = 0, hx, hy, hz;

			v = rreg(pos, AK_HXH);
			v = (v << 8) | rreg(pos, AK_HXL);
			hx = v;

			v = rreg(pos, AK_HYH);
			v = (v << 8) | rreg(pos, AK_HYL);
			hy = v;

			v = rreg(pos, AK_HZH);
			v = (v << 8) | rreg(pos, AK_HZL);
			hz = v;

			GPIO_SET(MUX_EN#, 1)
			printf("hx = %06d, hy=%06d, hz=%06d\n", hx, hy, hz);
			return 0;
		}

		int reg = -1;
		if(!strncmp(argv[2], "0x", 2)) sscanf(argv[2], "0x%x", &reg);
		else sscanf(argv[2], "%d", &reg);
		if(reg >= 0) {
			int val = rreg(argv[1][0], reg);
			printf("<%+d\n", val);
		}
	}

	return 0;
}

cmd_t cmd_cmps = {"cmps", cmd_cmps_func, "compass i/f commands"};
DECLARE_SHELL_CMD(cmd_cmps)

int cmd_xxx_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"*IDN?		to read identification string\n"
		"*RST		instrument reset\n"
	};

	if(!strcmp(argv[0], "*IDN?")) {
		printf("<0,Ulicar Technology,HUBPDI V1.x,%s,%s\n\r", __DATE__, __TIME__);
		return 0;
	}
	else if(!strcmp(argv[0], "*RST")) {
		printf("<+0, No Error\n\r");
		mdelay(50);
		NVIC_SystemReset();
	}
	else if(!strcmp(argv[0], "*?")) {
		printf("%s", usage);
		return 0;
	}
	else {
		printf("<-1, Unknown Command\n\r");
		return 0;
	}
	return 0;
}
