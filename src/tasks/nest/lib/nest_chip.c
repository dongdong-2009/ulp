/*
 * 	miaofng@2011 initial version
 */

#include "config.h"
#include "nest_chip.h"
#include "nest_message.h"
#include <string.h>
#include "debug.h"

int nest_chip_init(nest_chip_t *chip)
{
	nest_reg_t *reg;
	for(int i = 0; i < chip -> nr_of_regs; i ++) {
		reg = chip -> regs + i;
		reg -> msk = 0xff;
	}
	return 0;
}

//bind a chip pin to a specific device function
int nest_chip_bind(nest_chip_t *chip, const char *reg_name, const char *pin_name)
{
	int found = -1;
	for(int i = 0; i < chip -> nr_of_regs; i ++) {
		if(!strcmp(chip -> regs[i].name, reg_name)) {
			//assert(chip -> regs[i].bind == NULL); //to avoid once more bind by mistake
			chip -> regs[i].bind = pin_name;
			found = 0;
			break;
		}
	}

	assert(found != -1);
	return found;
}

//in some special situation, we may cann't get the ideal value, but it's still acceptable, then this api is used to change the ideal value to a new one
int nest_chip_trap(nest_chip_t *chip, const char *reg_name, int msk, int val)
{
	int found = -1;
	for(int i = 0; i < chip -> nr_of_regs; i ++) {
		if(!strcmp(chip -> regs[i].name, reg_name)) {
			chip -> regs[i].msk = msk;
			chip -> regs[i].val = val;
			found = 0;
			break;
		}
	}

	assert(found != -1);
	return found;
}

//to verify the reister values are correct or not
int nest_chip_verify(nest_chip_t *chip, const char *data)
{
	int i, ret = 0;
	int val;
	nest_reg_t *reg;

	for(i = 0; i < chip -> nr_of_regs; i ++) {
		reg = chip -> regs + i;
		val = *(((__packed int *) data) + (int)( reg -> ofs / 32));
		val = (val >> (reg -> ofs % 32));
		val &= ((1 << reg -> len) - 1);
		if(val ^ reg -> val) { //error detected
			nest_message("%s Fail, %s = 0x%02x(", chip -> name, reg -> name, val);
			if(reg -> bind != NULL)
				nest_message("%s ", reg -> bind);
			if(reg -> desc != NULL)
				nest_message("%s", reg -> desc[val]);
			if((val ^ reg -> val) & reg -> msk) {
				nest_message("?)\n");
				ret --;
			}
			else {
				nest_message("?) .. masked\n");
			}
		}
	}
	return ret;
}
