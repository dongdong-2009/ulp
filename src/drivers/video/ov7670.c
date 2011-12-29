/*
*  The OV7660/7171 Camera Chip is a low voltage CMOS image sensor that provide
*   the full functionality of a single-chip VGA camera and image processor in a small
*  footprint package. It provides full-frame, sub-sampled or windowed 8-bit images in
*  a wide range of formats, controlled through the serial camera control  bus(SCCB) i/f.
*
*	miaofng@2011 initial version
*/

#include "config.h"
#include "ulp/video.h"
#include "ulp/driver.h"
#include "ulp/device.h"
#include "i2c.h"
#include "sys/malloc.h"
#include "ulp/types.h"
#include "ulp_time.h"

static inline int sccb_write(const i2c_bus_t *i2c, uchar reg, uchar val)
{
	return i2c->wbuf(0x42, reg, 1, &val, 1);
}

static inline int sccb_read(const i2c_bus_t *i2c, uchar reg, uchar *pval)
{
	return i2c->rbuf(0x42, reg, 1, pval, 1);
}

struct ov_priv_s = {
	const i2c_bus_t *i2c_bus;
};

int ov_open(int fd, const void *pcfg)
{
	const struct ov_cfg_s *ov_cfg = pcfg;
	const i2c_bus_t *i2c;
	struct ov_priv_s *priv = sys_malloc(sizeof(struct ov_priv_s));
	assert(priv != NULL);
	priv->i2c = (pcfg == NULL) ? &i2c_soft : ov_cfg->i2c_bus;
	dev_priv_set(fd, priv);
	i2c = priv->i2c_bus;
	i2c->init(NULL);
	if(priv->xclk_set != NULL)
		priv->xclk_set(8000000);

	//sensor probe
	char id;
	sccb_write(i2c, 0x12, 0x80); //reset
	mdelay(50);
	sccb_read(i2c, 0x0b, &id);
	if(id != 0x73)
		return -1;

	//sensor init, default setting to: QVGA, RGB565
	sccb_write(i2c, 0x3a, 0x04);
	sccb_write(i2c, 0x40, 0x10);
	sccb_write(i2c, 0x12, 0x14);
	sccb_write(i2c, 0x32, 0x80);
	sccb_write(i2c, 0x17, 0x17);
	sccb_write(i2c, 0x18, 0x05);
	sccb_write(i2c, 0x19, 0x02);
	sccb_write(i2c, 0x1a, 0x7b);  /* 0x7a */
	sccb_write(i2c, 0x03, 0x0a);  /* 0x0a */
	sccb_write(i2c, 0x0c, 0x0c);
	sccb_write(i2c, 0x3e, 0x00);
	sccb_write(i2c, 0x70, 0x00);
	sccb_write(i2c, 0x71, 0x01);
	sccb_write(i2c, 0x72, 0x11);
	sccb_write(i2c, 0x73, 0x09);
	sccb_write(i2c, 0xa2, 0x02);
	sccb_write(i2c, 0x11, 0x00);
	sccb_write(i2c, 0x7a, 0x20);
	sccb_write(i2c, 0x7b, 0x1c);
	sccb_write(i2c, 0x7c, 0x28);
	sccb_write(i2c, 0x7d, 0x3c);
	sccb_write(i2c, 0x7e, 0x55);
	sccb_write(i2c, 0x7f, 0x68);
	sccb_write(i2c, 0x80, 0x76);
	sccb_write(i2c, 0x81, 0x80);
	sccb_write(i2c, 0x82, 0x88);
	sccb_write(i2c, 0x83, 0x8f);
	sccb_write(i2c, 0x84, 0x96);
	sccb_write(i2c, 0x85, 0xa3);
	sccb_write(i2c, 0x86, 0xaf);
	sccb_write(i2c, 0x87, 0xc4);
	sccb_write(i2c, 0x88, 0xd7);
	sccb_write(i2c, 0x89, 0xe8);
	sccb_write(i2c, 0x13, 0xe0);
	sccb_write(i2c, 0x00, 0x00);  /* AGC */
	sccb_write(i2c, 0x10, 0x00);
	sccb_write(i2c, 0x0d, 0x00);
	sccb_write(i2c, 0x14, 0x10);  /* 0x38 limit the max gain */
	sccb_write(i2c, 0xa5, 0x05);
	sccb_write(i2c, 0xab, 0x07);
	sccb_write(i2c, 0x24, 0x75);
	sccb_write(i2c, 0x25, 0x63);
	sccb_write(i2c, 0x26, 0xA5);
	sccb_write(i2c, 0x9f, 0x78);
	sccb_write(i2c, 0xa0, 0x68);
	sccb_write(i2c, 0xa1, 0x03);  /* 0x0b */
	sccb_write(i2c, 0xa6, 0xdf);  /* 0xd8 */
	sccb_write(i2c, 0xa7, 0xdf);  /* 0xd8 */
	sccb_write(i2c, 0xa8, 0xf0);
	sccb_write(i2c, 0xa9, 0x90);
	sccb_write(i2c, 0xaa, 0x94);
	sccb_write(i2c, 0x13, 0xe5);
	sccb_write(i2c, 0x0e, 0x61);
	sccb_write(i2c, 0x0f, 0x4b);
	sccb_write(i2c, 0x16, 0x02);
	sccb_write(i2c, 0x1e, 0x37);  /* 0x07 */
	sccb_write(i2c, 0x21, 0x02);
	sccb_write(i2c, 0x22, 0x91);
	sccb_write(i2c, 0x29, 0x07);
	sccb_write(i2c, 0x33, 0x0b);
	sccb_write(i2c, 0x35, 0x0b);
	sccb_write(i2c, 0x37, 0x1d);
	sccb_write(i2c, 0x38, 0x71);
	sccb_write(i2c, 0x39, 0x2a);
	sccb_write(i2c, 0x3c, 0x78);
	sccb_write(i2c, 0x4d, 0x40);
	sccb_write(i2c, 0x4e, 0x20);
	sccb_write(i2c, 0x69, 0x5d);
	sccb_write(i2c, 0x6b, 0x40);  /* PLL */
	sccb_write(i2c, 0x74, 0x19);
	sccb_write(i2c, 0x8d, 0x4f);
	sccb_write(i2c, 0x8e, 0x00);
	sccb_write(i2c, 0x8f, 0x00);
	sccb_write(i2c, 0x90, 0x00);
	sccb_write(i2c, 0x91, 0x00);
	sccb_write(i2c, 0x92, 0x00);  /* 0x19 0x66 */
	sccb_write(i2c, 0x96, 0x00);
	sccb_write(i2c, 0x9a, 0x80);
	sccb_write(i2c, 0xb0, 0x84);
	sccb_write(i2c, 0xb1, 0x0c);
	sccb_write(i2c, 0xb2, 0x0e);
	sccb_write(i2c, 0xb3, 0x82);
	sccb_write(i2c, 0xb8, 0x0a);
	sccb_write(i2c, 0x43, 0x14);
	sccb_write(i2c, 0x44, 0xf0);
	sccb_write(i2c, 0x45, 0x34);
	sccb_write(i2c, 0x46, 0x58);
	sccb_write(i2c, 0x47, 0x28);
	sccb_write(i2c, 0x48, 0x3a);
	sccb_write(i2c, 0x59, 0x88);
	sccb_write(i2c, 0x5a, 0x88);
	sccb_write(i2c, 0x5b, 0x44);
	sccb_write(i2c, 0x5c, 0x67);
	sccb_write(i2c, 0x5d, 0x49);
	sccb_write(i2c, 0x5e, 0x0e);
	sccb_write(i2c, 0x64, 0x04);
	sccb_write(i2c, 0x65, 0x20);
	sccb_write(i2c, 0x66, 0x05);
	sccb_write(i2c, 0x94, 0x04);
	sccb_write(i2c, 0x95, 0x08);
	sccb_write(i2c, 0x6c, 0x0a);
	sccb_write(i2c, 0x6d, 0x55);
	sccb_write(i2c, 0x6e, 0x11);
	sccb_write(i2c, 0x6f, 0x9f); /* 0x9e for advance AWB */
	sccb_write(i2c, 0x6a, 0x40);
	/* sccb_write(i2c, 0x01, 0x60);*/
	/* sccb_write(i2c, 0x02, 0x60); */
	sccb_write(i2c, 0x13, 0xe7);
	sccb_write(i2c, 0x15, 0x00);
	sccb_write(i2c, 0x4f, 0x80);
	sccb_write(i2c, 0x50, 0x80);
	sccb_write(i2c, 0x51, 0x00);
	sccb_write(i2c, 0x52, 0x22);
	sccb_write(i2c, 0x53, 0x5e);
	sccb_write(i2c, 0x54, 0x80);
	sccb_write(i2c, 0x55, 0x00);  /* 亮度 */
	sccb_write(i2c, 0x56, 0x60);  /* 对比度 */
	sccb_write(i2c, 0x57, 0x90);  /* 0x40 change according to Jim's request */
	sccb_write(i2c, 0x58, 0x9e);
	sccb_write(i2c, 0x41, 0x08);
	sccb_write(i2c, 0x3f, 0x05);  /* 边缘增强调整 */
	sccb_write(i2c, 0x75, 0x05);
	sccb_write(i2c, 0x76, 0xe1);
	sccb_write(i2c, 0x4c, 0x0F);  /* 噪声抑制强度 */
	sccb_write(i2c, 0x77, 0x0a);
	sccb_write(i2c, 0x3d, 0xc2);  /* 0xc0 */
	sccb_write(i2c, 0x4b, 0x09);
	sccb_write(i2c, 0xc9, 0xc8);
	sccb_write(i2c, 0x41, 0x38);

	sccb_write(i2c, 0x34, 0x11);
	sccb_write(i2c, 0x3b, 0x02);  /* 0x00 0x02 */
	sccb_write(i2c, 0xa4, 0x89);  /* 0x88 */
	sccb_write(i2c, 0x96, 0x00);
	sccb_write(i2c, 0x97, 0x30);
	sccb_write(i2c, 0x98, 0x20);
	sccb_write(i2c, 0x99, 0x30);
	sccb_write(i2c, 0x9a, 0x84);
	sccb_write(i2c, 0x9b, 0x29);
	sccb_write(i2c, 0x9c, 0x03);
	sccb_write(i2c, 0x9d, 0x4c);
	sccb_write(i2c, 0x9e, 0x3f);
	sccb_write(i2c, 0x78, 0x04);
	sccb_write(i2c, 0x79, 0x01);
	sccb_write(i2c, 0xc8, 0xf0);
	sccb_write(i2c, 0x79, 0x0f);
	sccb_write(i2c, 0xc8, 0x00);
	sccb_write(i2c, 0x79, 0x10);
	sccb_write(i2c, 0xc8, 0x7e);
	sccb_write(i2c, 0x79, 0x0a);
	sccb_write(i2c, 0xc8, 0x80);
	sccb_write(i2c, 0x79, 0x0b);
	sccb_write(i2c, 0xc8, 0x01);
	sccb_write(i2c, 0x79, 0x0c);
	sccb_write(i2c, 0xc8, 0x0f);
	sccb_write(i2c, 0x79, 0x0d);
	sccb_write(i2c, 0xc8, 0x20);
	sccb_write(i2c, 0x79, 0x09);
	sccb_write(i2c, 0xc8, 0x80);
	sccb_write(i2c, 0x79, 0x02);
	sccb_write(i2c, 0xc8, 0xc0);
	sccb_write(i2c, 0x79, 0x03);
	sccb_write(i2c, 0xc8, 0x40);
	sccb_write(i2c, 0x79, 0x05);
	sccb_write(i2c, 0xc8, 0x30);
	sccb_write(i2c, 0x69, 0xaa);
	sccb_write(i2c, 0x09, 0x00);
	sccb_write(i2c, 0x3b, 0x42);  /* 0x82 0xc0 0xc2 night mode */
	sccb_write(i2c, 0x2d, 0x01);  /* 0x82 0xc0 0xc2 night mode */
	return 0;
}

int ov_ioctl(int fd, int request, va_list args)
{
	return -1;
}

static int ov_init(int fd, const void *pcfg)
{
	return dev_class_register(DEV_CLASS_CAMERA, fd);
}

static const struct drv_ops_s ov_ops = {
	.init = ov_init,
	.open = ov_open,
	.ioctl = ov_ioctl,
	.read = NULL,
	.write = NULL,
	.close = NULL,
};

static struct driver_s ov_driver = {
	.name = "ov7670",
	.ops = &ov_ops,
};

static void __driver_init(void)
{
	drv_register(&ov_driver);
}
driver_init(__driver_init);
