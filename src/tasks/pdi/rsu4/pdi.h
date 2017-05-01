#ifndef __PDI_RSU_H_
#define __PDI_RSU_H_

#define PDI_HOST 1 //undef if no host exist
#define PDI_ITAC_MS 3000
#define PDI_PUSH_MS 500 //jitter cancel + pt204 integrate
#define PDI_HOST_MS 50 //interframe delay
#define PDI_LED_MS 300 //led flash ms
#define PDI_ECU_MS 100 //ECU ACK MS

#define PDI_DEBUG 1
#define debug if(PDI_DEBUG) printf
void hexdump(const char *prefix, const void *data, int nbytes);

enum {
	ERROR_NO = 0, /*UUTOK*/

	ERROR_NUM, /*N-UUTA*/
	ERROR_POSITION, /*P-UUTA*/

	ERROR_ECUNG, /*ECU-NG, in fact host not fully support it*/
	ERROR_BACHU, /*UUTANG-BRK*/
	ERROR_FUNC, /*UUTANG*/

	ERROR_ITAC,
};

typedef union {
	struct {
		unsigned char rsu1 : 2; //0b00 indiates no err
		unsigned char rsu2 : 2;
		unsigned char rsu3 : 2;
		unsigned char rsu4 : 2;
	};
	unsigned char byte;
} pdi_result_t;

typedef struct {
	pdi_result_t result;
	char datalog[8]; //to be sent to host
} pdi_report_t;

/*pls implement it in your own prj_name.c*/
extern const char *pdi_fixture_id;
extern int pdi_test(int pos, int mask, pdi_report_t *report); //pass return 0

int pdi_can_tx(const can_msg_t *msg);
int pdi_can_rx(can_msg_t *msg);

/*led_mask = --|-- RY|LY RG|LG RR|LR*/
void pdi_led_y(int led_mask);
void pdi_led_n(int led_mask);
void pdi_led_flash(int led_mask);
void pdi_mdelay(int ms);
void pdi_CalculateKey(unsigned char accessLevel, unsigned char seed[8], unsigned char key[3]);
#endif
