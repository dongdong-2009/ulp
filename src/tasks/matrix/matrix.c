#include "matrix.h"
#include "spi.h"
#include "config.h"
#include "time.h"
#include "stm32f10x.h"
#include "mbi5025.h"
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

//matrix board operation define
#define MATRIX_RELAY_OP_RELAYON		(1<<0)
#define MATRIX_RELAY_OP_RELAYOFF	(1<<1)
#define MATRIX_RELAY_OP_BUSSWON		(1<<2)
#define MATRIX_RELAY_OP_BUSSWOFF	(1<<3)
#define MATRIX_RELAY_OP_CHSWON		(1<<4)
#define MATRIX_RELAY_OP_CHSWOFF		(1<<5)

static mbi5025_t sr = {
	.bus = &spi2,
	.idx = SPI_CS_DUMMY,
};

//local varible define
/* 
 * BoardBuf is for ShiftRegister image
 * BoardBuf[CONFIG_SLOT_NR][47] is for bus switch
 */
static unsigned char BoardBuf[CONFIG_SLOT_NR][47];
static bdInfo_t bdi[5];
static int nboard=0;
//support max_channel = 256
static unsigned char row[256];
static unsigned char col[256];

//local function define
static matrix_BoardSelect(int bd);
static int matrix_DisconnectAll (int bd);
static int matrix_InitBoard(int bd, bdInfo_t * pInfo);
static int matrix_Update(int bd);
static void matrix_handler_setrelaystatus(unsigned char cmd,char *qdata);
static int matrix_ConnectImage(int bd, int ch, int bus);
static int matrix_DisconnectImage(int bd, int ch, int bus);
static int matrix_SetRelayImage(int bd, int ch, int bus, int op);

int matrix_init()
{
	int i;

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
	mbi5025_Init(&sr);

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
		matrix_DisconnectAll(i);
	}

	//fpga_Set(OEN,bdt);
	//BOARD_SR_OutputDisable();
	BOARD_SR_LoadDisable();
	BOARD_SR_OutputEable();

	return ERROR_OK;
}

void matrix_handler(unsigned char cmd,char *pdata)
{
	int len;

	len = sizeof(pdata);
	if (len <= 0) return;

#if 1
	//print the operation for debug
	switch(cmd){
		case MATRIX_CMD_CONNECT:
			printf("\nCMD = MATRIX_CMD_CONNECT! \n");
			break;
		case MATRIX_CMD_DISCONNECT:
			printf("\nCMD = MATRIX_CMD_DISCONNECT! \n");
			break;
		case MATRIX_CMD_DISCONNECTALL:
			printf("\nCMD = MATRIX_CMD_DISCONNECTALL! \n");
			break;
		default:
			break;
	}
#endif

	//implement the operation
	switch(cmd){
		case MATRIX_CMD_CONNECT:
		case MATRIX_CMD_DISCONNECT:
		case MATRIX_CMD_DISCONNECTALL:
			matrix_handler_setrelaystatus(cmd,pdata);
			break;
		default:
			break;
	}

	return;
}

//bd: board nr 0~7
static int matrix_Update(int bd)
{
	int i;

	matrix_BoardSelect(bd);

	//shift register load data, don't support SPI DMA mode
	//BoardBuf[bd][46] means bus switch status, 1:on, 0:off
	for(i = 46; i >= 0; i--)
		mbi5025_WriteByte(&sr, BoardBuf[bd][i]);
	BOARD_SR_LoadEable();
	BOARD_SR_LoadDisable();

	// BOARD_SR_OutputEable();
	// udelay(10);
	// BOARD_SR_OutputDisable();

	return ERROR_OK;
}

static matrix_BoardSelect(int bd)
{
	unsigned short port_temp;
	port_temp = GPIO_ReadOutputData(GPIOC);
	port_temp &= 0xfff8;
	port_temp |= bd;
	GPIO_Write(GPIOC, port_temp);		//chip select
}

static int matrix_InitBoard(int bd, bdInfo_t * pInfo)
{
	if(nboard == 0){
		pInfo-> bExist = 0;
		return ERROR_BOARD_NOT_EXIST;
	}
	//if board exists, detect board info through iic bus
	//iic access is not supported yet
	
	memset(pInfo,0,sizeof(bdInfo_t));
	pInfo-> bExist = 1;
	pInfo-> iLen = 47;
	pInfo-> iNrChannel = 46;
	pInfo-> iNrBus = 8;
	pInfo-> bBusSWExist = 1;
	pInfo-> bChSWExist = 0;
	return ERROR_OK;
}

/*
	This function will disconnect all of the channels from all of the buses.
	This will happen on all of the boards in the DigESwitch box.
	The disconnections include every channel from all eight on board buses
	and the eight external bus/chan pins from the on board buses.
	int bdNum:
	< 0 - Update all boards in the DigESwitch box
          = 0 to 7
*/
static int matrix_DisconnectAll(int bd)
{
	bdInfo_t * pInfo;
	int result;
	int cbd;

	for (cbd = 0; cbd < CONFIG_SLOT_NR; cbd++) {
		pInfo = &bdi[cbd];
		if(pInfo->bExist != 1) 
			continue;

		if ((bd < 0) || (cbd == bd)) {
			memset(BoardBuf[cbd], 0x00, pInfo->iLen);
			result = matrix_Update(cbd);
			if(result)
				return result;
		}
	}

	return result;
}

static void matrix_handler_setrelaystatus(unsigned char cmd,char *qdata)
{
	int bd;
	int ch;
	int bus;
	int i;
	unsigned char nr_of_ch;
	unsigned char bd_to_update[CONFIG_SLOT_NR];

	//disconnect all?
	if (cmd == MATRIX_CMD_DISCONNECTALL) {
		matrix_DisconnectAll(-1);
		return;
	}

	//init, no board need update
	memset(bd_to_update, 0, CONFIG_SLOT_NR);

	nr_of_ch = qdata[1];
	for (i = 0; i < nr_of_ch; i ++) {
		row[i] = qdata[2 + i];
		col[i] = qdata[2 + nr_of_ch + i];
	}

#if 1
	//print the config information
	printf("NR_OF_CH = %d \n", nr_of_ch);
	printf("<ROW|COL> = ");
	for(i=0; i<nr_of_ch; i++) 
		printf("<%03d|%03d>  ", row[i],col[i]);
	printf("\n");
#endif

	//implement
	for (i = 0; i < nr_of_ch; i++) {
		//matrix map calculate,we do not support relay brd with different size yet
		bus = row[i];
		bd = (int )(col[i] / 46);
		ch = col[i] % 46;
		bd_to_update[bd] = 1;
		if(cmd == MATRIX_CMD_CONNECT)
			matrix_ConnectImage(bd, ch, bus);
		else 
			matrix_DisconnectImage(bd, ch, bus);
	}

	for(i = 0; i< CONFIG_SLOT_NR; i++)
	{
		if(bd_to_update[i])
			matrix_Update(i);
	}

	return;
}

static int matrix_ConnectImage(int bd, int ch, int bus)
{
	return matrix_SetRelayImage( \
		bd, ch, bus, \
		MATRIX_RELAY_OP_RELAYON| \
		MATRIX_RELAY_OP_BUSSWON| \
		MATRIX_RELAY_OP_CHSWON \
	);
}

static int matrix_DisconnectImage(int bd, int ch, int bus)
{
	return matrix_SetRelayImage( \
		bd, ch, bus, \
		MATRIX_RELAY_OP_RELAYOFF \
	);
}

static int matrix_SetRelayImage(int bd, int ch, int bus, int op)
{
	int result;
	bdInfo_t * pInfo;

	result= ERROR_OK;
	pInfo = &bdi[bd];

	//set relay image
	if (op & MATRIX_RELAY_OP_RELAYON)
		BoardBuf[bd][ch] |= 1 << bus;
	else if(op & MATRIX_RELAY_OP_RELAYOFF)
		BoardBuf[bd][ch] &= ~(1 << bus);

	if (pInfo -> bBusSWExist == 1) {
		if(op & MATRIX_RELAY_OP_BUSSWON)
			BoardBuf[bd][pInfo->iNrChannel] |= 1 << bus;
		else if(op & MATRIX_RELAY_OP_BUSSWOFF)
			BoardBuf[bd][pInfo->iNrChannel] &= ~(1 << bus);
	}

	return result;
}


#if 1
#include "shell/cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int cmd_matrix_func(int argc, char *argv[])
{
	int temp = 0;
	int i;
	char simulator_data[4];

	const char * usage = { \
		" usage:\n" \
		" matrix con nbus nch, matrix connect a point \n" \
		" matrix dis nbus nch, matrix disconnect a point \n" \
		" matrix on nboard, connect all relays on nboard  \n" \
		" matrix off nboard, disconnect all relays on nboard \n" \
	};

	if (argc > 1) {
		if(argv[1][0] == 'c') {
			simulator_data[0] = (char)MATRIX_CMD_CONNECT;
			simulator_data[1] = 1;
			sscanf(argv[2], "%d", &temp);
			simulator_data[2] = (char)temp;
			sscanf(argv[3], "%d", &temp);
			simulator_data[3] = (char)temp;
			matrix_handler(MATRIX_CMD_CONNECT, simulator_data);
		}

		if(argv[1][0] == 'd') {
			simulator_data[0] = (char)MATRIX_CMD_DISCONNECT;
			simulator_data[1] = 1;
			sscanf(argv[2], "%d", &temp);
			simulator_data[2] = (char)temp;
			sscanf(argv[3], "%d", &temp);
			simulator_data[3] = (char)temp;
			matrix_handler(MATRIX_CMD_DISCONNECT, simulator_data);
		}

		if(argv[1][1] == 'n') {
			sscanf(argv[2], "%d", &temp);
			matrix_BoardSelect(temp);
			//shift register load data, don't support SPI DMA mode
			//BoardBuf[bd][46] means bus switch status, 1:on, 0:off
			for(i = 46; i >= 0; i--)
				mbi5025_WriteByte(&sr, 0xff);
			BOARD_SR_LoadEable();
			BOARD_SR_LoadDisable();
		}

		if(argv[1][1] == 'f') {
			sscanf(argv[2], "%d", &temp);
			matrix_BoardSelect(temp);
			//shift register load data, don't support SPI DMA mode
			//BoardBuf[bd][46] means bus switch status, 1:on, 0:off
			for(i = 46; i >= 0; i--)
				mbi5025_WriteByte(&sr, 0x00);
			BOARD_SR_LoadEable();
			BOARD_SR_LoadDisable();
		}
	}

	if(argc < 2) {
		printf(usage);
		return 0;
	}

	return 0;
}
const cmd_t cmd_matrix = {"matrix", cmd_matrix_func, "matrix board cmd"};
DECLARE_SHELL_CMD(cmd_matrix)
#endif

