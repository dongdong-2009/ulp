#include "matrix.h"
#include "spi.h"
#include "config.h"
#include "time.h"
#include "stm32f10x.h"
#include <stdio.h>
#include <string.h>

#define BDT_PIN						GPIO_Pin_6
#define BDT							GPIO_ReadInputDataBit(GPIOC,BDT_PIN)
#define BOARD_SR_LoadEable()		GPIO_SetBits(GPIOC,GPIO_Pin_3)
#define BOARD_SR_LoadDisable()		GPIO_ResetBits(GPIOC,GPIO_Pin_3)
#define BOARD_SR_OutputEable()		GPIO_ResetBits(GPIOA,GPIO_Pin_0)
#define BOARD_SR_OutputDisable()	GPIO_SetBits(GPIOA,GPIO_Pin_0)

//Error define
#define ERROR_OK					0
#define ERROR_TIMEOUT				-1
#define ERROR_BOARD_NOT_EXIST		-2
#define ERROR_CHANNEL_NOT_EXIST		-3
#define ERROR_BUSSW_NOT_EXIST		-4
#define ERROR_CHSW_NOT_EXIST		-5

#define CONFIG_SLOT_NR				5

#define MATRIX_RELAY_OP_RELAYON		(1<<0)
#define MATRIX_RELAY_OP_RELAYOFF	(1<<1)
#define MATRIX_RELAY_OP_BUSSWON		(1<<2)
#define MATRIX_RELAY_OP_BUSSWOFF	(1<<3)
#define MATRIX_RELAY_OP_CHSWON		(1<<4)
#define MATRIX_RELAY_OP_CHSWOFF		(1<<5)

matrix_t chip1 = {
		.bus = &spi2,
		.idx = SPI_2_NSS,
};

//local varible define
bdInfo_t bdi[5];
int nboard=0;
unsigned char BoardBuf[48];

//local function define
static matrix_BoardSelect(int bd);
static int matrix_DisconnectAll (int bd);

int matrix_init()
{
	int i;
	unsigned short port_temp;

	GPIO_InitTypeDef  GPIO_InitStructure;
	//for matrix board select
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	//for bus select input
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6|GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	//for OE output
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//for lan reset input
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	//for shift register spi bus init
	spi_cfg_t cfg = {
		.cpol = 0,
		.cpha = 0,
		.bits = 8,
		.bseq = 1,
		.csel = 0,
		.freq = 4000000,
	};	chip1.bus->init(&cfg);
	for(i=0; i<CONFIG_SLOT_NR; i++){
		matrix_BoardSelect(i);
		mdelay(2);
		if(!BDT)
			nboard++;
	}

	if(nboard == 0){
		printf("Warning: Non board exist???\n");
		return ERROR_BOARD_NOT_EXIST;
	}
	//init each board information
	for(i = 0; i < CONFIG_SLOT_NR; i ++) {
		matrix_InitBoard(i, &bdi[i]);
		matrix_DisconnectAll();
	}
	//fpga_Set(OEN,bdt);
	GPIO_SetBits(GPIOA,GPIO_Pin_0);
	BOARD_SR_LoadDisable();			//shift register load disable
	return ERROR_OK;
}

int matrix_InitBoard(int bd, bdInfo_t * pInfo)
{
	if(nboard == 0){
		pInfo-> bExist = 0;
		return ERROR_BOARD_NOT_EXIST;
	}
	//board exist, detect board info through iic bus
	memset(pInfo,0,sizeof(bdInfo_t));
	pInfo-> bExist = 1;

	//iic access is not supported yet
	pInfo-> iLen = 48;
	pInfo-> iNrChannel = 46;
	pInfo-> iNrBus = 8;
	pInfo-> bBusSWExist = 1;
	pInfo-> bChSWExist = 0;
	return ERROR_OK;
}

static int matrix_DisconnectAll (int bd)
{
	int result;
	memset(BoardBuf, 0x00, 48);
	result = matrix_Update(bd);
	return result;
}

int matrix_Update(int bd) //bd: board nr 0~7
{
	int i;
	bdInfo_t * pInfo;

	pInfo = &bdi[bd];

	matrix_BoardSelect(bd);
	BOARD_SR_LoadEable();

	for(i = 46; i >= 0; i--)
		chip1.bus->wreg(chip1.idx, BoardBuf[i]);

	udelay(10);
	BOARD_SR_LoadDisable();
	BOARD_SR_OutputEable();

	return ERROR_OK;
}

void matrix_handler_setrelaystatus(unsigned char cmd,char *qdata)
{
	int bd;
	int ch;
	int bus;
	int i;
	unsigned char nr_of_ch;
	unsigned char row[256],col[256];
	unsigned char bd_to_update[CONFIG_SLOT_NR];

	//disconnect all?
	if(cmd == MATRIX_CMD_DISCONNECTALL)
	{
		matrix_DisconnectAll();
		return;
	}

	for(i=0; i< CONFIG_SLOT_NR; i++) bd_to_update[i] = 0;

	nr_of_ch = qdata[1];
	for(i=0; i<nr_of_ch; i++)
		row[i] = qdata[2+i];
	for(i=0; i<nr_of_ch; i++)
		col[i] = qdata[2+nr_of_ch+i];

	//sock_read(&sock,&nr_of_ch, 1);
	//sock_read(&sock,row,nr_of_ch);
	//sock_read(&sock,col,nr_of_ch);

#ifdef APP_VERBOSE
	printf("NR_OF_CH = %d \n", nr_of_ch);
	printf("<ROW|COL> = ");
	for(i=0; i<nr_of_ch; i++) printf("<%03d|%03d>  ", row[i],col[i]);
	printf("\n");
#endif /*APP_VERBOSE*/

	//verify the data received

	//implement
	for(i=0; i< nr_of_ch; i++)
	{
		matrix_map(row[i],col[i],&bd,&ch,&bus);
		bd_to_update[bd] = 1;
		if(cmd == MATRIX_CMD_CONNECT)
			matrix_ConnectImage(bd, ch, bus);
		else matrix_DisconnectImage(bd,ch,bus);
	}

	for(i = 0; i< CONFIG_SLOT_NR; i++)
	{
		if(bd_to_update[i]) matrix_Update(bd);
	}

	return;
}

void matrix_handler(unsigned char cmd,char *pdata)
{
	int len;

	len = sizeof(pdata);
	if (len <= 0) return;

#ifdef APP_VERBOSE
	printf("CMD = 0X%02X \n", cmd);
#endif /*APP_VERBOSE*/

	//implement the operation
	switch(cmd){
		case MATRIX_CMD_CONNECT:
		case MATRIX_CMD_DISCONNECT:
		case MATRIX_CMD_DISCONNECTALL:
			matrix_handler_setrelaystatus(cmd,pdata);
			break;
		default:
	}

	return;
}

int matrix_map(int vch, int vbus, int * bd, int* ch, int*bus)
{
	//we do not support relay brd with different size yet
	*bus = vbus;
	*bd = (int )(vch / 46);
	*ch = vch % 46;

	return ERROR_OK;
}

int matrix_ConnectImage(int bd, int ch, int bus)
{
	return matrix_SetRelayImage( \
		bd, ch, bus, \
		MATRIX_RELAY_OP_RELAYON| \
		MATRIX_RELAY_OP_BUSSWON| \
		MATRIX_RELAY_OP_CHSWON \
	);
}

int matrix_DisconnectImage(int bd, int ch, int bus)
{
	return matrix_SetRelayImage( \
		bd, ch, bus, \
		MATRIX_RELAY_OP_RELAYOFF \
	);
}

int matrix_SetRelayImage(int bd,int ch, int bus, int op)
{
	int result;
	bdInfo_t * pInfo;

	result= ERROR_OK;
	pInfo = &bdi[bd];

	//set relay image
	if(op&MATRIX_RELAY_OP_RELAYON)
		BoardBuf[ch] = 1 << bus;
	else if(op&MATRIX_RELAY_OP_RELAYOFF)
		BoardBuf[ch] = 0 << bus;
	if (pInfo->bBusSWExist == 1){
		if(op&MATRIX_RELAY_OP_BUSSWON)
			BoardBuf[46] = 1 << bus;
		else if(op&MATRIX_RELAY_OP_BUSSWOFF)
			BoardBuf[46] = 0 << bus;
	}
	if (pInfo->bChSWExist == 1){
		if(op&MATRIX_RELAY_OP_CHSWON)
			BoardBuf[47+(ch+7)/8] = ch%8;
		else if(op&MATRIX_RELAY_OP_CHSWOFF)
			BoardBuf[47+(ch+7)/8] = ~ch%8;
	}
	return result;
}

static matrix_BoardSelect(int bd)
{
	unsigned short port_temp;
	port_temp = GPIO_ReadOutputData(GPIOC);
	port_temp &= 0xfff8;
	port_temp |= bd;
	GPIO_Write(GPIOC, port_temp);		//chip select
}