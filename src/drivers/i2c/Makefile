ifeq ($(CONFIG_CPU_STM32),y)
	obj-$(CONFIG_DRIVER_I2C1) += i2c1_stm32.o
	obj-$(CONFIG_DRIVER_I2C2) += i2c2_stm32.o
endif
obj-$(CONFIG_I2C_SOFT) += i2c_soft.o

#i2c chips
obj-$(CONFIG_DRIVER_AT24CXX) += at24cxx.o
obj-$(CONFIG_DRIVER_MCP23017) += mcp23017.o
