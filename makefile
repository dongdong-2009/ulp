# setup
ROOT_DIR = $(shell pwd)
LIBS += board stm32 gnu
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

all: board_config $(MAIN_OUT_ELF) $(MAIN_OUT_BIN)
	
$(MAIN_OUT_ELF): $(OBJS)
	@for dir in $(LIBS); do\
		make -C $$dir; \
	done
	$(LD) $(LDFLAGS) $(OBJS) $(LIB_FILES) --output $@

$(MAIN_OUT_BIN): $(MAIN_OUT_ELF)
	$(OBJCP) $(OBJCPFLAGS) $< $@

AUTOCONFIG_HEAD_FILE = autoconfig.h
board_config:
	@if [ -z $(wildcard $(AUTOCONFIG_HEAD_FILE)) ];\
	then \
		echo "System not configured!!!"; \
		echo "Please try: ";\
		echo "  make your_board_name_config"; \
		echo "  make"; \
		echo ""; \
		exit 1; \
	fi
	
zf103_config: board_unconfig
	@ln -s boards/zf103 ./board
	@echo -e "#define CONFIG_BOARD_ZF103\r" > $(AUTOCONFIG_HEAD_FILE)
	@echo -e "#define CONFIG_BOARD_DEBUG\r" >> $(AUTOCONFIG_HEAD_FILE)
	
hurry_config: board_unconfig
	@ln -s boards/hurry ./board
	@echo -e "#define CONFIG_BOARD_HURRY\r" > $(AUTOCONFIG_HEAD_FILE)

board_unconfig:
	@rm -rf ./board
	@rm -rf $(AUTOCONFIG_HEAD_FILE)
	
clean:
	@rm -rf *.o *.map $(MAIN_OUT_ELF) $(MAIN_OUT_BIN)
	@for dir in $(LIBS); do\
		make -C $$dir clean; \
	done

distclean: clean board_unconfig