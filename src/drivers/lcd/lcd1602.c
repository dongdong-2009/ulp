/*
 *  dusk@2010 initial version
 *  david@2012 modified
 */

#include "ulp/driver.h"
#include "lcd.h"
#include "lcd1602.h"
#include "ulp_time.h"
#include <string.h>
#include "lpt.h"

static lpt_bus_t *lcd_bus;
static char lcd1602_fail;

static lcd1602_status lcd1602_ReadStaus(void)
{
	lcd1602_status status;
	int value = lcd_bus -> read(STA);
	status = (value & 0x0080) ? Bit_Busy : Bit_Ok;
	return status;
}

static int lcd1602_wait(void)
{
	int ret = -1;
	time_t ms = time_get(10); //timeout = 10mS
	while(lcd1602_fail == 0) {
		if(!lcd1602_ReadStaus()) {
			ret = 0;
			break;
		}
		if(time_left(ms) < 0) {
			lcd1602_fail = 1;
			break;
		}
	}
	return ret;
}

static void lcd1602_WriteCommand(char command)
{
	lcd_bus -> write(CMD, command);
}

static void lcd1602_WriteData(char data)
{
	lcd_bus -> write(DIN, data);
}

int lcd1602_Init(const struct lcd_cfg_s *cfg)
{
	mdelay(15);
	lcd1602_WriteCommand(LCD1602_COMMAND_SETMODE);
	mdelay(5);
	lcd1602_WriteCommand(LCD1602_COMMAND_SETMODE);
	mdelay(5);
	lcd1602_WriteCommand(LCD1602_COMMAND_SETMODE);
	
	//check the busy bit
	lcd1602_wait();
	lcd1602_WriteCommand(LCD1602_COMMAND_SETMODE);		//set mode
	//check the busy bit
	lcd1602_wait();
	lcd1602_WriteCommand(LCD1602_COMMAND_OFFSCREEN);	//turn off screen
	//check the busy bit
	lcd1602_wait();
	lcd1602_WriteCommand(LCD1602_COMMAND_CLRSCREEN);	//clear screeen
	//check the busy bit
	lcd1602_wait();
	lcd1602_WriteCommand(LCD1602_COMMAND_SETPTMOVE);	//set lcd1602 ram pointer
	//check the busy bit
	lcd1602_wait();
	lcd1602_WriteCommand(LCD1602_COMMAND_SETCUSOR);		//set screen display
	
	return 0;
}

int lcd1602_WriteChar(int row,int column,char ch)
{
	char i=0;

	i = row ? (0xc0 + column):(0x80 + column);
	lcd1602_wait();    //check the busy bit
	lcd1602_WriteCommand(i);
	lcd1602_wait();    //check the busy bit
	lcd1602_WriteData(ch);
	return 0;
}

int lcd1602_WriteString(int column, int row, const char *s)
{
	char i=0,size;
	size = (char)strlen(s);
#if 0
	for( i = 0 ; i < size ; i++){
		lcd1602_WriteChar(row, column + i, *(s++));
	}
#endif

#if 1
	i = row ? (0xc0 + column):(0x80 + column);
	//check the busy bit
	lcd1602_wait();
	lcd1602_WriteCommand(i);
	for( i = 0 ; i < size ; i++){
		//check the busy bit
		lcd1602_wait();
		lcd1602_WriteData(*s ++);
	}
#endif

	return 0;
}

int lcd1602_ClearScreen(void)
{
	//check the busy bit
	lcd1602_wait();
	lcd1602_WriteCommand(LCD1602_COMMAND_CLRSCREEN);	//clear screeen

	return 0;
}

static const struct lcd_dev_s lcd1602 = {
	.xres = 16,
	.yres = 2,
	.init = lcd1602_Init,
	.puts = lcd1602_WriteString,

	.setwindow = NULL,
	.rgram = NULL,
	.wgram = NULL,

	.writereg = NULL,
	.readreg = NULL,
};

static void lcd1602_reg(void)
{
	lcd_add(&lcd1602, "LCD1602", LCD_TYPE_CHAR);
}

driver_init(lcd1602_reg);
