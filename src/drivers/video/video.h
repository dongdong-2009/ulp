/*
*	video capture, refer to V4L2 architecture
*	miaofng@2011 initial version
*/
#ifndef __VIDEO_H_
#define __VIDEO_H_

#include <inttypes.h>

#define DEV_CLASS_CAMERA "camera"
#define DEV_CLASS_VIDEO "video"

/*IOCTL CMDS*/
enum {
	VIDEO_FORMAT, //para: fmt, width, height, fmt =(long)"Y800"
	VIDEO_BUFFER, //para: fb_size, nr_of_fb, fb0, fb1 ...
	VIDEO_CAPTURE, //para: start, [repeat_mode?]
	VIDEO_POLL //para: a pointer of void * to recv fb which is ready
};

/*video port */
void vp_xclk_set(int hz); //optional, board dependant
struct vp_cfg_s {
	const char *camera; //camera name which existed at this video port
}

/*camera - ov driver configs*/
struct ov_cfg_s {
	const void *i2c_bus;
	const void (*xclk_set)(int hz); //8MHz
};

#endif /*__VIDEO_H_*/

