#
# bldc configuration written in linux configuration language
#
# Written by miaofng@2010
#

#use gpio as chip select
bool "PA4/SPI1_NSS as CS" CONFIG_SPI_CS_PA4
bool "PB1 as CS" CONFIG_SPI_CS_PB1
bool "PB10 as CS" CONFIG_SPI_CS_PB10
bool "PB12/SPI2_NSS as CS" CONFIG_SPI_CS_PB12
bool "PC4 as CS" CONFIG_SPI_CS_PC4
bool "PC5 as CS" CONFIG_SPI_CS_PC5

#for goldbull board TP_CS
bool "PC8 as CS" CONFIG_SPI_CS_PC8

bool "PF11 as CS" CONFIG_SPI_CS_PF11

#for vvt board dds
bool "PB0 as CS" CONFIG_SPI_CS_PB0

