/*
*	sky@2011 initial version
*/
#include "drivers/sss_can.h"
#include "stdio.h"
#include <string.h>
#include "sis_can.h"
#include "flash.h"
#include "time.h"
#include "sis_decode.h"
#include "sss_card.h"
#include "drivers/sss_adc.h"
#include "nvm.h"
char sensor[7] = {
	1,//奇瑞EFS
	2,//奇瑞SIS
	3,//其他
	4,//学习
	5,//学习
	6,//学习
	7//学习
};

static const can_bus_t *can_bus;
extern int card_address;
can_msg_t can_msg0;
extern char notexist_flag;
char can_query[8] __nvm;
char can_data[8];
char query_default[8] = {0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00};

char can_int(void) 
{
	can_bus = &can1;
	can_cfg_t cfg={500000,0};    
	can_bus -> init(&cfg);
	return 0;
}

char can1_updata(void)
{
	if(!can_bus -> recv(&can_msg0)) {
		memcpy(can_data,can_msg0.data,can_msg0.dlc);
		if(can_data[1]==card_address) {  
			if(can_data[0]==1) {
				char i,flag=0;
				for(i=0;i<sensornum;i++) {
					if(can_data[2]==sensor[i]) {
						flag=1;
						memcpy(can_query,can_data,can_msg0.dlc);
						nvm_save();
						break;
					}
				}
				if(flag==0) {
					static can_msg_t msg={0x0001,8,{0xee,0xee,0xee,0xee,0xee,0xee,0xee,0xee},0};
					mdelay(5);
					can_send(&msg);
					mdelay(5);
				}
			}
			else if(can_data[0]==2) {   
				static can_msg_t msg;
				if(can_query[0]==0xff||can_query[0]==0){
					memcpy(msg.data,query_default,8);
				}
				else {
					memcpy(msg.data,can_query,8);
				}
				msg.id=0x0001;
				msg.dlc=4;     
				msg.flag=0;
				mdelay(5);
				can_send(&msg);	
				mdelay(5);
			}
			else if(can_data[0]==3) {
				if(can_data[2]==4) {     
					//simulator_poweroff();     
					sensor_study4();   
				}
				else if(can_data[2]==5) {     
					//simulator_poweroff();     
					sensor_study5();   
				}
				else if(can_data[2]==6) {     
					//simulator_poweroff();     
					sensor_study6(); 
				}
				else if(can_data[2]==7) {     
					//simulator_poweroff();     
					sensor_study7();   
				}
				else {
					static can_msg_t msg={0x0001,8,{0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa,0xaa},0};
					mdelay(5);
					can_send(&msg);
					mdelay(5);
				}
			}
		}
	}
	return 0;
}
