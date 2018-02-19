/*
*
* miaofng@2018-02-19 initial version for fdiox board
* note:
* 1, max support 8 pcs of mcp23s17 in parallel connection!
* 2, mcp23s17 output doesn't not support OD mode
*
*/

#include "ulp/sys.h"
#include "led.h"
#include <string.h>
#include <ctype.h>
#include "shell/shell.h"
#include "common/bitops.h"
#include "gpio.h"
#include "common/debounce.h"
#include "mcp23s17.h"

typedef struct {
	const mcp23s17_t *chip;
	int index; //PA=>0, PB=>1
} gpio_port_t;

//pls do not access them directly, try to use: gpio_port()
static mcp23s17_t gpio_chip_temp;
static gpio_port_t gpio_port_temp = {.chip = NULL};

//bit mask, indicates which mcp23s17 chip has been reset
static int gpio_chip_reset_flag;

void gpio_mcp_init(const mcp23s17_t *mcp_bus)
{
	gpio_port_t *port = &gpio_port_temp;
	mcp23s17_t *chip = &gpio_chip_temp;

	port->chip = chip;
	memcpy(chip, mcp_bus, sizeof(mcp23s17_t));
	chip->option |= MCP23017_PORTA_IN;
	chip->option |= MCP23017_PORTB_IN;
	chip->option |= MCP23S17_HIGH_SPEED;
	gpio_chip_reset_flag = 0;
}

//bind := "mcp?:PAy" or "mcp?:PAyy", such as: "mcp0:PA01"
static const gpio_port_t *gpio_port(const char *bind)
{
	gpio_port_t *port = &gpio_port_temp;
	mcp23s17_t *chip = &gpio_chip_temp;

	//parse chip: 0-7
	int addr = atoi(&bind[3]);
	chip->option &= ~0x0f;
	chip->option |= addr & 0x0f;

	//parse port: PA or PB
	port->index = -1;
	port->index = (bind[6] == 'A') ? 0 : port->index;
	port->index = (bind[6] == 'B') ? 1 : port->index;
	return port;
}

//bind := "MCPx:PAy" or "MCPx:PAyy", such as: "mcp0:PA01"
static int gpio_pin(const char *bind)
{
	int pin = atoi(&bind[7]);
	int mask = 1 << pin;
	return mask;
}

static int gpio_set_hw(const gpio_t *gpio, int high)
{
	const gpio_port_t *port = gpio_port(gpio->bind);
	int mask = gpio_pin(gpio->bind);

	unsigned char addr;
	unsigned char regv = 0;

	//reg - olat
	addr = (unsigned char)(ADDR_OLATA + port->index);
	mcp23s17_ReadByte(port->chip, addr, &regv);
	regv &= ~ mask;
	if(high) regv |= mask;
	mcp23s17_WriteByte(port->chip, addr, regv);
	return 0;
}

static int gpio_get_hw(const gpio_t *gpio)
{
	const gpio_port_t *port = gpio_port(gpio->bind);
	int mask = gpio_pin(gpio->bind);

	unsigned char addr = (unsigned char)(ADDR_GPIOA + port->index);
	unsigned char regv = 0;

	mcp23s17_ReadByte(port->chip, addr, &regv);
	int yes = (regv & mask) ? 1 : 0;
	return yes;
}

static int gpio_config_hw(const gpio_t *gpio)
{
	const gpio_port_t *port = gpio_port(gpio->bind);
	int mask = gpio_pin(gpio->bind);
	const mcp23s17_t *chip = port->chip;
	int chip_addr = MCP23S17_ADDR(chip->option);

	sys_assert(port->chip != NULL); //gpio_mcp_init(..)?
	sys_assert(port->index >= 0); //must be PA or PB
	sys_assert(chip_addr >= 0);
	sys_assert(chip_addr <= 7);

	int out = 0, ipu = 0, lvl = 0;
	switch(gpio->mode) {
	case GPIO_DIN: out = 0; ipu = 0; break;
	case GPIO_IPU: out = 0; ipu = 1; break;
	case GPIO_PP0: out = 1; lvl = 0; break;
	case GPIO_PP1: out = 1; lvl = 1; break;
	default:
		sys_assert(1 == 0); //mode unsupported!
	}

	if(gpio_chip_reset_flag & (1 << chip_addr)) {}
	else {
		//default: all = input, ipu = yes
		mcp23s17_Init(chip);
		gpio_chip_reset_flag |= 1 << chip_addr;
	}

	//mcp reg access
	unsigned char addr;
	unsigned char regv = 0;

	//reg - olat
	addr = (unsigned char)(ADDR_OLATA + port->index);
	mcp23s17_ReadByte(chip, addr, &regv);
	regv &= ~ mask;
	if(lvl) regv |= mask;
	mcp23s17_WriteByte(chip, addr, regv);

	//reg - ipu
	addr = (unsigned char)(ADDR_GPPUA + port->index);
	mcp23s17_ReadByte(chip, addr, &regv);
	regv &= ~ mask;
	if(ipu) regv |= mask;
	mcp23s17_WriteByte(chip, addr, regv);

	//reg - iodir
	addr = (unsigned char)(ADDR_IODIRA + port->index);
	mcp23s17_ReadByte(chip, addr, &regv);
	regv &= ~ mask;
	if(!out) regv |= mask;
	mcp23s17_WriteByte(chip, addr, regv);
	return 0;
}

const gpio_drv_t gpio_mcp = {
	.name = "mcp",
	.config = gpio_config_hw,
	.set = gpio_set_hw,
	.get = gpio_get_hw,
};
