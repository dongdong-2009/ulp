obj-$(CONFIG_SPI_CS_GPIO) += spi_cs.o
ifeq ($(CONFIG_CPU_STM32),y)
	ifneq (,$(filter y, $(CONFIG_SPI1_CS_SOFT) $(CONFIG_SPI2_CS_SOFT)))
		obj-y += cs_stm32.o
	endif
	obj-$(CONFIG_DRIVER_SPI1) += spi1_stm32.o
	obj-$(CONFIG_DRIVER_SPI2) += spi2_stm32.o
endif
