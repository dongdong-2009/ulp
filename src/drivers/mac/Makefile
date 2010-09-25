ifeq ($(CONFIG_DRIVER_MAC),y)
	obj-y += ethernetif.o
	obj-$(CONFIG_CPU_STM32) += stm32_mac.o
	obj-$(CONFIG_CPU_LM3S) += stellarisif.o
endif
