ifeq ($(CONFIG_CPU_STM32),y)
	ifneq (,$(filter y, $(CONFIG_DRIVER_SPI1) $(CONFIG_DRIVER_SPI2) $(CONFIG_DRIVER_SPI3)))
		obj-y += cs_stm32.o
	endif
	obj-$(CONFIG_DRIVER_SPI1) += spi1_stm32.o
	obj-$(CONFIG_DRIVER_SPI2) += spi2_stm32.o
	obj-$(CONFIG_DRIVER_SPI3) += spi3_stm32.o
endif

ifeq ($(CONFIG_CPU_ADUC706X),y)
	obj-$(CONFIG_DRIVER_SPI) += spi_aduc.o
endif

#spi chips
obj-$(CONFIG_DRIVER_VCHIP) += vchip.o
obj-$(CONFIG_DRIVER_AD9833) += ad9833.o
obj-$(CONFIG_DRIVER_MCP41X) += mcp41x.o
obj-$(CONFIG_DRIVER_DAC1X8S) += dac1x8s.o
obj-$(CONFIG_DRIVER_74HCT595) += 74hct595.o
obj-$(CONFIG_DRIVER_MBI5025) += mbi5025.o
obj-$(CONFIG_DRIVER_MCP23S17) += mcp23s17.o
obj-$(CONFIG_DRIVER_AD5663) += ad5663.o
