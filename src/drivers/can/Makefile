obj-$(CONFIG_CMD_CAN) += can_cmd.o
obj-$(CONFIG_CAN_BPMON) += can_bpmon.o

#stm32
ifeq ($(CONFIG_CPU_STM32),y)
	obj-$(CONFIG_DRIVER_CAN1) += can1_stm32.o
	obj-$(CONFIG_DRIVER_CAN2) += can2_stm32.o
endif

#lm3s
ifeq ($(CONFIG_CPU_LM3S),y)
	obj-$(CONFIG_DRIVER_CAN0) += can0_lm3s.o
endif
