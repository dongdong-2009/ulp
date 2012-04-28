/*
 * 	dusk@2010 initial version
 */

#include "config.h"
#include "shell/cmd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "zbar.h"
#include "assert.h"
#include "ulp/debug.h"
#include "sys/malloc.h"

#ifdef CONFIG_ZBAR_ZBARIMG
#include "nf123.png.h"
static int get_pattern_from_image(int fd, zbar_image_t *zimage)
{
	int width = IMAGE_W;
	int height = IMAGE_H;
	zbar_image_set_size(zimage, width, height);
	zbar_image_set_format(zimage, *(unsigned long*)"Y800");

	// extract grayscale image pixels
	// FIXME color!! ...preserve most color w/422P
	// (but only if it's a color image)
	size_t bloblen = width * height;
	unsigned char *blob = sys_malloc(bloblen);
	zbar_image_set_data(zimage, blob, bloblen, zbar_image_free_data);
	memcpy(blob, image, bloblen);
	return 0;
}
#endif

static int get_pattern_from_video(int vfd, zbar_image_t *zimage)
{
	unsigned char *blob = NULL;
	if(zbar_image_get_data(zimage) == NULL) {
		int width = 320;
		int height = 240;
		zbar_image_set_size(zimage, width, height);
		zbar_image_set_format(zimage, *(unsigned long*)"Y800");

		// extract grayscale image pixels
		// FIXME color!! ...preserve most color w/422P
		// (but only if it's a color image)
		size_t bloblen = width * height;
		blob = sys_malloc(bloblen);
		assert(blob != NULL);
		zbar_image_set_data(zimage, blob, bloblen, zbar_image_free_data);

		dev_ioctl(vfd, VIDEO_FORMAT, *(unsigned long*)"Y800", width, height);
		dev_ioctl(vfd, VIDEO_BUFFER, bloblen, 1, blob); //setup framebuffer
	}

	dev_ioctl(vfd, VIDEO_CAPTURE, 1); //start capture, stop automatically just like normal DMA ops
	while(blob == NULL) { //dead loop warnning !!!!
		dev_ioctl(vfd, VIDEO_POLL, &blob); //to poll which fb is ready for use
	}

	return 0;
}

static int cmd_zbar_func(int argc, char *argv[])
{
	//zprocessor
	static zbar_processor_t *processor;
	processor = zbar_processor_create(0);
	assert(processor);
	if(zbar_processor_init(processor, NULL, 0)) {
		zbar_processor_error_spew(processor, 0);
		return(-1);
	}

	//zimage
	zbar_image_t *zimage = zbar_image_create();
	assert(zimage);
	int fd = 0, loop = 1, fail;
#ifdef CONFIG_ZBAR_ZBARIMG
	if(argc > 1 && !strcmp(argv[1], "image")) {
		loop = 0;
		fail = get_pattern_from_image(0, zimage);
	}
#endif
	if(argc > 1 && !strcmp(argv[1], "video")) {
		fd = drv_open((argc > 2) ? argv[2] : "video0");
		fail = get_pattern_from_video(fd, zimage);
	}

	do {
		//got barcode pattern?
		if(fail) {
			printf("fail: cann't get barcode pattern\n");
			break;
		}

		//bar code identify
		zbar_process_image(processor, zimage);

		// output result data
		const zbar_symbol_t *sym = zbar_image_first_symbol(zimage);
		for(; sym; sym = zbar_symbol_next(sym)) {
			zbar_symbol_type_t typ = zbar_symbol_get_type(sym);
			if(typ == ZBAR_PARTIAL)
				continue;

			//successfully decoded bar code
			loop = 0;
			printf("%s%s:%s\n", zbar_get_symbol_name(typ), zbar_get_addon_name(typ), zbar_symbol_get_data(sym));
		}

		zbar_image_destroy(zimage);
		zbar_processor_destroy(processor);

		if(loop)
			get_pattern_from_video(fd);
	} while (loop);

	if(fd)
		drv_close(fd);
	return 0;
}
const cmd_t cmd_zbar = {"zbar", cmd_zbar_func, "zbar demo program"};
DECLARE_SHELL_CMD(cmd_zbar)
