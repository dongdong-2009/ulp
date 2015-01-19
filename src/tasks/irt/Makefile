ifeq ($(CONFIG_IRT_IRC),y)
	obj-y += err.o
	obj-y += irc.o
	obj-y += vm.o
	obj-y += bsp.o
	obj-y += mxc.o
	obj-y += dps.o
	obj-y += rut.o
endif

ifeq ($(CONFIG_IRT_DPS),y)
	obj-y += dps/
endif

ifeq ($(CONFIG_IRT_MXC5324),y)
	obj-y += mxc.5x32.4/
endif

ifeq ($(CONFIG_IRT_PROBE),y)
	obj-y += probe.o
endif
