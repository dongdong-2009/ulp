/*
 *	king@2011 initial version
 */

#ifndef __PSI5_H
#define __PSI5_H

#define PSI5_TGAP_US_MIN 10 /*T-gap, 8.4uS min*/
#define PSI5_TGAP_US 20 /*T-gap, 8.4uS min*/
#define PSI5_TBIT_US 8 /*T-bit, 7.6uS min, 8.4uS max*/
#define PSI5_BLK_MS_MIN 10
#define PSI5_BLK_MS 20

/* psi5 protocol speed */
enum {
	PSI5_SPEED_INVALID,
	PSI5_SPEED_125KBPS,
	PSI5_SPEED_189KBPS,
};

static enum {
	PSI5_EVENPARITY,
	PSI5_3CRC,
} psi5_parity = PSI5_EVENPARITY;

/* psi5 protocol sensor para */
struct psi5_sensor_s {
	unsigned char speed; //1->125kbps, 2->189kbps
	unsigned char parity; //0->PSI5_EVENPARITY, 1->PSI5_3CRC
	unsigned char data_bits; //每帧中的位数（不包括2位起始位）（11~31）
	unsigned short init_data[32]; //初始化数据
	unsigned char init_data_num; //初始化数据的个数
	unsigned short status_data[16]; //状态数据
	unsigned char status_data_num; //状态数据的个数
	unsigned char init_repeat; //初始化数据重复的次数
};

//normal run
void psi5_init(struct psi5_sensor_s *, void *cfg);
void psi5_update(void);
void psi5_poweroff(void); //power lost

//learn mode
void psi5_learn_init(void);
void psi5_learn_update(void);
int psi5_learn_finish(void); //finish return 1
int psi5_learn_result(struct psi5_sensor_s *); //success return 0
#endif