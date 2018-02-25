obj-y += led.o
obj-$(CONFIG_LED_GPIO) += led_gpio.o
ifeq ($(CONFIG_LED_HWDRV),y)
	obj-$(CONFIG_CPU_STM32) += led_stm32.o
	obj-$(CONFIG_CPU_LM3S) += led_lm3s.o
	obj-$(CONFIG_CPU_ADUC706X) += led_aduc.o
endif
