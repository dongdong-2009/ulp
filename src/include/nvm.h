/*
 * 	miaofng@2010 initial version
 *
 */
#ifndef __NVM_H_
#define __NVM_H_

#include "config.h"

#pragma segment=".nvm.rom"
#pragma section=".nvm.ram" 4
#define __nvm @".nvm.ram"

//move __nvm data from section ".nvm.flash" to ".nvm.ram"
int nvm_init(void);

//move __nvm data from ".nvm.ram" to section ".nvm.flash"
int nvm_save(void);
int nvm_clear(void);

//for power-down auto save function
void nvm_isr(void);

//return true if nvm is null(no correct info inside it)
int nvm_is_null(void);

#endif /*__NVM_H_*/
