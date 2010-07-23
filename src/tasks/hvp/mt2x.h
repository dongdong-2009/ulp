/*
 * 	miaofng@2010 initial version
 */
#ifndef __MT2X_H_
#define __MT2X_H_

/*mt2x program algo state machine*/
enum {
	READY,

	/*setup comm with old dut soft*/
	START, /*kwp wake up*/
	START_COMM, /*SID=0X81, start communication service*/
	START_DIAG, /*SID=0X10, start diagnostic session*/

	/*transfer utility to dut RAM*/
	REQ_DNLOAD, /*SID=0x34, request to download*/
	DNLOAD, /*SID=0X36, transfer data*/
	DNLOAD_EXIT, /*SID=0X37, request transfer data exit*/
	EXECUTE, /*SID=0X38, start routine by addr*/

	/*reprogram according to rules of utility*/
	INTERPRETER,

	FINISH,
};

typedef struct {
	int addr; /*utility dnload addr*/
	int size; /*utility dnload size*/
} mt2x_model_t;

#endif

