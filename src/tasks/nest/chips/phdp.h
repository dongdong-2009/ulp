/*
*	mt92 etc pdhp chip driver
 * 	miaofng@2012 initial version
 */

#ifndef __PHDP_H_
#define __PHDP_H_

nest_reg_t phdp_regs[] = {
	{.ofs = 0x00, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "DIE TEMP", .bind = NULL, .desc = NULL},
	{.ofs = 0x01, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "DIE TEMP", .bind = NULL, .desc = NULL},
	{.ofs = 0x02, .len = 0x01, .msk = 0x01, .val = 0x01, .name = "EN1/EN2 STATUS", .bind = NULL, .desc = NULL},
	{.ofs = 0x03, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "LS COMP TRIP", .bind = NULL, .desc = NULL},
	{.ofs = 0x04, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "HS CUR LIM IN TWCLM", .bind = NULL, .desc = NULL},
	{.ofs = 0x05, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "OPENFLT", .bind = NULL, .desc = NULL},
	{.ofs = 0x06, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "SHGND", .bind = NULL, .desc = NULL},
	{.ofs = 0x07, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "SHBAT", .bind = NULL, .desc = NULL},
};

nest_chip_t phdp = {
	.name = "phdp",
	.regs = (nest_reg_t *) phdp_regs,
	.nr_of_regs = 8,
};

#define phdp_init() nest_chip_init(&phdp)
#define phdp_bind(reg, pin) nest_chip_bind(&phdp, reg, pin)
#define phdp_mask(reg) nest_chip_trap(&phdp, reg, 0x00, 0x00)
#define phdp_trap(reg, msk, val) nest_chip_trap(&phdp, reg, msk, val)
#define phdp_verify(data) nest_chip_verify(&phdp, data)

#endif
