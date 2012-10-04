/**
* this file is used to provide ulp specific api func, sys_xxx
*
* miaofng@2012 initial version
*
*/

#ifndef __ULP_SYS_H_
#define __ULP_SYS_H_

#include "config.h"
#include "ulp_time.h"

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
#define sys_update	task_Update /*to avoid init sequence issue, never call me at xxx_init!!!*/
#define sys_mdelay	task_mdelay /*to avoid init sequence issue, never call me at xxx_init!!!*/

/*optional callback functions*/
#define __sys_tick task_tick /*called by system timer isr periodly*/
#define __sys_init bsp_init /*called by sys_init as early as possible*/
void __sys_update(void); /*called by sys_update*/

/* ulp api for dynamic memory management
 * sys_malloc
 * sys_free
*/
#include "sys/malloc.h"
static inline int sys_align(int x, int base) {
	int left = x % base;
	x += (left == 0) ? 0 : (base - left);
	return x;
}

/* ulp api for debug purpose*/
#include "ulp/debug.h"
#define sys_assert	assert
#define sys_dump	dump /*dump(unsigned addr, const void *p, int bytes)*/

/*obsolete*/
#define sys_tick	task_tick /*be called periodly per 1mS*/

#endif /*__ULP_SYS_H_*/
