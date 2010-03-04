TOP_DIR ?= $(shell pwd)
AUTOCONFIG_HEAD_FILE = $(TOP_DIR)/autoconfig.h
AUTOCONFIG_MAKE_FILE = $(TOP_DIR)/.config
IAR_TOOL = $(TOP_DIR)/scripts/iar.py
IAR_FILE = $(TOP_DIR)/projects/bldc/bldc.ewp
M ?= src/

#create iar project file according to the result of 'make xconfig'
-include $(AUTOCONFIG_MAKE_FILE)
-include $(M)Makefile

iar: detect_config iar_clr iar_inc iar_add
	@echo "projects/bldc/bldc.ewp has been created!"

iar_clr:
	$(IAR_TOOL) clr $(IAR_FILE)
iar_inc:
	$(IAR_TOOL) inc $(IAR_FILE) ./
	$(IAR_TOOL) inc $(IAR_FILE) src/include/
	$(IAR_TOOL) inc $(IAR_FILE) src/cpu/stm32/cmsis/
	$(IAR_TOOL) inc $(IAR_FILE) src/cpu/stm32/StdPeriph/inc/
iar_add:
	@echo target=$@ M=$(M): obj-y = $(obj-y)
	@$(IAR_TOOL) add $(IAR_FILE) $(M) $(obj-y)
	@for dir in $(obj-y); do\
		if [ -d $(M)$$dir ];\
		then \
			make -s -C $(TOP_DIR) M=$(M)$$dir $@; \
		fi \
	done

#xconfig
TKSCRIPTS_DIR = $(TOP_DIR)/scripts/tkconfig
PARSER = tkparse.exe

export PARSER

xconfig: $(PARSER)
	@$(TKSCRIPTS_DIR)/$(PARSER) src/Kconfig > main.tk
	@cat $(TKSCRIPTS_DIR)/header.tk main.tk $(TKSCRIPTS_DIR)/tail.tk > lconfig.tk
	@chmod a+x lconfig.tk
	@if test -r "/usr/bin/wish84.exe"; then /usr/bin/wish84.exe -f lconfig.tk; \
	else \
	  wish -f lconfig.tk; \
	fi; \
	if test $$? = "2" ; then                   \
		unix2dos autoconfig.h; \
		echo file .config/autoconfig.h has been created!; \
	fi

$(PARSER):
	@make -s -C $(TKSCRIPTS_DIR) $@

xconfig_clean:
	@make -s -C $(TKSCRIPTS_DIR) clean
	@rm -rf .tmp main.tk lconfig.tk

detect_config:
	@if [ -z $(wildcard $(AUTOCONFIG_HEAD_FILE)) ];\
	then \
		echo "System not configured!!!"; \
		echo "Please try: ";\
		echo "  make xconfig"; \
		echo "  make"; \
		echo ""; \
		exit 1; \
	fi

unconfig:
	@rm -rf $(AUTOCONFIG_HEAD_FILE)
	@rm -rf $(AUTOCONFIG_MAKE_FILE)

clean: xconfig_clean
	@rm -rf $(TOP_DIR)/projects/bldc/Debug
	@rm -rf $(TOP_DIR)/projects/bldc/Release
	@rm -rf $(TOP_DIR)/projects/bldc/settings
	@rm -rf $(TOP_DIR)/projects/bldc/bldc.dep
	@rm -rf $(TOP_DIR)/projects/bldc/bldc.ewd

distclean: clean iar_clr unconfig
