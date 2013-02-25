ifeq ($(CONFIG_OID_MAIN),y)
	obj-y += oid_gui.o
	obj-$(CONFIG_OID_HWV1) += oid.o
	obj-$(CONFIG_OID_HWV2) += oid2.o
	obj-$(CONFIG_OID_HWV2) += oid_mcd.o
	obj-$(CONFIG_OID_HWV2) += oid_gui.o
endif

ifeq ($(CONFIG_OID_DMM),y)
	obj-$(CONFIG_OID_HWV2) += oid_dmm.o
	obj-$(CONFIG_OID_HWV2) += aduc_adc.o
endif

obj-$(CONFIG_OID_YBS) += oid_ybs.o
