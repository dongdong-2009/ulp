TOP_DIR ?= $(shell pwd)
AUTOCONFIG_HEAD_FILE = $(TOP_DIR)/autoconfig.h
AUTOCONFIG_PROJ_FILE ?= ""
AUTOCONFIG_MAKE_FILE = $(TOP_DIR)/.config
IAR_TOOL = python $(TOP_DIR)/scripts/iar.py
IAR_FILE = $(TOP_DIR)/projects/bldc/bldc.ewp
ENV_FILE = $(TOP_DIR)/make.env
M ?= src/
PRJ_DIR = $(TOP_DIR)/projects/bldc
ULP_DIR = $(TOP_DIR)/projects/ulp
ULP_ICF = $(ULP_DIR)/ulp.icf
PRJ_ICF = $(PRJ_DIR)/ulp.icf
EXE_DIR = $(PRJ_DIR)/Debug/Exe
EXPORT_INC_DIR = $(EXE_DIR)/inc

#create iar project file according to the result of 'make xconfig'
-include $(AUTOCONFIG_MAKE_FILE)
-include $(M)Makefile
-include $(ENV_FILE)

iar: iar_script detect_config iar_clr iar_cfg iar_inc iar_add icf_cfg
	@echo "projects/bldc/bldc.ewp has been created!"

icf_cfg:
ifeq ($(CONFIG_CPU_ADUC706X),y)
	@if test -r $(PRJ_ICF); then \
		sed -i -r 's/\<symbol __ICFEDIT_size_irqstack__.*;/symbol __ICFEDIT_size_irqstack__ = 0x$(CONFIG_IRQSTACK_SIZE);/' $(PRJ_ICF); \
	fi
endif
	@if test -r $(PRJ_ICF); then \
		echo "apply settings to $(PRJ_ICF) ..."; \
		sed -i -r 's/\<symbol __ICFEDIT_size_cstack__.*;/symbol __ICFEDIT_size_cstack__ = 0x$(CONFIG_STACK_SIZE);/' $(PRJ_ICF); \
		sed -i -r 's/\<symbol __ICFEDIT_size_heap__.*;/symbol __ICFEDIT_size_heap__   = 0x$(CONFIG_HEAP_SIZE);/' $(PRJ_ICF); \
		sed -i -e "s/$$/\r/" $(PRJ_ICF) #change LF to windows CRLF; \
	fi

iar_help:
	$(IAR_TOOL)
iar_clr:
	$(IAR_TOOL) clr $(IAR_FILE)
iar_cfg:
ifeq ($(CONFIG_CPU_STM32),y)
	$(IAR_TOOL) cfg $(IAR_FILE) 'STM32F10xxE	ST STM32F10xxE' 'ulp.icf'
endif
ifeq ($(CONFIG_CPU_LM3S),y)
	$(IAR_TOOL) cfg $(IAR_FILE) 'LM3Sx9xx	Luminary LM3Sx9xx' 'ulp.icf'
endif
ifeq ($(CONFIG_CPU_SAM3U),y)
	$(IAR_TOOL) cfg $(IAR_FILE) 'AT91SAM3U4	Atmel AT91SAM3U4' 'ulp.icf'
endif
ifeq ($(CONFIG_CPU_LPC178X),y)
	$(IAR_TOOL) cfg $(IAR_FILE) 'LPC1788	NXP LPC1788' 'ulp.icf'
endif
ifeq ($(CONFIG_CPU_ADUC706X),y)
	$(IAR_TOOL) cfg $(IAR_FILE) 'ADuC7061	AnalogDevices ADuC7061' 'ulp.icf'
endif
iar_inc:
	$(IAR_TOOL) inc $(IAR_FILE) ./
	$(IAR_TOOL) inc $(IAR_FILE) src/include/
	#copy ulp header files to export folder
ifeq ($(CONFIG_TARGET_LIB),y)
	@rm -rf $(EXPORT_INC_DIR)
	@mkdir -p $(EXPORT_INC_DIR)
	@cp $(TOP_DIR)/*.h $(EXPORT_INC_DIR)/
	@mkdir -p $(EXPORT_INC_DIR)/src/include
	@-cp $(TOP_DIR)/src/include/*.h $(EXPORT_INC_DIR)/src/include
	@-for dir in asm linux ulp; do\
		mkdir -p $(EXPORT_INC_DIR)/src/include/$$dir; \
		cp $(TOP_DIR)/src/include/$$dir/*.h $(EXPORT_INC_DIR)/src/include/$$dir/ 2>&-; \
	done
endif

iar_add:
	@echo target=$@ M=$(M): obj-y = $(obj-y) inc-y = $(inc-y) icf-y = $(icf-y) bin-y = $(bin-y)
	@for dir in $(inc-y); do\
		if [ -d $(M)$$dir ];\
		then \
			$(IAR_TOOL) inc $(IAR_FILE) $(M)$$dir; \
		fi \
	done
	@$(IAR_TOOL) add $(IAR_FILE) $(M) $(obj-y)
	@for dir in $(obj-y); do\
		if [ -d $(M)$$dir ];\
		then \
			cd $(TOP_DIR) && $(MAKE) -s M=$(M)$$dir $@; \
		fi \
	done
	@for icf in $(icf-y); do \
		echo "found new icf file" $(M)$$icf ...; \
		cp -f $(M)$$icf $(PRJ_ICF); \
		cat $(ULP_ICF) >> $(PRJ_ICF); \
	done
	@for bin in $(bin-y); do \
		if test -r "$(M)$$bin";\
		then \
			name=`echo $$bin | sed -e "s/\./_/g"`; \
			$(IAR_TOOL) ldf $(IAR_FILE) --image_input "$$"PROJ_DIR"$$""/../.."/$(M)$$bin,$$name,.text,4; \
			$(IAR_TOOL) ldf $(IAR_FILE) --keep $$name; \
		fi \
	done
ifeq ($(CONFIG_TARGET_LIB),y)
	@-for dir in $(inc-y); do\
		if [ -d $(M)$$dir ];\
		then \
			mkdir -p $(EXPORT_INC_DIR)/$(M)$$dir; \
			cp $(TOP_DIR)/$(M)$$dir*.h $(EXPORT_INC_DIR)/$(M)$$dir 2>&-; \
		fi \
	done
	#copy lib header files according to the different lib used
	#M=src/lib/, obj-y=sys/ iar/ shell/ common/ priv/
	@if test $(M) = src/lib/; \
	then \
		for dir in $(obj-y); do\
			if [ -d $(TOP_DIR)/src/include/$$dir ];\
			then \
				cp -rf $(TOP_DIR)/src/include/$$dir $(EXPORT_INC_DIR)/src/include/ 2>&-; \
			fi \
		done \
	fi
endif

#xconfig
TKSCRIPTS_DIR = $(TOP_DIR)/scripts/tkconfig
PARSER = tkparse.exe

export PARSER

config_help:
	@echo "#auto generated, please do not edit me!!!" > config.help
	@echo "generationg file config.help, please wait ...";
	@cat `find src/ -name config.help` >>  config.help

xconfig: $(PARSER) config_help
	@$(TKSCRIPTS_DIR)/$(PARSER) src/Kconfig > main.tk
	@cat $(TKSCRIPTS_DIR)/header.tk main.tk $(TKSCRIPTS_DIR)/tail.tk > lconfig.tk
	@chmod a+x lconfig.tk
	@if test -r "/usr/bin/wish84.exe"; then /usr/bin/wish84.exe -f lconfig.tk; \
	else \
	  wish -f lconfig.tk; \
	fi
	@if test $$? = "2" ; then   \
		unix2dos autoconfig.h; \
	fi
	@echo file .config/autoconfig.h has been created!
	@if test $(AUTOCONFIG_PROJ_FILE) != "" ; then	\
		echo saving file .config to file $(AUTOCONFIG_PROJ_FILE);	\
		cp .config $(AUTOCONFIG_PROJ_FILE);	\
		make icf_cfg; \
	fi
	@rm -rf config.help

iar_script:
	@cp $(ULP_DIR)/ulp.ewd $(PRJ_DIR)/bldc.ewd
	@cp $(ULP_DIR)/ulp.ewp $(PRJ_DIR)/bldc.ewp
	@cp $(ULP_DIR)/ulp.eww $(PRJ_DIR)/bldc.eww
ifeq ($(CONFIG_TARGET_LIB),y)
	@cp $(ULP_DIR)/lib.ewp $(PRJ_DIR)/bldc.ewp
endif

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
	@rm -rf $(ENV_FILE)

clean: xconfig_clean
	@rm -rf $(TOP_DIR)/projects/bldc/Debug
	@rm -rf $(TOP_DIR)/projects/bldc/Release
	@rm -rf $(TOP_DIR)/projects/bldc/settings
	@rm -rf $(TOP_DIR)/projects/bldc/bldc.dep

distclean: clean iar_clr unconfig

%_config:
	@if test -r $(subst _config,.config,$@) ; \
	then \
		echo file $(subst _config,.config,$@) is used as template;\
	else \
		echo creating new file $(subst _config,.config,$@) from defconfig;\
		cp defconfig $(subst _config,.config,$@); \
	fi
	@echo AUTOCONFIG_PROJ_FILE = $(subst _config,.config,$@) > $(ENV_FILE)
	@rm -rf .config
	@cp -f $(subst _config,.config,$@) .config ; \
	make xconfig

# "make src/drivers/time.c.debug"
# self debug src/drivers/time.c with gcc
%.c.debug:
	@echo compiling $(subst .debug,,$@) with gcc ...;
	@gcc -std=c99 -o $(notdir $(subst .c.debug,.exe,$@)) $(subst .debug,,$@) -iquote $(TOP_DIR) -iquote $(TOP_DIR)/src/include/ -iquote ./  -D __SELF_DEBUG=1
