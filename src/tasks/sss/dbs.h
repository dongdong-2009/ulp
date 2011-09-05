/*
 * 	miaofng@2011 initial version
 *	DBS = Data-Based Satellite Communication Standard(Manchester Protocol)
 *	ref protocol document: DELPHI CS-10006398-001.01
 *	First Used Model Year 2007
 *
 *	bits types define:
 *		0	current low to high
 *		1	current high to low
 *		idle	current low to low
 *
 *	data rates:
 *		8000mps	T=5.35~5.95uS => 410/72MHz = 5.694uS => div = 410/2= 205
 *		4000mps	T=10.7~11.9uS
 *		2000mps	T=21.3~23.8uS
 *		1000mps	T=42.8~47.6uS
 */

#ifndef __DBS_H
#define __DBS_H

#define DBS_BLK_MS 130 /*comm blackout after powerup, 30mS min*/
#define DBS_BLK_MS_MIN 30
#define DBS_IMG_MS 20 /*inter msg group delay, 10mS min*/
#define DBS_IMD_MS 1 /*inter msg delay, 1ms max, 2bit normal*/
#define DBS_OKD_MS 1000 /*delay between SOH-OK*/

/*message types*/
enum {
	SOH_OK, /*0*/
	SOH_NOK,
	SOH_SELF_TEST_F,
	SOH_OUT_CHECK_F,
	SOH_DA_MIN_MAX_F,
	SOH_CELL_F,
	SOH_OTP_PARITY_F,
	SOH_THERMAL_SHUTDOWN, /*7*/
	TRACE,
	ACC,
};

/* dbs protocol bit sequence(19bits+2):
	start bits	2bits(0b00), sync purpose
	class bits	2bits(0b00 admin class, 0b01/0b10/0b11 to support up to 3 sensor in a package)
	data bits	10bits(acceleration data, $1FF = 511, $200 = -512)
	crc bits	5bits(x^5 + x^2 + 1)
	idle bits	2bits
*/
union dbs_msg_s {
	struct { //lsb
		unsigned dummy : 3;
		unsigned crc : 5; /*lsb, 5bit crc value*/
		unsigned data : 8; /*tracebility data*/
		unsigned addr : 2; /*addr bit*/
		unsigned type : 2; /*class bits, always 00*/
		unsigned start : 2; /*start bytes, always 00*/
	} trace; /*tracebility msg*/

	struct {
		unsigned dummy : 3;
		unsigned crc : 5; /*lsb, 5bit crc value*/
		unsigned data : 6; /*state of health data*/
		unsigned addr : 2; /*addr bit*/
		unsigned d98 : 2; /*d9,d8 always 00*/
		unsigned type : 2; /*class bits, always 00*/
		unsigned start : 2; /*start bytes, always 00*/
	} soh; /*state of health msg*/

	struct {
		unsigned dummy : 3;
		unsigned crc : 5; /*lsb, 5bit crc value*/
		unsigned data : 10; /*acceleration data*/
		unsigned type : 2; /*class bits*/
		unsigned start : 2; /*start bytes, always 00*/
	} acc; /*acceleration data msg*/

	unsigned value; //bit [21, 3] effective
};
enum {
	DBS_SPEED_INVALID,
	DBS_SPEED_8000MPS,
	DBS_SPEED_4000MPS,
	DBS_SPEED_2000MPS,
	DBS_SPEED_1000MPS,
};

/*dbs protocol sensor para*/
struct dbs_sensor_s {
	char addr; //0x01 0x02 0x03
	char speed; //1->8000mps, 2->4000mps, 3->2000mps, 4->1000mps
	char trace[8];
};

//normal run
void dbs_init(struct dbs_sensor_s *, void *cfg);
void dbs_update(void);
void dbs_poweroff(void); //power lost

//learn mode
void dbs_learn_init(void);
void dbs_learn_update(void);
int dbs_learn_finish(void); //finish return 1
int dbs_learn_result(struct dbs_sensor_s *); //success return 0
#endif