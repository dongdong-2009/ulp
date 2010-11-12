ifeq ($(CONFIG_DRIVER_PD),y)
	obj-y += pd.o
	obj-$(CONFIG_PD_CMD) += pd_cmd.o
	obj-$(CONFIG_PD_CMD) += pd_cal.o
	obj-$(CONFIG_PD_TSC2046) += tsc2046.o
	obj-$(CONFIG_PD_RSM1843) += rsm1843.o
	obj-y += pd_linear.o
endif
