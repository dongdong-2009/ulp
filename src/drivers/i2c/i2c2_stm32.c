/*
*	miaofng@2010 initial version
*/

#include "config.h"
#include "stm32f10x.h"
#include "i2c.h"
#include "device.h"

static int i2c_Init(const i2c_cfg_t *i2c_cfg)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	I2C_InitTypeDef  I2C_InitStructure;

	/* pin map:	I2C2		WaitStandByState
		SCL		PB7			PB11
		SDA		PB6			PB10
	*/
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);   

	/* Configure pins: SCL and SDA */
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_10 | GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
  
	/* I2C configuration */
	I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
	I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
	//I2C_InitStructure.I2C_OwnAddress1 = I2C_SLAVE_ADDRESS7;
	//I2C_InitStructure.I2C_OwnAddress1 = 0xA0;
	I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
	I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	I2C_InitStructure.I2C_ClockSpeed = i2c_cfg->speed;
  
	I2C_Cmd(I2C2, ENABLE);
	I2C_Init(I2C2, &I2C_InitStructure);

	return 0;
}

static int i2c_WriteByte(unsigned char chip, unsigned addr, int alen, unsigned char *buffer)
{
	/* While the bus is busy */
	while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY));

	I2C_GenerateSTART(I2C2, ENABLE);
	while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT));  
	I2C_Send7bitAddress(I2C2, chip, I2C_Direction_Transmitter);
	while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

	if (alen == 1) {
		/* Send internal address to write to : only one byte Address */
		I2C_SendData(I2C2, addr);
	} else if (alen == 2) {
		/* Send the internal address to write to : MSB of the address first */
		I2C_SendData(I2C2, (unsigned char)((addr & 0xFF00) >> 8));
		/* Test on EV8 and clear it */
		while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
  		/* Send the EEPROM's internal address to write to : LSB of the address */
		I2C_SendData(I2C2, (unsigned char)(addr & 0x00FF));
	} else
		return 1;

	while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
	I2C_SendData(I2C2, *buffer); 
	while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

	I2C_GenerateSTOP(I2C2, ENABLE);

	return 0;
}
static int i2c_WriteBuffer(unsigned char chip, unsigned addr, int alen, unsigned char *buffer, int len)
{
	/* While the bus is busy */
	while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY));

	I2C_GenerateSTART(I2C2, ENABLE);
	while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT)); 
	I2C_Send7bitAddress(I2C2, chip, I2C_Direction_Transmitter);
	while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));  

	if (alen == 1) {
		/* Send internal address to write to : only one byte Address */
		I2C_SendData(I2C2, addr);
	} else if (alen == 2) {
		/* Send the internal address to write to : MSB of the address first */
		I2C_SendData(I2C2, (unsigned char)((addr & 0xFF00) >> 8));
		/* Test on EV8 and clear it */
		while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
  		/* Send the internal address to write to : LSB of the address */
		I2C_SendData(I2C2, (unsigned char)(addr & 0x00FF));
	} else
		return 1;

	while(! I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED));

	while (len--) {
		I2C_SendData(I2C2, *buffer); 
		buffer++;  
		while (!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
	}

	I2C_GenerateSTOP(I2C2, ENABLE);

	return 0;
}

static int i2c_ReadBuffer(unsigned char chip, unsigned addr, int alen, unsigned char *buffer, int len)
{
	/* While the bus is busy */
	while(I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY));

	I2C_GenerateSTART(I2C2, ENABLE);
	while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT));
	I2C_Send7bitAddress(I2C2, chip, I2C_Direction_Transmitter);
	while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED));

	if (alen == 1) {
		/* Send the internal address to write to : MSB of the address first */
		I2C_SendData(I2C2, addr);
	} else if (alen == 2) {
		/* Send the internal address to write to : MSB of the address first */
		I2C_SendData(I2C2, (unsigned char)((addr & 0xFF00) >> 8));
		while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
  		/* Send the internal address to write to : LSB of the address */
		I2C_SendData(I2C2, (unsigned char)(addr & 0x00FF));
	} else
		return 1;

	while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED));
	I2C_GenerateSTART(I2C2, ENABLE);
	while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT));
	I2C_Send7bitAddress(I2C2, chip, I2C_Direction_Receiver);
	while(!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED));

	while (len) {
		if (len == 1) {
			I2C_AcknowledgeConfig(I2C2, DISABLE);
			I2C_GenerateSTOP(I2C2, ENABLE);
		}

		if (I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_RECEIVED)) {      
			*buffer = I2C_ReceiveData(I2C2);
			buffer++;
			len--;
		}
	}

	I2C_AcknowledgeConfig(I2C2, ENABLE);

	return 0;
}

static int i2c_WaitStandByState(unsigned char chip)
{
	volatile unsigned short SR1_Tmp = 0;
	do {
		/* Send START condition */
		I2C_GenerateSTART(I2C2, ENABLE);

		/* Read I2C_EE SR1 register to clear pending flags */
		SR1_Tmp = I2C_ReadRegister(I2C2, I2C_Register_SR1);

		/* Send EEPROM address for write */
		I2C_Send7bitAddress(I2C2, chip, I2C_Direction_Transmitter);

	} while (!(I2C_ReadRegister(I2C2, I2C_Register_SR1) & 0x0002));

	I2C_ClearFlag(I2C2, I2C_FLAG_AF);
	I2C_GenerateSTOP(I2C2, ENABLE);

	return 0;
}

i2c_bus_t i2c2 = {
	.init = i2c_Init,
	.wreg = i2c_WriteByte,
	.rreg = NULL,
	.wbuf = i2c_WriteBuffer,
	.rbuf = i2c_ReadBuffer,
	.WaitStandByState = i2c_WaitStandByState,
};
