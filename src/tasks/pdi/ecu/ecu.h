#ifndef __PDI_ECU_H_
#define __PDI_ECU_H_

#define ECU_BAUD 500000

//project must provide its specific start session function
extern int ecu_start_session(void);
extern int cmd_ecu_func_ex(int argc, char *argv[]); //optional

int ecu_can_init(int txid, int rxid);
int ecu_transceive(const can_msg_t *txmsg, can_msg_t *rxmsg);
int ecu_read(int addr, void *pbuf, int szbuf, char sid, char alen);
int ecu_did_read(int addr, void *pbuf, int szbuf);
int ecu_did_write(int addr, const void *pbuf, int szbuf);
int ecu_dtc_read(int addr, void *pbuf, int szbuf);
int ecu_dtc_clear(void);
int ecu_reset(void);

//decrypt algos
typedef void (*ecu_algo_t)(unsigned char access_level, unsigned char seed[8], unsigned char key[8]);
void ecu_decrypt_CalculateKey(unsigned char access_level, unsigned char seed[8], unsigned char key[8]);
void ecu_decrypt_CalculateKey_VOLVO(unsigned char access_level, unsigned char seed[8], unsigned char key[8]);
void ecu_decrypt_239A(unsigned char access_level, unsigned char seed[8], unsigned char key[8]);
void ecu_decrypt_34AB(unsigned char access_level, unsigned char seed[8], unsigned char key[8]);
void ecu_decrypt_B1234C12(unsigned char access_level, unsigned char seed[8], unsigned char key[8]);

//probe can id
int ecu_probe_id(int txid, int rxid);
int ecu_probe_session(int session);
int ecu_probe_algo(int session, ecu_algo_t decrypt_algo_func);

#endif
