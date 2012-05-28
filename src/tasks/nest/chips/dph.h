/*
 *	mt92 dph chip driver
 * 	miaofng@2012 initial version
 */

#ifndef __DPH_H_
#define __DPH_H_

nest_reg_t dph_regs[] = {
	//bigendian byte 0x25
	{.ofs = 0x00, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "INJ0", .bind = NULL, .desc = NULL},
	{.ofs = 0x01, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "INJ1", .bind = NULL, .desc = NULL},
	{.ofs = 0x02, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "INJ2", .bind = NULL, .desc = NULL},
	{.ofs = 0x03, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "INJ3", .bind = NULL, .desc = NULL},
	{.ofs = 0x04, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "ILO", .bind = NULL, .desc = NULL},
	{.ofs = 0x05, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "IHI", .bind = NULL, .desc = NULL},
	{.ofs = 0x06, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "SPI", .bind = NULL, .desc = NULL},
	{.ofs = 0x07, .len = 0x01, .msk = 0x01, .val = 0x01, .name = "DCB1", .bind = NULL, .desc = NULL},
	//bigendian byte 0x24
	{.ofs = 0x08, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "FOFFB", .bind = NULL, .desc = NULL},
	{.ofs = 0x09, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "FOFFG", .bind = NULL, .desc = NULL},
	{.ofs = 0x0a, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "INJHB", .bind = NULL, .desc = NULL},
	{.ofs = 0x0b, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "INJHG", .bind = NULL, .desc = NULL},
	{.ofs = 0x0c, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "SNSFLT", .bind = NULL, .desc = NULL},
	{.ofs = 0x0d, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "SETREJ", .bind = NULL, .desc = NULL},
	{.ofs = 0x0e, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "WD*", .bind = NULL, .desc = NULL},
	{.ofs = 0x0f, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "FAULT FLAG", .bind = NULL, .desc = NULL},
};

nest_chip_t dph = {
	.name = "dph",
	.regs = (nest_reg_t *) dph_regs,
	.nr_of_regs = 16,
};

#define dph_init() nest_chip_init(&dph)
#define dph_bind(reg, pin) nest_chip_bind(&dph, reg, pin)
#define dph_mask(reg) nest_chip_mask(&dph, reg, 0x00)
#define dph_trap(reg, msk, val) nest_chip_trap(&dph, reg, msk, val)
#define dph_verify(data) nest_chip_verify(&dph, data)

#endif
