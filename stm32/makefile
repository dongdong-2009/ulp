LIB 	:= $(obj)libstm32.a

COBJS-y += src/stm32f10x_adc.o
COBJS-y += src/stm32f10x_bkp.o
COBJS-y += src/stm32f10x_can.o
COBJS-y += src/stm32f10x_dma.o
COBJS-y += src/stm32f10x_exti.o
COBJS-y += src/stm32f10x_flash.o
COBJS-y += src/stm32f10x_gpio.o
COBJS-y += src/stm32f10x_i2c.o
COBJS-y += src/stm32f10x_iwdg.o
COBJS-y += src/stm32f10x_lib.o
COBJS-y += src/stm32f10x_nvic.o
COBJS-y += src/stm32f10x_pwr.o
COBJS-y += src/stm32f10x_rcc.o
COBJS-y += src/stm32f10x_rtc.o
COBJS-y += src/stm32f10x_spi.o
COBJS-y += src/stm32f10x_systick.o
COBJS-y += src/stm32f10x_tim.o
COBJS-y += src/stm32f10x_tim1.o
COBJS-y += src/stm32f10x_usart.o
COBJS-y += src/stm32f10x_wwdg.o

COBJS	:= $(COBJS-y)
SRCS 	:= $(COBJS:.o=.c)
OBJS 	:= $(addprefix $(obj),$(COBJS))

all:	$(LIB)

$(LIB): $(OBJS)
	@echo $(AR) $(ARFLAGS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

$(OBJS): $(ROOT_DIR)/stm32f10x_conf.h


clean:
	@rm -rf $(LIB)
	@rm -rf src/*.o
