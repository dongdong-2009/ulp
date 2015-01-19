ifeq ($(CONFIG_YBS_CAL),y)
	obj-y += ybs_cal.o
	obj-y += ybs_dio.o
	obj-$(CONFIG_BRD_HWV10) += ybs_mcd.o
	obj-$(CONFIG_BRD_HWV20) += ybs_mcd_uart.o
	obj-y += ybs_mon.o
endif

ifeq ($(CONFIG_YBS_DMM),y)
	obj-$(CONFIG_BRD_HWV10) += dmm/
	obj-$(CONFIG_BRD_HWV20) += dmm2/
endif
