obj-y += lm3s_adc.o
obj-y += lm3s_can.o
obj-y += lm3s_comp.o
obj-y += lm3s_cpu.o
obj-y += lm3s_epi.o
obj-y += lm3s_ethernet.o
obj-y += lm3s_flash.o
obj-y += lm3s_gpio.o
obj-y += lm3s_hibernate.o
obj-y += lm3s_i2c.o
obj-y += lm3s_i2s.o
obj-y += lm3s_interrupt.o
obj-y += lm3s_mpu.o
obj-y += lm3s_pwm.o
obj-y += lm3s_qei.o
obj-y += lm3s_ssi.o
obj-y += lm3s_sysctl.o
obj-y += lm3s_systick.o
obj-y += lm3s_timer.o
obj-y += lm3s_uart.o
obj-y += lm3s_udma.o
obj-y += lm3s_usb.o
obj-y += lm3s_watchdog.o

#rename original file name to lm3s_xxx
rename:
	@for file in $(patsubst lm3s_%.o, %.c, $(obj-y)); do\
		echo $$file "->" $(join lm3s_, $$file); \
		mv -f $$file $(join lm3s_, $$file); \
	done
