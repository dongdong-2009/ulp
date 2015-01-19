/*
 * 	xiaoming@2013 initial version
 */


#ifndef __C2MIO_H_
#define __C2MIO_H_

const char *c2mio_desc[] = {
	"OK",
	"Open load",
	"Short to Ground",
	"Short to Battery"
};

nest_reg_t c2mio_regs[] = {
	{.ofs = 0x00, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PCH01", .bind = NULL, .desc = c2mio_desc},
	{.ofs = 0x02, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PCH02", .bind = NULL, .desc = c2mio_desc},
	{.ofs = 0x04, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PCH03", .bind = NULL, .desc = c2mio_desc},
	{.ofs = 0x06, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PCH04", .bind = NULL, .desc = c2mio_desc},
	{.ofs = 0x08, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PCH05", .bind = NULL, .desc = c2mio_desc},
	{.ofs = 0x0a, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PCH06", .bind = NULL, .desc = c2mio_desc},
	{.ofs = 0x0c, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PCH07", .bind = NULL, .desc = c2mio_desc},
	{.ofs = 0x0e, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PCH08", .bind = NULL, .desc = c2mio_desc},
	{.ofs = 0x10, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PCH09", .bind = NULL, .desc = c2mio_desc},
	{.ofs = 0x12, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PCH10", .bind = NULL, .desc = c2mio_desc},
	{.ofs = 0x14, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PCH11", .bind = NULL, .desc = c2mio_desc},
	{.ofs = 0x16, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PCH12", .bind = NULL, .desc = c2mio_desc},
	{.ofs = 0x18, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PCH13", .bind = NULL, .desc = c2mio_desc},
	{.ofs = 0x1a, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PCH14", .bind = NULL, .desc = c2mio_desc},
	{.ofs = 0x1c, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PCH15", .bind = NULL, .desc = c2mio_desc},
	{.ofs = 0x1e, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PCH16", .bind = NULL, .desc = c2mio_desc},
	{.ofs = 0x20, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PCH17", .bind = NULL, .desc = c2mio_desc},
	{.ofs = 0x22, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PCH18", .bind = NULL, .desc = c2mio_desc},
	{.ofs = 0x24, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PCH19", .bind = NULL, .desc = c2mio_desc},
	{.ofs = 0x26, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PCH20", .bind = NULL, .desc = c2mio_desc},
	{.ofs = 0x28, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PCH21", .bind = NULL, .desc = c2mio_desc},
	{.ofs = 0x2a, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PCH22", .bind = NULL, .desc = c2mio_desc},
	{.ofs = 0x2c, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PCH23", .bind = NULL, .desc = c2mio_desc},
	{.ofs = 0x2e, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PCH24", .bind = NULL, .desc = c2mio_desc},
	{.ofs = 0x30, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PCH25", .bind = NULL, .desc = c2mio_desc},
	{.ofs = 0x32, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PCH26", .bind = NULL, .desc = c2mio_desc},
	{.ofs = 0x34, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PCH27", .bind = NULL, .desc = c2mio_desc},
	{.ofs = 0x36, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PCH28", .bind = NULL, .desc = c2mio_desc},
	{.ofs = 0x38, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PCH29", .bind = NULL, .desc = c2mio_desc},
	{.ofs = 0x3a, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PCH30", .bind = NULL, .desc = c2mio_desc},
	{.ofs = 0x3c, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PCH31", .bind = NULL, .desc = c2mio_desc},
	{.ofs = 0x3e, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PCH32", .bind = NULL, .desc = c2mio_desc},
	{.ofs = 0x40, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PCH33", .bind = NULL, .desc = c2mio_desc},
	{.ofs = 0x42, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PCH34", .bind = NULL, .desc = c2mio_desc},
	{.ofs = 0x44, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PCH35", .bind = NULL, .desc = c2mio_desc},
	{.ofs = 0x46, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PCH36", .bind = NULL, .desc = c2mio_desc},
	{.ofs = 0x48, .len = 0x02, .msk = 0x03, .val = 0x00, .name = "PCH37", .bind = NULL, .desc = c2mio_desc},
};

nest_chip_t c2mio = {
	.name = "C2MIO",
	.regs = (nest_reg_t *) &c2mio_regs,
	.nr_of_regs = 37,
};

#define c2mio_init() nest_chip_init(&c2mio)
#define c2mio_bind(reg, pin) nest_chip_bind(&c2mio, reg, pin)
#define c2mio_mask(reg) nest_chip_mask(&c2mio, reg, 0x00)
#define c2mio_trap(reg, msk, val) nest_chip_trap(&c2mio, reg, msk, val)
#define c2mio_verify(data) nest_chip_verify(&c2mio, data)

#endif