#!/usr/bin/env python
#coding:utf8
#easy_install pyvisa
#ni visa driver should be installed, to be used as pyvisa backend

import sys, os, signal
import visa
from shell import Shell

scpi_visa = "USB0::0x0957::0x0607::MY53011514::INSTR"
scpi_instr = None

def cmd_list(argc, argv):
	rm = visa.ResourceManager()
	list = rm.list_resources()
	result = []
	for instr in list:
		result.append(instr+"\n\r")
	result = "".join(result)
	return result

def cmd_select(argc, argv):
	global scpi_visa
	global scpi_instr
	if argc > 1:
		try:
			rm = visa.ResourceManager()
			instr = rm.open_resource(argv[1])
		except visa.VisaIOError as e:
			scpi_instr = None
			result = "open(%s)\n\r"%argv[1]
			result = result + str(e)+"\n\r"
			return result
		scpi_visa = argv[1]
		scpi_instr = instr
	result = "scpi_visa = %s\n\r"%scpi_visa
	return result

def cmd_debug(argc, argv):
	del(argv[0])
	scpi = " ".join(argv)
	if scpi.find('?') != -1:
		return '%s("%s")\n\r'%('scpi_query', scpi)
	else:
		return '%s("%s")\n\r'%('scpi_write', scpi)

def cmd_default(argc, argv):
	global scpi_visa
	global scpi_instr
	if scpi_instr is None:
		try:
			rm = visa.ResourceManager()
			scpi_instr = rm.open_resource(scpi_visa)
		except visa.VisaIOError as e:
			scpi_instr = None
			return str(e)+"\n\r"
	try:
		scpi = " ".join(argv)
		if scpi.find('?') != -1:
			result = scpi_instr.query(scpi)
		else:
			result = scpi_instr.write(scpi)
	except visa.VisaIOError as e:
		result = e

	result = str(result) + "\n\r"
	return result

def signal_handler(signal, frame):
	sys.exit(0)

if __name__ == '__main__':
	signal.signal(signal.SIGINT, signal_handler)
	saddr = ('localhost', 10003)
	shell = Shell(saddr)
	shell.register("list", cmd_list, "list all instruments")
	shell.register("select", cmd_select, "select/show current instrument")
	shell.register("debug", cmd_debug, "scpi software debug")
	shell.register("default", cmd_default, "scpi write&query")
	while True:
		shell.update()

