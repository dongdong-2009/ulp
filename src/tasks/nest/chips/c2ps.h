/*
 *	mt92 power c2ps chip driver
 * 	miaofng@2012 initial version
 */

#ifndef __C2PS_H_
#define __C2PS_H_

nest_reg_t c2ps_regs[] = {
	//bigendian - byte 01, bit 0
	{.ofs = 0x00, .len = 0x01, .msk = 0x01, .val = 0x01, .name = "BATT_LOW*", .bind = NULL, .desc = NULL},
	{.ofs = 0x01, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "VKREG", .bind = NULL, .desc = NULL},
	{.ofs = 0x02, .len = 0x01, .msk = 0x01, .val = 0x01, .name = "FLT", .bind = NULL, .desc = NULL},
	{.ofs = 0x03, .len = 0x01, .msk = 0x01, .val = 0x01, .name = "TSD*", .bind = NULL, .desc = NULL},
	{.ofs = 0x04, .len = 0x01, .msk = 0x01, .val = 0x01, .name = "ETW*", .bind = NULL, .desc = NULL},
	{.ofs = 0x05, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "SPI_FLT", .bind = NULL, .desc = NULL},
	{.ofs = 0x06, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "PORST_INT", .bind = NULL, .desc = NULL},
	{.ofs = 0x07, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "RST5V_INT", .bind = NULL, .desc = NULL},
	//bigendian - byte 00
	{.ofs = 0x08, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "COPDIS", .bind = NULL, .desc = NULL},
	{.ofs = 0x09, .len = 0x01, .msk = 0x01, .val = 0x01, .name = "RST5VB", .bind = NULL, .desc = NULL},
	{.ofs = 0x0a, .len = 0x01, .msk = 0x01, .val = 0x01, .name = "WTO*", .bind = NULL, .desc = NULL},
	{.ofs = 0x0b, .len = 0x01, .msk = 0x01, .val = 0x01, .name = "COPTO*", .bind = NULL, .desc = NULL},
	{.ofs = 0x0c, .len = 0x01, .msk = 0x01, .val = 0x01, .name = "HWRSTB", .bind = NULL, .desc = NULL},
	{.ofs = 0x0d, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "EXTCLK", .bind = NULL, .desc = NULL},
	{.ofs = 0x0e, .len = 0x01, .msk = 0x01, .val = 0x01, .name = "IOEN_ST", .bind = NULL, .desc = NULL},
	{.ofs = 0x0f, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "LPC_ST", .bind = NULL, .desc = NULL},
	//bigendian - byte 03, bit 0
	{.ofs = 0x10, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "VR1REG*", .bind = NULL, .desc = NULL},
	{.ofs = 0x11, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "VR2REG", .bind = NULL, .desc = NULL},
	{.ofs = 0x12, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "VR3REG", .bind = NULL, .desc = NULL},
	{.ofs = 0x13, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "VR4REG", .bind = NULL, .desc = NULL},
	{.ofs = 0x14, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "VR5REG", .bind = NULL, .desc = NULL},
	{.ofs = 0x15, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "VCCREG", .bind = NULL, .desc = NULL},
	{.ofs = 0x16, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "VCLREG", .bind = NULL, .desc = NULL},
	{.ofs = 0x17, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "VC2REG", .bind = NULL, .desc = NULL},
	//bigendian - byte 02
	{.ofs = 0x18, .len = 0x01, .msk = 0x01, .val = 0x01, .name = "VERSN0", .bind = NULL, .desc = NULL},
	{.ofs = 0x19, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "VERSN1", .bind = NULL, .desc = NULL},
	{.ofs = 0x1a, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "VERSN2", .bind = NULL, .desc = NULL},
	{.ofs = 0x1b, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "CMODE0", .bind = NULL, .desc = NULL},
	{.ofs = 0x1c, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "CMODE1", .bind = NULL, .desc = NULL},
	//bigendian - byte 05, bit 0
	{.ofs = 0x23, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "BSTREG", .bind = NULL, .desc = NULL},
	{.ofs = 0x24, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "DRVFLT0", .bind = NULL, .desc = NULL},
	{.ofs = 0x25, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "DRVFLT1", .bind = NULL, .desc = NULL},
	{.ofs = 0x26, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "BKHREG", .bind = NULL, .desc = NULL},
	{.ofs = 0x27, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "VKLREG", .bind = NULL, .desc = NULL},
};

nest_chip_t c2ps = {
	.name = "C2PS",
	.regs = (nest_reg_t *) &c2ps_regs,
	.nr_of_regs = 34,
};

#define c2ps_init() nest_chip_init(&c2ps)
#define c2ps_bind(reg, pin) nest_chip_bind(&c2ps, reg, pin)
#define c2ps_mask(reg) nest_chip_mask(&c2ps, reg, 0x00)
#define c2ps_trap(reg, msk, val) nest_chip_trap(&c2ps, reg, msk, val)
#define c2ps_verify(data) nest_chip_verify(&c2ps, data)
#endif
