ZBAR_VERSION = 0.10
ZBAR_FOLDER = zbar-$(ZBAR_VERSION)/zbar

#zbar core
obj-y += $(ZBAR_FOLDER)/config.o
obj-y += $(ZBAR_FOLDER)/error.o
obj-y += $(ZBAR_FOLDER)/symbol.o
obj-y += $(ZBAR_FOLDER)/image.o
obj-y += $(ZBAR_FOLDER)/convert.o
obj-y += $(ZBAR_FOLDER)/processor.o
obj-y += $(ZBAR_FOLDER)/processor/lock.o
obj-y += $(ZBAR_FOLDER)/refcnt.o
obj-y += $(ZBAR_FOLDER)/window.o
obj-y += $(ZBAR_FOLDER)/video.o
obj-y += $(ZBAR_FOLDER)/img_scanner.o
obj-y += $(ZBAR_FOLDER)/scanner.o
obj-y += $(ZBAR_FOLDER)/decoder.o

#zbar decoder
obj-$(CONFIG_ZBAR_EAN) += $(ZBAR_FOLDER)/decoder/ean.o
obj-$(CONFIG_ZBAR_CODE128) += $(ZBAR_FOLDER)/decoder/code128.o
obj-$(CONFIG_ZBAR_CODE39) += $(ZBAR_FOLDER)/decoder/code39.o
obj-$(CONFIG_ZBAR_I25) += $(ZBAR_FOLDER)/decoder/i25.o
obj-$(CONFIG_ZBAR_PDF417) += $(ZBAR_FOLDER)/decoder/pdf417.o
obj-$(CONFIG_ZBAR_QRCODE) += $(ZBAR_FOLDER)/decoder/qr_finder.o
obj-$(CONFIG_ZBAR_QRCODE) += $(ZBAR_FOLDER)/qrcode/qrdec.o
obj-$(CONFIG_ZBAR_QRCODE) += $(ZBAR_FOLDER)/qrcode/qrdectxt.o
obj-$(CONFIG_ZBAR_QRCODE) += $(ZBAR_FOLDER)/qrcode/rs.o
obj-$(CONFIG_ZBAR_QRCODE) += $(ZBAR_FOLDER)/qrcode/isaac.o
obj-$(CONFIG_ZBAR_QRCODE) += $(ZBAR_FOLDER)/qrcode/bch15_5.o
obj-$(CONFIG_ZBAR_QRCODE) += $(ZBAR_FOLDER)/qrcode/binarize.o
obj-$(CONFIG_ZBAR_QRCODE) += $(ZBAR_FOLDER)/qrcode/util.o

#if WIN32
#zbar_libzbar_la_SOURCES += zbar/processor/win.c zbar/libzbar.rc
#zbar_libzbar_la_CPPFLAGS += -mthreads
#zbar_libzbar_la_LDFLAGS += -mthreads
# FIXME broken
#zbar_libzbar_la_LIBADD += zbar/libzbar-rc.lo
#else
#zbar_libzbar_la_SOURCES += zbar/processor/posix.h zbar/processor/posix.c
#endif

#obj-y += $(ZBAR_FOLDER)/video/null.o
#obj-y += $(ZBAR_FOLDER)/processor/null.o
#obj-y += $(ZBAR_FOLDER)/window/null.o
obj-y += proc_ulp.o
obj-y += video_ulp.o
obj-y += window_ulp.o

obj-$(CONFIG_ZBAR_CMD) += zbar_cmd.o

#include path
inc-y += $(ZBAR_FOLDER)/
#inc-y += $(ZBAR_FOLDER)/processor/
#inc-y += $(ZBAR_FOLDER)/decoder/
#inc-y += $(ZBAR_FOLDER)/qrcode/
#inc-y += $(ZBAR_FOLDER)/video/
#inc-y += $(ZBAR_FOLDER)/window/
inc-y += $(ZBAR_FOLDER)/../include/
inc-y += $(ZBAR_FOLDER)/../../

