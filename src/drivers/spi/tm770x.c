/*
*	miaofng@2010 initial version
*/

#include "config.h"
#include "tm770x.h"
#include "spi.h"
#include "gpio.h"
#include "shell/cmd.h"

#define REG0_COMM 0x01
#define REG1_CONF 0x11
#define REG2_FILH 0x21
#define REG3_DATA 0x33
#define REG4_TEST 0x41
#define REG5_FILL 0x51
#define REG6_CALZ 0x63
#define REG7_CALF 0x73

static const tm770x_t *tm_chip = NULL;
static int tm_ainx = 0;

static void wreg(int reg, int data)
{
	gpio_set_h(tm_chip->pin_nss, 0);
	tm_chip->bus->wreg(0, (reg & 0xf0) | tm_ainx);
	gpio_set_h(tm_chip->pin_nss, 1);

	gpio_set_h(tm_chip->pin_nss, 0);
	tm_chip->bus->wreg(0, data);
	gpio_set_h(tm_chip->pin_nss, 1);
}

static int rreg(int reg)
{
	int bytes = reg & 0x0f;
	int value = 0;

	gpio_set_h(tm_chip->pin_nss, 0);
	tm_chip->bus->wreg(0, (reg & 0xf0) | (1 << 3) | tm_ainx);
	gpio_set_h(tm_chip->pin_nss, 1);

	gpio_set_h(tm_chip->pin_nss, 0);
	for(int i = 0; i < bytes; i ++) {
		int d = tm_chip->bus->wreg(0, 0xff);
		value = (value << 8) | d;
	}
	gpio_set_h(tm_chip->pin_nss, 1);
	return value;
}

void tm770x_init(const tm770x_t *chip, int notch_hz)
{
	spi_cfg_t cfg = {
		.cpol = 1, //idle high
		.cpha = 1, //2nd edge(upgoing edge)
		.bits = 8,
		.bseq = 1, //msb
		.freq = 2000000, //100nS high & low at least
	};

	chip->bus->init(&cfg);
	gpio_set_h(chip->pin_nss, 1);
	if(chip->pin_nrst != GPIO_NONE) {
		gpio_set_h(chip->pin_nrst, 0);
		gpio_set_h(chip->pin_nrst, 1);
	}

	tm_chip = chip;
	int code = chip->mclk_hz / 128 / notch_hz;
	code = (code < 19) ? 19 : code;
	code = (code > 4000) ? 4000 : code;
	notch_hz = chip->mclk_hz / 128 / code;
	printf("tm770x: notch = %d Hz\n", notch_hz);
	wreg(REG5_FILL, code >> 0);
	wreg(REG2_FILH, code >> 8);
	tm770x_config(chip, 1, 0, 1);

	//must be done before read!!
	tm770x_cal_self(chip, 0);
	tm770x_cal_self(chip, 1);
}

static int log2(int y)
{
	int x = 0;
	for(x = 0; (1 << x) < y; x ++);
	return x;
}

void tm770x_config(const tm770x_t *chip, int gain, int buf, int unipolar)
{
	gain = log2(gain);
	buf = buf ? 1 : 0;
	unipolar = unipolar ? 1 : 0;
	tm_chip = chip;
	wreg(REG1_CONF, (0 << 6) | (gain << 3) | (unipolar << 2) | (buf << 2));
}

void tm770x_cal_self(const tm770x_t *chip, int ch)
{
	tm_chip = chip;
	tm_ainx = ch;

	int cfg = rreg(REG1_CONF);
	cfg |= 1 << 6;
	wreg(REG1_CONF, cfg);
	while(tm770x_poll(chip) == 0);
}

int tm770x_read(const tm770x_t *chip, int AIN)
{
	tm_chip = chip;
	tm_ainx = AIN;
	int value = rreg(REG3_DATA);
	return value;
}

int tm770x_poll(const tm770x_t *chip)
{
	tm_chip = chip;
	int value = rreg(REG0_COMM);
	return (value & 0x80) ? 0 : 1;
}

static int cmd_tm770x_func(int argc, char *argv[])
{
	const char *usage = {
		"usage:\n"
		"tm770x reg		read reg\n"
		"tm770x reg v		write reg = v\n"
		"tm770x cfg g b p	gain, buf, unipolar\n"
		"tm770x cal ch		ch must be cal before adc\n"
		"tm770x adc ch [vref]	ch=0/1, vref=3.0v\n"
	};

	if(argc < 2) {
		printf("%s", usage);
		return 0;
	}

	if(!strcmp(argv[1], "adc")) {
		int ch = 0;
		float vref = 3.000;

		if(argc > 2) sscanf(argv[2], "%d", &ch);
		if(argc > 3) sscanf(argv[3], "%f", &vref);
		int adc = tm770x_read(tm_chip, ch);
		float v = (vref * adc) / (1 << 24);
		printf("adc = 0x%06x (%1.6fv)\n", adc, v);
		return 0;
	}

	if(!strcmp(argv[1], "cfg")) {
		int g = 1; //gain = 1
		int b = 0; //buf off
		int p = 1; //unipolar

		if(argc > 2) sscanf(argv[2], "%d", &g);
		if(argc > 3) sscanf(argv[3], "%d", &b);
		if(argc > 4) sscanf(argv[3], "%d", &p);

		tm770x_config(tm_chip, g, b, p);
		return 0;
	}

	if(!strcmp(argv[1], "cal")) {
		int ch = 0;
		if(argc > 2) sscanf(argv[2], "%d", &ch);

		tm770x_cal_self(tm_chip, ch);
		return 0;
	}

	int reg = 0;
	int val = -1;

	if((argv[1][1] == 'x') || (argv[1][1] == 'X')) sscanf(argv[1], "%x", &reg);
	else sscanf(argv[1], "%d", &reg);

	if(argc > 2) {
		if((argv[2][1] == 'x') || (argv[2][1] == 'X')) sscanf(argv[2], "%x", &val);
		else sscanf(argv[2], "%d", &val);
	}

	//convert reg to macro definition
	const int reg_list[] = {
		REG0_COMM,
		REG1_CONF,
		REG2_FILH,
		REG3_DATA,
		REG4_TEST,
		REG5_FILL,
		REG6_CALZ,
		REG7_CALF,
	};

	if(val >= 0) wreg(reg_list[reg & 0x07], val);
	else {
		val = rreg(reg_list[reg & 0x07]);
		int bytes = reg_list[reg & 0x07] & 0x0f;
		if(bytes == 1) printf("reg[%02x] = 0x%02x\n", reg, val);
		else printf("reg[%02x] = 0x%06x\n", reg, val);
	}
	return 0;
}

cmd_t cmd_tm770x = {"tm770x", cmd_tm770x_func, "tm770x i/f commands"};
DECLARE_SHELL_CMD(cmd_tm770x)
