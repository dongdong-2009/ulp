# setup
ROOT_DIR = $(shell pwd)
LIBS += stm32 gnu
INCLUDE_DIRS = -I . -I $(ROOT_DIR) $(addprefix -I $(ROOT_DIR)/,$(addsuffix /inc,$(LIBS)))
LIBRARY_DIRS = $(addprefix -L $(ROOT_DIR)/,$(LIBS))
COMPILE_OPTS = -mcpu=cortex-m3 -mthumb -Wall -g -O0

CC = arm-none-eabi-gcc
CFLAGS = $(COMPILE_OPTS) $(INCLUDE_DIRS)

CXX = arm-none-eabi-g++
CXXFLAGS = $(COMPILE_OPTS) $(INCLUDE_DIRS)

AS = arm-none-eabi-gcc
ASFLAGS = $(COMPILE_OPTS) -c

LD = arm-none-eabi-gcc
LDFLAGS = -Wl,--gc-sections,-Map=$@.map,-cref,-u,Reset_Handler $(INCLUDE_DIRS) $(LIBRARY_DIRS) -T stm32.ld

OBJCP = arm-none-eabi-objcopy
OBJCPFLAGS = -O binary

AR = arm-none-eabi-ar
ARFLAGS = cr

export ROOT_DIR
export CC CFLAGS CXX CXXFLAGS AS ASFLAGS LD LDFLAGS AR ARFLAGS

#app module compile
MAIN_OUT = bldc
MAIN_OUT_ELF = $(MAIN_OUT).elf
MAIN_OUT_BIN = $(MAIN_OUT).bin
LIB_FILES = $(addsuffix .a, $(join $(LIBS), $(addprefix /lib,$(LIBS))))

COBJS-y += main.o
COBJS-y += stm32f10x_it.o

COBJS	:= $(COBJS-y)
SRCS 	:= $(COBJS:.o=.c)
OBJS 	:= $(addprefix $(obj),$(COBJS))

all: $(MAIN_OUT_ELF) $(MAIN_OUT_BIN)

$(MAIN_OUT_ELF): $(OBJS)
	@for dir in $(LIBS); do\
		make -C $$dir; \
	done
	$(LD) $(LDFLAGS) $(OBJS) $(LIB_FILES) --output $@

$(MAIN_OUT_BIN): $(MAIN_OUT_ELF)
	$(OBJCP) $(OBJCPFLAGS) $< $@
	
clean:
	@rm -rf *.o *.map $(MAIN_OUT_ELF) $(MAIN_OUT_BIN)
	@for dir in $(LIBS); do\
		make -C $$dir clean; \
	done
