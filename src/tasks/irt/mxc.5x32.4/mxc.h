/*
*  design hints: (dead wait a response msg is not allowed!!!)
*  1, mxc send can msg INT to irc once 100mS if it's in INIT(power-up) state
*  2, irc response with OFFLINE then adds the slot to active list
*  3, when pc issue MODE cmd to irc, irc bring all slots to ready state
*  4, irc poll slot cards periodly and remove the dead card from the active list(error?yes)
*  5, if irc encounter an error, slot hold le signal then send INT msg to irc once 100mS until error cleared
*
*  miaofng@2014-6-5   initial version
*  miaofng@2014-9-17 mxc communication protocol update(fast birth slow die:)
*
*/

#ifndef __MXC5X32_H__
#define __MXC5X32_H__

#include "config.h"
#include "../mxc.h"
#include "../vm.h"
#include "../irc.h"
#include "../err.h"

void oe_set(int high);
void le_set(int high);
int le_get(void); //read hardware signal level
int le_is_pulsed(void);
void le_lock(void); //lock le level to 0, ignore le_set
void le_unlock(void); //unlock level to 0

int mxc_addr_get(void);
int mxc_has_diag(void); //slot card has self diag function return 1
int mxc_has_dcfm(void); //slot card has din cal fixture mounted return 1

//only change the memory of current relay image
void mxc_relay_clr_all(void);
void mxc_relay_set(int line, int bus, int on);
void mxc_vsense_set(int mask);
void mxc_linesw_set(unsigned line_mask);

void mxc_image_store(void); /*store image changes*/
void mxc_image_restore(void); /*ignore image changes*/
void mxc_image_select_static(void); /*set image pointer to static image*/
void mxc_image_write(void); //write selected image to mbi5025 & reset image pointer to dynamic image

void board_init(void);
void board_reset(void);

void mxc_can_handler(can_msg_t *msg);
void mxc_can_echo(int slot, int flag, int ecode);

int mxc_execute(void); //actual relay acts  here!!
void mxc_error(int ecode);

#endif
