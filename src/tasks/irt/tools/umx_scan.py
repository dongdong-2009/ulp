#!/usr/bin/env python
#coding:utf8
#easy_install pyvisa
#ni visa driver should be installed, to be used as pyvisa backend

import visa

class dmm:
	def __init__(self, visa_name="USB0::0x0957::0x0607::MY53011514::INSTR"):
		self.rm = visa.ResourceManager()
		self.instr = rm.open_resource(visa_name)
		self.idn = self.instr.query("*idn?")
		self.reset()
		self.cls()

	def reset(self):
		self.instr.write("*rst")
	def cls(self):
		self.instr.write("*cls")
	def err(self):
		#such as: -230,"Data stale"
		return self.instr.query("system:err?")
	def disp(self, message=None):
		if message != None:
			self.instr.write('disp:window:text "%s"'%message)
		else:
			self.instr.write('disp:window:text:clear')

	def measure_res(self):
		return self.instr.query("meas:res?")
	def measure_dcv(self):
		return self.instr.query("meas:volt:dc?")
	def measure_diode(self):
		return self.instr.query("meas:diode?")
	def measure_beeper(self):
		#0-10R beep, 10R-1K2 disp value, >1K2 disp OPEN
		#open return +9.90000000E+37
		return self.instr.query("meas:CONTinuity?")

	def trig_ext_start(slope="pos", sdelay="auto", count="inf"):
		self.instr.write("outp:trig:slope %s"%"slope")
		self.instr.write("trig:slope %s"%"slope")
		#note: trig delay is useful except diode&CONTinuity mode
		if type(sdelay) != int:
			self.instr.write("trig:delay:auto on")
		else:
			self.instr.write("trig:delay %d"%sdelay)
		if type(count) != count:
			self.instr.write("trig:count %s"%count)
		else:
			self.instr.write("trig:count %d"%count)
		self.instr.write("trig:source ext")
		#dmm internal buffer will be cleared here
		self.instr.write("init")

	def poll(self):
		#return nr of points pending in dmm's internal buffer
		n = self.instr.query("data:points?")
		n = int(n)
		return n

	def read(self, npoints="all"):
		if type(npoints) != int:
			results = self.instr.query("r?")
		else:
			results = self.instr.write("r? %d"%npoints)
		return results

if __name__ == '__main__':
	def instr_list():
		rm = visa.ResourceManager()
		list = rm.list_resources()
		for instr in list:
			print(instr)

	from sys import *
	import signal

	def signal_handler(signal, frame):
		sys.exit(0)

	signal.signal(signal.SIGINT, signal_handler)
	argc = len(argv)
	if(argc == 1 or argv[1] == 'help'):
		print 'umx_scan list			list all instruments'
		print 'umx_scan query cmd		query instr with the specified command'
		quit();

	if argv[1] == 'list':
		instr_list();
	elif argv[1] == 'query':
		rm = visa.ResourceManager()
		instr = rm.open_resource('USB0::0x0957::0x0607::MY53011514::INSTR')
		print(instr.query(argv[2]))
	elif argv[1] == 'write':
		rm = visa.ResourceManager()
		instr = rm.open_resource('USB0::0x0957::0x0607::MY53011514::INSTR')
		print(instr.write(argv[2]))

