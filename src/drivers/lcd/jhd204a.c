/* jhd204a.c
 * David@2011,8 initial version
 */

#include "driver.h"
#include "lcd.h"
#include "jhd204a.h"
#include "time.h"
#include <string.h>
#include "lpt.h"

static lpt_bus_t *lcd_bus;

static jhd204a_status jhd204a_ReadStaus(void)
{
	int value = lcd_bus -> read(STA);
	jhd204a_status status;
	
	status = (value & 0x80) ? Bit_Busy : Bit_Ok;
	return status;
}

static char jhd204a_fail;
static int jhd204a_wait(void)
{
	int ret = -1;
	time_t ms = time_get(10); //timeout = 10mS
	
	while(jhd204a_fail == 0) {
		if(!jhd204a_ReadStaus()) {
			ret = 0;
			break;
		}
		
		if(time_left(ms) < 0) {
			jhd204a_fail = 1;
			break;
		}
	}
	
	return ret;
}

static void jhd204a_WriteCommand(char command)
{
	lcd_bus -> write(CMD, command);
}

static void jhd204a_WriteData(char data)
{
	lcd_bus -> write(DIN, data);
}

int jhd204a_Init(void)
{
	mdelay(15);
	jhd204a_WriteCommand(JHD204A_COMMAND_SETFUNC);
	mdelay(5);
	jhd204a_WriteCommand(JHD204A_COMMAND_SETFUNC);
	mdelay(1);
	jhd204a_WriteCommand(JHD204A_COMMAND_SETFUNC);

	jhd204a_WriteCommand(JHD204A_COMMAND_SETFUNC);
	jhd204a_wait();
	jhd204a_WriteCommand(JHD204A_COMMAND_OFFSCREEN);	//turn off screen
	//check the busy bit
	jhd204a_wait();
	jhd204a_WriteCommand(JHD204A_COMMAND_CLRSCREEN);	//clear screeen
	//check the busy bit
	jhd204a_wait();
	jhd204a_WriteCommand(jhd204a_COMMAND_SETPTMOVE);	//set jhd204a ram pointer
	//check the busy bit
	jhd204a_wait();
	jhd204a_WriteCommand(JHD204A_COMMAND_ONSCREEN);		//set screen display	jhd204a_wait();
	return 0;
}

int jhd204a_WriteChar(int row,int column,char ch)
{
	char i=0;

	switch (row) {
		case 0:
			i = 0x80 + column;
			break;
		case 1:
			i = 0xa0 + column;
			break;
		case 2:
			i = 0xc0 + column;
			break;
		case 3:
			i = 0xe0 + column;
			break;
		default:
			break;
	}

	//check the busy bit
	jhd204a_wait();
	jhd204a_WriteCommand(i);

	//check the busy bit
	jhd204a_wait();
	jhd204a_WriteData(ch);

	return 0;
}

int jhd204a_WriteString(int column, int row, const char *s)
{
	char i=0,size;
	size = (char)strlen(s);

#if 0
	for( i = 0 ; i < size ; i++){
		jhd204a_WriteChar(row, column + i, *(s++));
	}
#endif

#if 1
	switch (row) {
		case 0:
			i = 0x80 + column;
			break;
		case 1:
			i = 0xa0 + column;
			break;
		case 2:
			i = 0xc0 + column;
			break;
		case 3:
			i = 0xe0 + column;
			break;
		default:
			break;
	}

	//check the busy bit
	jhd204a_wait();
	jhd204a_WriteCommand(i);

	for( i = 0 ; i < size ; i++){
		//check the busy bit
		jhd204a_wait();
		jhd204a_WriteData(*s ++);
	}
#endif

	return 0;
}

int jhd204a_ClearScreen(void)
{
	//check the busy bit
	jhd204a_wait();
	jhd204a_WriteCommand(JHD204A_COMMAND_CLRSCREEN);	//clear screeen

	return 0;
}

static const lcd_t jhd204a = {
	.w = 20,
	.h = 4,
	.init = jhd204a_Init,
	.puts = jhd204a_WriteString,
	.clear_all = jhd204a_ClearScreen,
	.clear_rect = NULL,
	.scroll = NULL,
};

static void jhd204a_reg(void)
{
	lcd_bus = &lpt;
	lcd_bus->init();
	lcd_add(&jhd204a);
}
driver_init(jhd204a_reg);
