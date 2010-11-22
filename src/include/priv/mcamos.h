/*
 * 	miaofng@2010 initial version
 *
 */
#ifndef __MCAMOS_H_
#define __MCAMOS_H_

#include "config.h"
#include "can.h"

// Message type IDs.
#define MCAMOS_MSG_1_STD_ID      (0x7EE)
#define MCAMOS_MSG_2_STD_ID      (0x7EF)

// Global ID mask and "reject" filter.
#define MCAMOS_STD_GLOBAL_MASK   (0x7FF)
#define MCAMOS_STD_REJECT_FLTR   (0x7FF)

// Direction directives.
#define MCAMOS_DOWNLOAD          (0x00)
#define MCAMOS_UPLOAD            (0xFF)

// Execute directives.
#define MCAMOS_NOEXECUTE         (0x00)
#define MCAMOS_EXECUTE           (0xFF)

struct mcamos_cmd_s {
    char byAddress[4];
    char byByteCnt[2];
    char byDirection;
    char byExecute;
};

int mcamos_init(const can_bus_t *can, int baud);
int mcamos_dnload(const can_bus_t *can, int addr, const char *buf, int n, int timeout);
int mcamos_upload(const can_bus_t *can, int addr, char *buf, int n, int timeout);
int mcamos_execute(const can_bus_t *can, int addr, int timeout);


#endif /*__MCAMOS_H_*/
