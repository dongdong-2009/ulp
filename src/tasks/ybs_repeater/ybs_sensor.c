/*
 * 	king@2012 initial version
 */

#include "ybs_sensor.h"
#include "ybs_repeater.h"
#include <string.h>

ybs_sensor_t sensor[SENSORNUMBER];

static void com_choice_init()
{
	GPIO_SetDir(1, MUX_CHANNEL | MUX_ENABLE, 1);		//set p1[18-21] as output
	LPC_GPIO1->SET = MUX_ENABLE;				//disable multiplexer
}

static void com_choice(uint32_t com)
{
	com = com << 18;
	LPC_GPIO1->SET = MUX_ENABLE;				//disable multiplexer
	LPC_GPIO1->PIN = ((LPC_GPIO1->PIN) & (~(7 << 18))) | com ;
	LPC_GPIO1->CLR = MUX_ENABLE;				//enable multiplexer
}

static void com_init()
{
	// UART Configuration structure variable
	UART_CFG_Type UARTConfigStruct;
	// UART FIFO configuration Struct variable
	UART_FIFO_CFG_Type UARTFIFOConfigStruct;

	/*
	 * Initialize UART2 pin connect
	 * P0.10: U2_TXD
	 * P0.11: U2_RXD
	 */
	PINSEL_ConfigPin(0,10,1);
	PINSEL_ConfigPin(0,11,1);
	/*
	 * Initialize UART3 pin connect
	 * P0.0: U3_TXD
	 * P0.1: U3_RXD
	 */
	PINSEL_ConfigPin(0,0,2);
	PINSEL_ConfigPin(0,1,2);

	UARTConfigStruct.Baud_rate = 115200;
	UARTConfigStruct.Databits = UART_DATABIT_8;
	UARTConfigStruct.Parity = UART_PARITY_NONE;
	UARTConfigStruct.Stopbits = UART_STOPBIT_1;
	UART_Init(LPC_UART2, &UARTConfigStruct);
	UART_Init(LPC_UART3, &UARTConfigStruct);

	UART_FIFOConfigStructInit(&UARTFIFOConfigStruct);
	UART_FIFOConfig(LPC_UART2, &UARTFIFOConfigStruct);
	UART_FIFOConfig(LPC_UART3, &UARTFIFOConfigStruct);

	UART_TxCmd(LPC_UART2, ENABLE);
	UART_TxCmd(LPC_UART3, ENABLE);
}

void sensor_init()
{
	com_choice_init();
	com_init();
}

void read_sensor(uint32_t com, uint16_t *bank0, uint16_t *bank1)
{
	com_choice(com);
	LPC_UART2->FCR |= 4;
	LPC_UART3->FCR |= 4;	//clear txbuf
	if(LPC_UART2->LSR & UART_LSR_RDR) {
		*bank0 = (uint16_t)(LPC_UART2->RBR & UART_RBR_MASKBIT);
		*bank0 = (*bank0 << 8) + (LPC_UART2->RBR & UART_RBR_MASKBIT);
	}
	else {
		*bank0 = 0x88;
	}
	if(LPC_UART3->LSR & UART_LSR_RDR) {
		*bank1 = (uint16_t)(LPC_UART3->RBR & UART_RBR_MASKBIT);
		*bank1 = (*bank1 << 8) + (LPC_UART3->RBR & UART_RBR_MASKBIT);
	}
	else {
		*bank1 = 0x88;
	}
}

void write_sensor(uint32_t com, uint8_t cmd)
{
	com_choice(com);
	LPC_UART2->FCR |= 2;
	LPC_UART3->FCR |= 2;	//clear rxbuf
	LPC_UART2->THR = cmd & UART_THR_MASKBIT;
	LPC_UART3->THR = cmd & UART_THR_MASKBIT;
}

void analize_sensor(uint32_t com, uint16_t data)
{
	if(data > (1 << 15)) {
		data = (1 << 15);
	}
	if(data & (1 << 15)) {
		if(sensor[com].noresponse_cnt + 1 > NORESPONSECNT) {
			sensor[com].status = sensor[com].status | (1 << 4);
			sensor[com].noresponse_cnt = NORESPONSECNT;
		}
		else {
			sensor[com].noresponse_cnt++;
		}
	}
	else {
		if(sensor[com].noresponse_cnt) {
			sensor[com].noresponse_cnt--;
		}
		if(!sensor[com].noresponse_cnt) {
			sensor[com].status = sensor[com].status & ~(1 << 4);
		}

		if(!sensor[com].average) {
			sensor[com].average = data;
			sensor[com].high_average = data;
			sensor[com].low_average = data;
			sensor[com].high_value = data;
			sensor[com].low_value = data;
		}
		sensor[com].average = (uint16_t)(((uint32_t)sensor[com].average * 99 + (uint32_t)data) / 100);

		if(data > sensor[com].high_value) {
			sensor[com].high_value = data;
		}
		else if(data < sensor[com].low_value) {
			sensor[com].low_value = data;
		}

		if(sensor[com].average > sensor[com].high_average) {
			sensor[com].high_average = sensor[com].average;
		}
		else if(sensor[com].average < sensor[com].low_average) {
			sensor[com].low_average = sensor[com].average;
		}

		if(sensor[com].low_cut > sensor[com].average) {
			sensor[com].status = (sensor[com].status | (1 << 5)) & ~(1 << 1);
			sensor[com].alarm_cnt = 0;
			sensor[com].cut_cnt = 0;
		}
		else if(sensor[com].high_cut > (1 << 15) - sensor[com].average) {
			sensor[com].status = (sensor[com].status | (1 << 5)) & ~(1 << 1);
			sensor[com].alarm_cnt = 0;
			sensor[com].cut_cnt = 0;
		}
	}


	if(sensor[com].status & (1 << 0)) {
		if(sensor[com].status & (1 << 1)) {
			if((data < (sensor[com].average + sensor[com].high_alarm)) && (data > (sensor[com].average + sensor[com].low_alarm))) {
				if(sensor[com].alarm_cnt) {
					sensor[com].alarm_cnt--;
				}
				if(sensor[com].cut_cnt) {
					sensor[com].cut_cnt--;
				}
			}
			else {
				sensor[com].alarm_cnt++;
				if(sensor[com].alarm_cnt > ALARMCNT && !(sensor[com].status & (1 << 2))) {
					sensor[com].status = sensor[com].status | (3 << 2);
					uint32_t temp1 = sensor[com].alarmdata.data_now & ~(0xffffffff << ALARMDATANUMBERSHIFT);
					RTC_GetFullTime (LPC_RTC, &(sensor[com].alarmdata.data[temp1].time));
					sensor[com].alarmdata.data[temp1].backward_data_now = 0;
					sensor[com].alarmdata.data[temp1].forward_data_now = ALARMDATASIZE >> 1;
					sensor[com].normaldata.record_data_now = sensor[com].normaldata.data_now & ~(0xffffffff << ALARMDATANUMBERSHIFT);;
					sensor[com].alarm++;
				}
				if((data < (sensor[com].average + sensor[com].high_cut)) && (data > (sensor[com].average + sensor[com].low_cut))) {
					if(sensor[com].cut_cnt) {
						sensor[com].cut_cnt--;
					}
				}
				else {
					sensor[com].cut_cnt++;
					if(sensor[com].cut_cnt > CUTCNT) {
						//cut cut cut cut cut cut cut cut cut cut cut cut cut cut 
						sensor[com].status = sensor[com].status & ~(1 << 0);
						sensor[com].cut++;
					}
				}
			}
		}
		else {
			sensor[com].alarm_cnt = 0;
			sensor[com].cut_cnt = 0;
		}
	}
	else {
		sensor[com].alarm_cnt = 0;
		sensor[com].cut_cnt = 0;
	}

	if(sensor[com].status & (1 << 3)) {
		uint32_t temp2 = sensor[com].alarmdata.data_now & ~(0xffffffff << ALARMDATANUMBERSHIFT);
		if(sensor[com].alarmdata.data[temp2].forward_data_now >= ALARMDATASIZE) {
			sensor[com].alarmdata.data[temp2].forward_data_now = ALARMDATASIZE >> 1;
			sensor[com].status = sensor[com].status & ~(1 << 3);
		}
		else {
			uint32_t temp3 = sensor[com].alarmdata.data[temp2].forward_data_now;
			sensor[com].alarmdata.data[temp2].data[temp3] = data;
			sensor[com].alarmdata.data[temp2].forward_data_now++;
		}
	
	}
}

void sensor_update()
{
	for(uint32_t i = 0; i < SENSORNUMBER; i++) {
		if(sensor[i].status & (1 << 2)) {
			uint32_t temp1 = sensor[i].alarmdata.data_now & ~(0xffffffff << ALARMDATANUMBERSHIFT);
			uint32_t temp2 = sensor[i].normaldata.record_data_now & ~(0xffffffff << NORMALDATASIZESHIFT);
			if(temp2 < (ALARMDATASIZE >> 1) - 1) {
				uint32_t temp3 = (ALARMDATASIZE >> 1) -1 - temp2;
				memcpy(sensor[i].alarmdata.data[temp1].data, &sensor[i].normaldata.data[NORMALDATASIZE - temp3], temp3 << 1);
				memcpy(&sensor[i].alarmdata.data[temp1].data[temp3], sensor[i].normaldata.data, (NORMALDATASIZE - temp3) << 1);
			}
			else {
				memcpy(sensor[i].alarmdata.data[temp1].data, &sensor[i].normaldata.data[temp2 + 1 - (ALARMDATASIZE >> 1)], ALARMDATASIZE);
			}
			sensor[i].status = sensor[i].status & ~(1 << 2);
		}
	}
}

