LIB 	:= $(obj)libboard.a


COBJS-y += src/board.o
COBJS-y += src/time.o
COBJS-y += src/led.o
COBJS-y += src/console.o
COBJS-y += src/board_debug.o

COBJS	:= $(COBJS-y)
SRCS 	:= $(COBJS:.o=.c)
OBJS 	:= $(addprefix $(obj),$(COBJS))

all:	$(LIB)

$(LIB): $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

$(OBJS): $(ROOT_DIR)/config.h


clean:
	@rm -rf $(LIB)
	@rm -rf src/*.o
