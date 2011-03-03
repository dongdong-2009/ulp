/*
*	this file tries to provide all common nest ops here
 * 	miaofng@2010 initial version
 */

#ifndef __NEST_H_
#define __NEST_H_

#include "config.h"

//std-c lib
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//basic nest resource driver
#include "cncb.h"

//nest_xxx functions
#include "nest_core.h"
#include "nest_time.h"
#include "nest_error.h"
#include "nest_light.h"
#include "nest_power.h"
#include "nest_message.h"
#include "nest_can.h"
#include "nest_chip.h"

//communication protocols
#include "priv/mcamos.h"
#include "priv/ccp.h"

#endif
