/*
 *	mt92 difo chip driver
 * 	miaofng@2012 initial version
 */

#ifndef __DIFO_H_
#define __DIFO_H_

nest_reg_t difo_regs[] = {
	//bigendian byte 0x1b, bit 0 ~ 15
	{.ofs = 0x00, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "IMODE0", .bind = NULL, .desc = NULL},
	{.ofs = 0x01, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "IMODE1", .bind = NULL, .desc = NULL},
	{.ofs = 0x02, .len = 0x01, .msk = 0x01, .val = 0x01, .name = "IMODE2", .bind = NULL, .desc = NULL},
	{.ofs = 0x03, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "FPUMP", .bind = NULL, .desc = NULL},
	{.ofs = 0x04, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "FMODE0", .bind = NULL, .desc = NULL},
	{.ofs = 0x05, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "FMODE1", .bind = NULL, .desc = NULL},
	//bigendian byte 0x1a
	{.ofs = 0x08, .len = 0x01, .msk = 0x01, .val = 0x01, .name = "VERSN0", .bind = NULL, .desc = NULL},
	{.ofs = 0x09, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "VERSN1", .bind = NULL, .desc = NULL},
	{.ofs = 0x0a, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "VERSN2", .bind = NULL, .desc = NULL},
	//bigendian byte 0x1d, bit 0 ~ 15
	{.ofs = 0x10, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "PRCTR2/1", .bind = NULL, .desc = NULL},
	{.ofs = 0x11, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "PRCTR4/3", .bind = NULL, .desc = NULL},
	{.ofs = 0x12, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "PRCTR5/6", .bind = NULL, .desc = NULL},
	{.ofs = 0x13, .len = 0x01, .msk = 0x01, .val = 0x01, .name = "STATUS", .bind = NULL, .desc = NULL},
	{.ofs = 0x14, .len = 0x01, .msk = 0x01, .val = 0x01, .name = "1", .bind = NULL, .desc = NULL},
	{.ofs = 0x15, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "OVDET", .bind = NULL, .desc = NULL},
	{.ofs = 0x16, .len = 0x01, .msk = 0x01, .val = 0x01, .name = "SSTEND", .bind = NULL, .desc = NULL},
	{.ofs = 0x17, .len = 0x01, .msk = 0x01, .val = 0x01, .name = "BSTLOW", .bind = NULL, .desc = NULL},
	//bigendian byte 0x1c
	{.ofs = 0x18, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "SHORT1", .bind = NULL, .desc = NULL},
	{.ofs = 0x19, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "SHORT2", .bind = NULL, .desc = NULL},
	{.ofs = 0x1a, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "SHORT3", .bind = NULL, .desc = NULL},
	{.ofs = 0x1b, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "SHORT4", .bind = NULL, .desc = NULL},
	{.ofs = 0x1c, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "SHORT5", .bind = NULL, .desc = NULL},
	{.ofs = 0x1d, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "SHORT6", .bind = NULL, .desc = NULL},
	{.ofs = 0x1e, .len = 0x01, .msk = 0x01, .val = 0x01, .name = "1", .bind = NULL, .desc = NULL},
	{.ofs = 0x1f, .len = 0x01, .msk = 0x01, .val = 0x01, .name = "1", .bind = NULL, .desc = NULL},
	//bigendian - byte 1f, bit 0
	{.ofs = 0x20, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "OPEN5", .bind = NULL, .desc = NULL},
	{.ofs = 0x21, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "OPEN6", .bind = NULL, .desc = NULL},
	{.ofs = 0x22, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "LSTG5", .bind = NULL, .desc = NULL},
	{.ofs = 0x23, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "LSTB5", .bind = NULL, .desc = NULL},
	{.ofs = 0x24, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "LSTG6", .bind = NULL, .desc = NULL},
	{.ofs = 0x25, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "LSTB6", .bind = NULL, .desc = NULL},
	{.ofs = 0x26, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "HSTG5/6", .bind = NULL, .desc = NULL},
	{.ofs = 0x27, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "HSTB5/6", .bind = NULL, .desc = NULL},
	//bigendian - byte 1e
	{.ofs = 0x28, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "HDLW1/2", .bind = NULL, .desc = NULL},
	{.ofs = 0x29, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "PKLW1/2", .bind = NULL, .desc = NULL},
	{.ofs = 0x2a, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "HDLW1/2", .bind = NULL, .desc = NULL},
	{.ofs = 0x2b, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "PKLW3/4", .bind = NULL, .desc = NULL},
	{.ofs = 0x2c, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "HDLW5/6", .bind = NULL, .desc = NULL},
	{.ofs = 0x2d, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "PKLW5/6", .bind = NULL, .desc = NULL},
	{.ofs = 0x2e, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "SPIERR", .bind = NULL, .desc = NULL},
	{.ofs = 0x2f, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "0", .bind = NULL, .desc = NULL},
	//bigendian - byte 21, bit 0
	{.ofs = 0x30, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "OPEN1", .bind = NULL, .desc = NULL},
	{.ofs = 0x31, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "OPEN2", .bind = NULL, .desc = NULL},
	{.ofs = 0x32, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "LSTG1", .bind = NULL, .desc = NULL},
	{.ofs = 0x33, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "LSTB1", .bind = NULL, .desc = NULL},
	{.ofs = 0x34, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "LSTG2", .bind = NULL, .desc = NULL},
	{.ofs = 0x35, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "LSTB2", .bind = NULL, .desc = NULL},
	{.ofs = 0x36, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "HSTG1/2", .bind = NULL, .desc = NULL},
	{.ofs = 0x37, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "HSTB1/2", .bind = NULL, .desc = NULL},
	//bigendian - byte 20
	{.ofs = 0x38, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "OPEN3", .bind = NULL, .desc = NULL},
	{.ofs = 0x39, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "OPEN4", .bind = NULL, .desc = NULL},
	{.ofs = 0x3a, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "LSTG3", .bind = NULL, .desc = NULL},
	{.ofs = 0x3b, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "LSTB3", .bind = NULL, .desc = NULL},
	{.ofs = 0x3c, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "LSTG4", .bind = NULL, .desc = NULL},
	{.ofs = 0x3d, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "LSTB4", .bind = NULL, .desc = NULL},
	{.ofs = 0x3e, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "HSTG3/4", .bind = NULL, .desc = NULL},
	{.ofs = 0x3f, .len = 0x01, .msk = 0x01, .val = 0x00, .name = "HSTB3/4", .bind = NULL, .desc = NULL},
};

nest_chip_t difo = {
	.name = "difo",
	.regs = (nest_reg_t *) difo_regs,
	.nr_of_regs = 57,
};

#define difo_init() nest_chip_init(&difo)
#define difo_bind(reg, pin) nest_chip_bind(&difo, reg, pin)
#define difo_trap(reg, msk, val) nest_chip_trap(&difo, reg, msk, val)
#define difo_verify(data) nest_chip_verify(&difo, data)

#endif
