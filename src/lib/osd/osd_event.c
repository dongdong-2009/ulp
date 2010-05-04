/*
 * 	miaofng@2009 initial version
 */

#include "config.h"
#include "osd/osd_event.h"
#include "key.h"

int osd_event_init(void)
{
	key_Init();
	return 0;
}

int osd_event_get(void)
{
	return key_GetKey();
}
