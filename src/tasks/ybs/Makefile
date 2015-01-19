ifeq ($(CONFIG_YBS_SENSOR),y)
	obj-y += ybs.o
	obj-y += ybsd.o
	obj-$(CONFIG_YBS_MODBUS) += ybsmb.o
endif

ifeq ($(CONFIG_YBS_CAL),y)
	obj-y += ybs_cal/
endif

ifeq ($(CONFIG_YBS_DMM),y)
	obj-y += ybs_cal/
endif
