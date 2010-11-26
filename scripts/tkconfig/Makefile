PARSER ?= tkparse.exe

obj-y += tkparse.o
obj-y += tkcond.o
obj-y += tkgen.o

$(PARSER): $(obj-y)
	gcc $(obj-y) -o $@

.c.o:
	gcc -c $< 

clean:
	@rm -rf $(obj-y) $(PARSER)