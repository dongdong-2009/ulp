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
static int cmd_zbarimg_func(int argc, char *argv[])
{
	static zbar_processor_t *processor;
	processor = zbar_processor_create(0);
	assert(processor);
	if(zbar_processor_init(processor, NULL, 0)) {
		zbar_processor_error_spew(processor, 0);
		return(-1);
	}

	zbar_image_t *zimage = zbar_image_create();
	assert(zimage);
	zbar_image_set_format(zimage, *(unsigned long*)"Y800");

	int width = IMAGE_W;
	int height = IMAGE_H;
	zbar_image_set_size(zimage, width, height);

	// extract grayscale image pixels
	// FIXME color!! ...preserve most color w/422P
	// (but only if it's a color image)
	size_t bloblen = width * height;
	unsigned char *blob = sys_malloc(bloblen);
	zbar_image_set_data(zimage, blob, bloblen, zbar_image_free_data);
	memcpy(blob, image, bloblen);

	zbar_process_image(processor, zimage);

	// output result data
	const zbar_symbol_t *sym = zbar_image_first_symbol(zimage);
	for(; sym; sym = zbar_symbol_next(sym)) {
		zbar_symbol_type_t typ = zbar_symbol_get_type(sym);
		if(typ == ZBAR_PARTIAL)
			continue;

		printf("%s%s:%s\n", zbar_get_symbol_name(typ), zbar_get_addon_name(typ), zbar_symbol_get_data(sym));
	}

	zbar_image_destroy(zimage);
	zbar_processor_destroy(processor);
	return 0;
}
const cmd_t cmd_zbarimg = {"zbarimg", cmd_zbarimg_func, "zbarimg demo program"};
DECLARE_SHELL_CMD(cmd_zbarimg)
#endif
