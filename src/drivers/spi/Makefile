obj-$(CONFIG_SPI_CS_GPIO) += spi_cs.o
ifeq ($(CONFIG_CPU_STM32),y)
	ifneq (,$(filter y, $(CONFIG_SPI1_CS_SOFT) $(CONFIG_SPI2_CS_SOFT)))
		obj-y += cs_stm32.o
	endif
	obj-$(CONFIG_DRIVER_SPI1) += spi1_stm32.o
	obj-$(CONFIG_DRIVER_SPI2) += spi2_stm32.o
	obj-$(CONFIG_DRIVER_SPI3) += spi3_stm32.o
endif

#spi chips
obj-$(CONFIG_DRIVER_AD9833) += ad9833.o
obj-$(CONFIG_DRIVER_MCP41X) += mcp41x.o
obj-$(CONFIG_DRIVER_DAC1X8S) += dac1x8s.o
obj-$(CONFIG_DRIVER_74HCT595) += 74hct595.o
