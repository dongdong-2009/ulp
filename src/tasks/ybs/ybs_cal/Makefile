ifeq ($(CONFIG_YBS_CAL),y)
	obj-y += ybs_cal.o
	obj-y += ybs_dio.o
	obj-y += ybs_mcd.o
	obj-y += ybs_mon.o
endif

ifeq ($(CONFIG_YBS_DMM),y)
	obj-$(CONFIG_BRD_HWV10) += dmm/aduc_adc.o
	obj-$(CONFIG_BRD_HWV10) += dmm/ybs_dmm.o
endif
