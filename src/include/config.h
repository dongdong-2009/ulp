/* config.h
 * 	miaofng@2009 initial version
 */
 
#ifndef __CONFIG_H_
#define __CONFIG_H_

#include "autoconfig.h"

#ifdef CONFIG_LIB_ZBAR
#include "zbar_config.h"
#endif

#ifdef CONFIG_CPU_STM32
#include "cpu_config.h"
#endif

#endif /*__CONFIG_H_*/
