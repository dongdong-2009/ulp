/*
 *  miaofng@2010 initial version
 */

#ifndef __ERR_H_
#define __ERR_H_

#include "config.h"

enum {
	ERR_OK,
	ERR_UNDEF,
	ERR_TIMEOUT,
	ERR_BUSY,
	ERR_ABORT,
	ERR_RETRY,
	ERR_PACKET,
	ERR_QUIT,
	ERR_OVERFLOW,
};

#endif /* __ERR_H_ */
