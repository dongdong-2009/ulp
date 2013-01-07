#stm32
ifeq ($(CONFIG_CPU_STM32),y)
	obj-$(CONFIG_DRIVER_UART1) += uart1_stm32.o
	obj-$(CONFIG_DRIVER_UART2) += uart2_stm32.o
	obj-$(CONFIG_DRIVER_UART3) += uart3_stm32.o
endif

#lm3s
ifeq ($(CONFIG_CPU_LM3S),y)
	obj-$(CONFIG_DRIVER_UART0) += uart0_lm3s.o
	obj-$(CONFIG_DRIVER_UART1) += uart1_lm3s.o
endif

#sam3u
ifeq ($(CONFIG_CPU_SAM3U),y)
	obj-$(CONFIG_DRIVER_UARTg) += uartg_sam3u.o
endif

#lpc178x
ifeq ($(CONFIG_CPU_LPC178X),y)
	obj-$(CONFIG_DRIVER_UART0) += uart0_lpc178x.o
endif

obj-$(CONFIG_UART_CMD) += uart_cmd.o
#aduc706x
ifeq ($(CONFIG_CPU_ADUC706X),y)
	obj-$(CONFIG_DRIVER_UART0) += uart0_aduc706x.o
endif

obj-$(CONFIG_DEVICE_MD204L) += md204l.o
obj-$(CONFIG_DEVICE_LS1203) += ls1203.o
