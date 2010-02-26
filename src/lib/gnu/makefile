LIB 	:= $(obj)libgnu.a

COBJS-y += cortexm3_macro.o
COBJS-y += stm32f10x_vector.o

COBJS	:= $(COBJS-y)
SRCS 	:= $(COBJS:.o=.c)
OBJS 	:= $(addprefix $(obj),$(COBJS))

all:	$(LIB)

$(LIB): $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)

$(OBJS): $(ROOT_DIR)/stm32f10x_conf.h


clean:
	@rm -rf $(LIB)
	@rm -rf *.o
