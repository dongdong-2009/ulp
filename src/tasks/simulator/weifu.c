/*
 * david@2012 initial version
 * note: there are two clock reference
 * 1, axle_clock_cnt, its' range is (0, 120 * 16)
 * 2, op_clock_cnt,   its' range is (0, 37 * 52)
 * and both range of them is approximate equal
 *
 */

///7 10 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "stm32f10x.h"
#include "shell/cmd.h"
#include "config.h"
#include "sys/task.h"
#include "ulp_time.h"
#include "priv/mcamos.h"
#include "ad9833.h"
#include "mcp41x.h"
#include "sim_driver.h"
#include "nvm.h"
#include "wave.h"
#include "weifu_lcm.h"

#define WEIFU_DEBUG 1
#if WEIFU_DEBUG
#define DEBUG_TRACE(args)    (printf args)
#else
#define DEBUG_TRACE(args)
#endif

//define sample points for axle and op
#define SAMPLE_DENSITY_HIGH  1
#if SAMPLE_DENSITY_HIGH
#define AXLE_SAMPLE_POINTS  32
#define OP_SAMPLE_POINTS    104
#define POINTS  3840
#else
#define AXLE_SAMPLE_POINTS  16
#define OP_SAMPLE_POINTS    52
#endif

//define two wave output support
#define WEIFU_SUPPORT_TWO_WAVE  1
enum {
	OP_WAVE_MODE_37X,
	OP_WAVE_MODE_120X,
};
static int op_mode = OP_WAVE_MODE_37X;
//op and eng rpm wave cofficient define
// (0 - 500)=0.3, (500 - 1000) = 0.5
// (1000 - 1500) = 0.7, (1500 - 5000) = 1
#define WAVE_FACTOR_3 3
#define WAVE_FACTOR_5 5
#define WAVE_FACTOR_7 7
#define WAVE_FACTOR_10 10
#define DAC_1 (volatile unsigned)(DAC_BASE + (unsigned)0x00000008 + DAC_Align_12b_R)
#define DAC_2 (volatile unsigned)(DAC_BASE + (unsigned)0x00000014 + DAC_Align_12b_R)
#define axle_min1 58*32
#define axle_max1 59*32
#define axle_min2 118*32
#define axle_max2 119*32

#define op_min1 28*32
#define op_max1 29*32
#define op_min2 58*32
#define op_max2 59*32
#define op_min3 88*32
#define op_max3 89*32
#define op_min4 118*32
#define op_max4 119*32
// define the time clock reference
static int axle_clock_cnt = 0;  //for axle gear
static int op_clock_cnt = 0;    //for oil pump gear
static int op_clock_cnt_1 = 0;
static int op_tri_flag = 1;

//for engine axle gear singal
static int axle_cnt, op_cnt,op_cnt_1;
static int prev_axle_cnt, prev_op_cnt;
static volatile int eng_factor, op_factor;
static volatile int eng_rpm, op_rpm, phase_diff, vss, hfmsig, hfmref;
static int tim_dc, tim_frq;
static int temp,temp_1,temp1,temp2,temp3, result = 0;

static int diff;
unsigned char result_axle[120];
unsigned char result_op_37[120];
unsigned char result_op_120[120];
int index_result=0;
static unsigned short gear32_120[3840];
static unsigned short op32_120[3840];

unsigned short *s;

//timer define for eng_speed and wtout
static time_t eng_speed_timer;
static time_t wtout_timer;

//for mcamos defines, com between lcm and core board
static struct mcamos_s lcm = {
	.can = &can1,
	.baud = 500000,
	.id_cmd = MCAMOS_MSG_CMD_ID,
	.id_dat = MCAMOS_MSG_DAT_ID,
	.timeout = 50,
};
static lcm_dat_t cfg_data;

//private functions
static int weifu_ConfigData(void);

void weifu_Init(void)
{
	clock_Init();
	axle_Init(0);
	op_Init(0);
	vss_Init();
	wss_Init();
	pwmin1_Init();
	counter2_Init();
	eng_speed_timer = time_get(1000);
	wtout_timer = time_get(1000);

	mcamos_init_ex(&lcm);
	cfg_data.eng_speed = 0;
	cfg_data.wtout = 0;
	phase_diff = 2;
	eng_factor = 10;
	op_factor = 10;
	//vss = 0;
        vss = 0;
	tim_dc = 0;
	tim_frq = 0;
	hfmsig = 0;
	hfmref = 0;
       /* for(index_result = 0;index_result < 120;index_result ++ )
        {
            result_axle[index_result] = 0;
            result_op_37[index_result] = 0;
            result_op_120[index_result] = 0;
            result_axle[index_result] = IS_IN_RANGE(index_result, 58, 59) | IS_IN_RANGE(index_result, 118, 119);
            result_op_37[index_result] = IS_IN_RANGE(index_result, 36, 36);
            result_op_120[index_result] = IS_IN_RANGE(index_result, 28, 29) | IS_IN_RANGE(index_result, 58, 59) \
                                        | IS_IN_RANGE(index_result, 88, 89) | IS_IN_RANGE(index_result, 118, 119);
        }*/
        for(index_result = 0;index_result < 3840;index_result ++ )
        {
          gear32_120[index_result] = gear32_1[index_result%32];
          op32_120[index_result] = gear32_1[index_result%32];
          if(IS_IN_RANGE(index_result, axle_min1, axle_max1) | IS_IN_RANGE(index_result, axle_min2, axle_max2))
          {
             gear32_120[index_result] = gear32_1[0];
          }
          if(IS_IN_RANGE(index_result, op_min1, op_max1) | IS_IN_RANGE(index_result, op_min2, op_max2) | IS_IN_RANGE(index_result, op_min3, op_max3) | IS_IN_RANGE(index_result, op_min4, op_max4))
          {
              op32_120[index_result] = gear32_1[0];
          }
        }
	simulator_Start();
}

void weifu_Update(void)
{
	//communication update
	weifu_ConfigData();
        
	if (eng_rpm != cfg_data.eng_rpm) {
		#if SAMPLE_DENSITY_HIGH
		clock_SetFreq(cfg_data.eng_rpm << 5);
		#else
		clock_SetFreq(cfg_data.eng_rpm << 4);
		#endif
		eng_rpm = cfg_data.eng_rpm;
		op_rpm = eng_rpm >> 1;
            
		if (IS_IN_RANGE(eng_rpm, 0, 500)) {
			eng_factor = WAVE_FACTOR_3;
		} else if (IS_IN_RANGE(eng_rpm, 500, 1000)) {
			eng_factor = WAVE_FACTOR_5;
		} else if (IS_IN_RANGE(eng_rpm, 1000, 1500)) {
			eng_factor = WAVE_FACTOR_7;
		} else {
			eng_factor = WAVE_FACTOR_10;
		}

		if (IS_IN_RANGE(op_rpm, 0, 500)) {
			op_factor = WAVE_FACTOR_3;
		} else if (IS_IN_RANGE(op_rpm, 500, 1000)) {
			op_factor = WAVE_FACTOR_5;
		} else if (IS_IN_RANGE(op_rpm, 1000, 1500)) {
			op_factor = WAVE_FACTOR_7;
		} else {
			op_factor = WAVE_FACTOR_10;
		}
	}
	if (phase_diff != cfg_data.phase_diff) {
		phase_diff = cfg_data.phase_diff;
	}
	if (vss != cfg_data.vss) {        //need vss
		vss = cfg_data.vss;
		vss_SetFreq(vss);
	}
	if (tim_dc != cfg_data.tim_dc) {  //need pwm1
		tim_dc = cfg_data.tim_dc;
		tim_frq = cfg_data.tim_frq;
		pwm1_Init(tim_frq + 3, 100 - tim_dc);
	}
	if (tim_frq != cfg_data.tim_frq) {  //need pwm1
		tim_dc = cfg_data.tim_dc;
		tim_frq = cfg_data.tim_frq;
		pwm1_Init(tim_frq + 3, 100 - tim_dc);
	}
	if (hfmsig != cfg_data.hfmsig) {  //need wss
		hfmsig = cfg_data.hfmsig;
		wss_SetFreq(hfmsig);
	}
	if (hfmref != cfg_data.hfmref) {  //need pwm2
		hfmref = cfg_data.hfmref;
		pwm2_Init(hfmref, 50);
	}
	#if WEIFU_SUPPORT_TWO_WAVE
	if (op_mode != cfg_data.op_mode) {  //need pwm2
		op_mode = cfg_data.op_mode;
		axle_clock_cnt = 0;
		op_clock_cnt  = 0;
                op_clock_cnt_1 = 0;
		op_tri_flag = 1;
                if(op_mode == OP_WAVE_MODE_37X){
                  s = gear104_3840;
                }
                if(op_mode == OP_WAVE_MODE_120X){
                s = op32_120;
                }
	}
	#endif

	//for eng_speed and wtout output 
	if (time_left(wtout_timer) < 0) {
		cfg_data.wtout = 100 - pwmin1_GetDC();
		wtout_timer = time_get(250);
	}
	if (time_left(eng_speed_timer) < 0) {
		cfg_data.eng_speed = counter2_GetValue();
		counter2_SetValue(0);
		//2hz = 1 eng round
		eng_speed_timer = time_get(1000);
	}
      /*  if (op_tri_flag) {
          if (op_mode == OP_WAVE_MODE_120X)
		if(phase_diff < 0) {  //advanced
			if (axle_cnt == (61 + phase_diff/3)) {
                 // if (axle_cnt == (abs(phase_diff))) {
				//op_clock_cnt = 0;
                      //op_clock_cnt = 11*abs(phase_diff);
                       if(0==abs(phase_diff)%3)
				op_clock_cnt = (abs(phase_diff)%3+1) << 4;
                       else if(1==abs(phase_diff)%3){
                                 //op_clock_cnt =abs(11*(abs(phase_diff))-32*(abs(phase_diff)));
                                 //op_clock_cnt =22*(abs(phase_diff)+3)/3+32*(abs(phase_diff))/3;
                            //op_clock_cnt =32*((abs(phase_diff)+3)/3) +11;
                            op_clock_cnt = 43;
                           }
                           else  
                                 //op_clock_cnt = abs(22*(abs(phase_diff))-32*(abs(phase_diff)));
                             //op_clock_cnt = 20*(abs(phase_diff)+3)/3+32*(abs(phase_diff))/3;
                             //op_clock_cnt =32*((abs(phase_diff)+3)/3) +11;
                              op_clock_cnt=54;
                          
				op_tri_flag = 0;
			}
		} else {         //delayed
			if (axle_cnt == phase_diff) {   //三者本质都是21.3*phase_diff,但是这样会导致相位差不断的叠加，所以用if的形式，在第三个周期归零
                          if(0==phase_diff%3)
				//op_clock_cnt = 32*(phase_diff-phase_diff/3);
                            op_clock_cnt = (phase_diff<<6)/3;
                           else  
                                op_clock_cnt = 22*phase_diff;
                           
                           
				op_tri_flag = 0;
			}
		}      
	}*/
       /*if(op_tri_flag){
              if(phase_diff < 0) {  //advanced
			if (axle_cnt == (120 + phase_diff)) {
				op_clock_cnt = 0;
                                op_clock_cnt_1 = 0;
				op_tri_flag = 0;
			}
		} else {         //delayed
			if (axle_cnt == phase_diff) {
				op_clock_cnt = 0;
				op_tri_flag = 0;
                                op_clock_cnt_1 = 0;
			}
                } 
        }*/
       
         //axle_clock_cnt = 0;
        /* if(phase_diff < 0) //发动机超前
         {
           axle_clock_cnt -=11* phase_diff;
         }
         else
         {
           axle_clock_cnt +=11* phase_diff;
         }*/
         //if(axle_clock_cnt > 3839) axle_clock_cnt = 0;
         //if(op_clock_cnt > 3839) op_clock_cnt = 0;
         //if(op_clock_cnt_1 > 3839) op_clock_cnt_1 = 0;
         if(phase_diff < 0) //发动机超前，即OP落后，那么OP的表应该滞后
         {
           if(0 == abs(phase_diff) % 3)
             diff = 3840 + 32 * phase_diff / 3; //这是为了不产生相移叠加
           else diff = 3840 + 11 * phase_diff;
         }
         else 
         {
           if(0 == phase_diff)
             diff = 32 * phase_diff / 3;
           else diff = 11 * phase_diff;
         }
}

void weifu_isr(void)
{
	axle_clock_cnt ++;
	//op_clock_cnt ++;  
        //op_clock_cnt_1 ++;
        temp = axle_clock_cnt % AXLE_SAMPLE_POINTS;//0~31 用于每32个中断赋值一次，与下面应该可以通用？
       // temp_1 = op_clock_cnt % OP_SAMPLE_POINTS;
        temp1 = axle_clock_cnt % POINTS;//0~3839
        //temp2 = op_clock_cnt % POINTS; //用于码表赋值 与上下可以通用？
        //temp3 = op_clock_cnt_1 % POINTS;
        axle_cnt = (axle_clock_cnt >> 5) % 120;//除以32后取余，也就是说【axle_clock_cnt >> 5】这个值是每32次值加1，axle_cnt是0~119，每32次中断变一下
	if (prev_axle_cnt != temp) {
            *(volatile unsigned *)DAC_1 = gear32_120[temp1] * eng_factor  ;
             prev_axle_cnt = temp;
	}
        if(prev_op_cnt != temp) {
            *(volatile unsigned *)DAC_2 = *(s+(temp1+diff)%3840) * eng_factor ;
        }
          
        /*if (op_mode == OP_WAVE_MODE_37X){
          temp = op_clock_cnt % OP_SAMPLE_POINTS;
          if (prev_op_cnt != temp) {
            *(volatile unsigned *)DAC_2 = gear104_3840[temp2+diff] * eng_factor ;
            prev_op_cnt = temp;
          }
        }
        if (op_mode == OP_WAVE_MODE_120X){
          temp = op_clock_cnt_1 % OP_SAMPLE_POINTS;
          if (prev_op_cnt != temp) {
            *(volatile unsigned *)DAC_2 = op32_120[(temp3+diff)%3840] * eng_factor  ;
            prev_op_cnt = temp;
          }
        }*/
        /*if (result_axle[axle_cnt]) {
                            if (prev_axle_cnt != temp) {
                          //axle_SetAmp(((gear32[0]+0x0100) * eng_factor) / 15);                            
                              //*(volatile unsigned *)DAC_1 = (gear32[0]+0x01000) * eng_factor / 16;
                               *(volatile unsigned *)DAC_1 = gear32_1[0] * eng_factor ;
				prev_axle_cnt = temp;
			
			}
		} else {
			if (prev_axle_cnt != temp) {
                         // axle_SetAmp(((gear32[temp%32]+0x0100) * eng_factor) / 15);
                              *(volatile unsigned *)DAC_1 = gear32_1[temp] * eng_factor;//这边可以继续优化
                              //*(volatile unsigned *)DAC_1 = (gear32[temp]+0x0100) * eng_factor/16;//这边可以继续优化
				prev_axle_cnt = temp;
			}
                }*/
           /* if (op_mode == OP_WAVE_MODE_37X) {
		//calculate the oil pump gear output
		temp = op_clock_cnt % OP_SAMPLE_POINTS;
		op_cnt = (op_clock_cnt/OP_SAMPLE_POINTS) % 37;
                
			if (!result_op_37[op_cnt]) {  //if hypodontia, output zero//也就是说这里是第37个周期的码表
				if (prev_op_cnt != temp) {
                                  //temp=temp%104;
                                   *(volatile unsigned *)DAC_2 = (gear104[(temp+op_cnt/8)%104]+0x0100) * op_factor / 16;
                                  //*(volatile unsigned *)DAC_2 = gear104_1[(temp+op_cnt/5)%104] * op_factor ;
                                  //*(volatile unsigned *)DAC_2 = gear104_1[(temp+op_cnt/5)%104] * op_factor ;
                                   prev_op_cnt = temp;
                                      
				}
			} else {
				if (prev_op_cnt != temp) {
                                   //op_SetAmp(((gear104[(temp+op_cnt/5)%104]+0x0050) * op_factor) / 15);
                                   //temp1=temp+7;
                                     *(volatile unsigned *)DAC_2 = (gear104[(temp+4)%104]+0x0100) * op_factor / 16;
                                 // *(volatile unsigned *)DAC_2 = gear104_1[(temp+7)%104] * op_factor ;
                                  //*(volatile unsigned *)DAC_2 = gear104_1[(temp+7)%104] * op_factor ;
				     prev_op_cnt = temp;
                                     op_tri_flag = 1;        
				}
			}
		
                       }*/
        /*if (op_mode == OP_WAVE_MODE_120X) {
		//calculate the oil pump gear output
		temp = op_clock_cnt_1 % AXLE_SAMPLE_POINTS;//temp的值也是0~31
			op_cnt_1 = (op_clock_cnt_1 >> 5) % 120;//除以32后取余，也就是说【op_clock_cnt >> 5】这个值是每32次值加1，axle_cnt是0~119，每32次中断变一下
			if (result_op_120[op_cnt_1]) {
                          if (prev_op_cnt != temp) {
                                  //op_SetAmp(((gear32[0]+0x0050) * eng_factor) / 15);
                                   *(volatile unsigned *)DAC_2 = gear32_1[0] * eng_factor;
                            //*(volatile unsigned *)DAC_2 =(gear32[0]+0x0100) * eng_factor/16;
					prev_op_cnt = temp;
				}
			} else {
				if (prev_op_cnt != temp) {
                                   //op_SetAmp(((gear32[temp]+0x0050) * eng_factor) / 15);//0x0100是为了处理DAC输出截止，而/15是为了处理差分芯片输出截止
                                  if(op_cnt_1 == 118||op_cnt_1 == 119){
                                        op_tri_flag = 1;
                                  }
                                        *(volatile unsigned *)DAC_2 = gear32_1[temp] * eng_factor;
                                  //*(volatile unsigned *)DAC_2 =(gear32[temp]+0x0100) * eng_factor/16;
                                         prev_op_cnt = temp;
				}
			}                    
	}*/
}

void EXTI9_5_IRQHandler(void)
{
        weifu_isr();
        EXTI->PR = EXTI_Line6;
}

  int main(void)
{
	task_Init();
	weifu_Init();
	while(1) {
		task_Update();
		weifu_Update();
	}
}

/****************************************************************
****************** static local function  ******************
****************************************************************/

static int weifu_mdelay(int ms)
{
	int left;
	time_t deadline = time_get(ms);
	do {
		left = time_left(deadline);
		if(left >= 10) { //system update period is expected less than 10ms
			task_Update();
		}
	} while(left > 0);

	return 0;
}

//how to handle the data, please see lcm mcamos server module.
static int weifu_ConfigData(void)
{
	int ret = 0;
	char lcm_cmd = LCM_CMD_READ;
	//communication with the lcm board
	ret += mcamos_dnload_ex(MCAMOS_INBOX_ADDR, &lcm_cmd, 1);
	ret += mcamos_upload_ex(MCAMOS_OUTBOX_ADDR + 2, &cfg_data, 16);
	if (ret) {
		//DEBUG_TRACE(("##MCAMOS ERROR!##\n"));
		return -1;
	}

	lcm_cmd = LCM_CMD_WRITE;
	//communication with the lcm board
	ret += mcamos_dnload_ex(MCAMOS_INBOX_ADDR, &lcm_cmd, 1);
	ret += mcamos_dnload_ex(MCAMOS_INBOX_ADDR + 1, &(cfg_data.eng_speed), 4);
	if (ret) {
		DEBUG_TRACE(("##MCAMOS ERROR!##\n"));
		return -1;
	}
	return 0;
}

#if 1
static int cmd_weifu_func(int argc, char *argv[])
{
	int result = -1;

	if(!strcmp(argv[1], "axle") && (argc == 3)) {
		axle_SetFreq(atoi(argv[2]));
		result = 0;
	}

	if(!strcmp(argv[1], "phase") && (argc == 3)) {
		phase_diff = atoi(argv[2]);
		result = 0;
	}

	if(!strcmp(argv[1], "start")) {
		simulator_Start();
		result = 0;
	} else if (!strcmp(argv[1], "stop")) {
		simulator_Stop();
		result = 0;
	}

	if(result == -1) {
		printf("uasge:\n");
		printf(" weifu axle  100	unit: hz/rpm\n");
		printf(" weifu phase 2		minus means advance, positive means delay\n");
		printf(" weifu start		open the 58x output\n");
		printf(" weifu stop 		shut up the 58x output\n");
	}

	return 0;
}

cmd_t cmd_weifu = {"weifu", cmd_weifu_func, "commands for weifu simulator"};
DECLARE_SHELL_CMD(cmd_weifu)

#endif
