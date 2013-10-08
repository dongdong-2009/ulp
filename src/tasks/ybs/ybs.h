/*
*
* miaofng@2013-9-26
*
*/

#ifndef __YBS_H__
#define __YBS_H__

enum {
	YBS_E_OK,
	YBS_E_CFG,
};

enum {
	YBS_CMD_RESET,
	YBS_CMD_SAVE,
	YBS_CMD_UNLOCK,
};

enum {
	REG_CMD, //like 
	REG_CAL = REG_CMD + 2,
	REG_SYS = REG_CAL + 16,
};


/*
note: 
1, clip is a sub function of digital ybs sensor
2, analog ybs sensor could be changed to digital ybs sensor by an special key sequece
*/
struct ybs_cfg_s {
	char cksum;
	char mode; //work mode, 'Digital' 'Analog' 'Undef'
	char sn[10]; //format: 130328001, NULL terminated
	float Zo; /*zero offset, unit: gf, set when reset button is clicked*/
	float Gi;
	float Di; //unit: gf
	float Go;
	float Do;
};


void ybs_isr(void);

#endif
