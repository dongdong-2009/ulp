/**
* this file is used to provide ulp specific api func, sys_xxx
*
* miaofng@2012 initial version
*
*/

#ifndef __ULP_SYS_H_
#define __ULP_SYS_H_

#include "config.h"

/* ulp api for task related ops, such as:
 * void main(void) {
 * 	sys_init();
 * 	while(1) {
 * 		sys_update();
 * 	}
 * }
 *
 * or
 *
 * void main(void) {
 * 	sys_init();
 * 	while(!start_button_is_pressed) {
 * 		sys_update();
 * 	}
 * 	sys_mdelay(100);
 * 	scan_dut_bar_code();
 * 	...
 * }
 *
 */
#include "sys/task.h"
#define sys_init	task_Init
#define sys_update	task_Update
#define sys_mdelay	task_mdelay
#define sys_tick	task_tick /*be called periodly per 1mS*/

/* ulp api for dynamic memory management
 * sys_malloc
 * sys_free
*/
#include "sys/malloc.h"

/* ulp api for debug purpose*/
#include "ulp/debug.h"
#define sys_assert	assert

/*optional*/
void bsp_init(void);

#endif /*__ULP_SYS_H_*/
