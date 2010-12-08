#stm32
ifeq ($(CONFIG_CPU_STM32),y)
	obj-$(CONFIG_DRIVER_UART1) += uart1_stm32.o
	obj-$(CONFIG_DRIVER_UART2) += uart2_stm32.o
endif

#lm3s
ifeq ($(CONFIG_CPU_LM3S),y)
	obj-$(CONFIG_DRIVER_UART0) += uart0_lm3s.o
endif

obj-$(CONFIG_DEVICE_MD204L) += md204l.o
