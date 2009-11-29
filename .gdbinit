#
# J-LINK GDB SERVER initialization
#
# This connects to a GDB Server listening
# for commands on localhost at tcp port 2331
target remote 192.168.1.4:2331
monitor flash device = STM32F103RB
monitor flash download = 1
monitor flash breakpoints = 1

# Set JTAG speed to 30 kHz
monitor endian little
monitor speed 30

#reset target
monitor reset
monitor sleep 10

#perform peripheral reset
#disable watchdog
#init PLL
monitor reg r13 = 0x00000000
monitor reg pc = 0x00000004

#setup GDB for faster downloads
#set remote memory-write-packet-size 4096
#set remote memory-write-packet-size fixed
monitor speed 2000

file main.elf
break Reset_Handler
load
#monitor go


