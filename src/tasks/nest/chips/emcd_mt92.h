/*
 *	mt92 emcd chip driver
 * 	miaofng@2012 initial version
 */

#ifndef __EMCD_H_
#define __EMCD_H_

nest_reg_t emcd_regs[] = {
	{.ofs = 0x00, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "DRN1", .bind = NULL, .desc = NULL},
	{.ofs = 0x01, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "DRN2", .bind = NULL, .desc = NULL},
	{.ofs = 0x02, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "DRN3", .bind = NULL, .desc = NULL},
	{.ofs = 0x03, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "DRN4", .bind = NULL, .desc = NULL},
	{.ofs = 0x04, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "DRN5", .bind = NULL, .desc = NULL},
	{.ofs = 0x05, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "DRN6", .bind = NULL, .desc = NULL},
	{.ofs = 0x06, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "DRN7", .bind = NULL, .desc = NULL},
	{.ofs = 0x07, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "DRN8", .bind = NULL, .desc = NULL},
};

nest_chip_t emcd = {
	.name = "EMCD",
	.regs = (nest_reg_t *) &emcd_regs,
	.nr_of_regs = 8,
};

#define emcd_init() nest_chip_init(&emcd)
#define emcd_bind(reg, pin) nest_chip_bind(&emcd, reg, pin)
#define emcd_mask(reg) nest_chip_mask(&emcd, reg, 0x00)
#define emcd_trap(reg, msk, val) nest_chip_trap(&emcd, reg, msk, val)
#define emcd_verify(data) nest_chip_verify(&emcd, data)
#endif
