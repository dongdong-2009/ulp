#!/usr/bin/env python
#coding:utf8
#linktron techology matrix driver
#miaofng@2015-8-31 initial version
#note:
#1, execute() must be called if pipeling is enabled
#	vm queue full error is handled by execute's inner wait loop
#2, only HV related ecode will be return by execute(),
#	others exception is raised
#3, simplify function could be used to remove dummy
#	relay "open/close" operaions between "route scan"

import sys, os, signal
import serial #http://pythonhosted.org/pyserial/
import re
import functools
import time

class MatrixIoError(Exception):
	def __init__(self, echo):
		self.echo = echo

class Matrix:
	timeout = 5 #unit: S
	cmdline_max_bytes = 128
	IRT_E_VM_OPQ_FULL = -16
	IRT_E_HV_UP = -7
	IRT_E_HV_DN = -8
	IRT_E_HV = -9

	def __del__(self):
		if self.uart is not None:
			self.query("shell -m", echo=False)
			self.uart.close()
			self.uart = None

	def __init__(self, port='COM6', baud=115200):
		self.uart = serial.Serial(port, baud, self.timeout)
		self.query("shell -a", echo=False)
		self.opq = []
		self.pipeling_enable = False
		self.pipeling_simplify = False

	def pipeling(self, enable=True, autosimplify=True):
		''' pipling mode means:
		in case of pipeling mode, ops will be gathered
		in operation queue until "excute()" is called

		autosimplify effects only when pipeling enabled
		auto remove identical operation and reversed operation
		except "SCAN"/"FSCN"
		'''
		assert len(self.opq) == 0
		self.pipeling_enable = enable
		self.pipeling_simplify = enable and autosimplify

	def abort(self):
		pass

	def reset(self):
		self.query("*RST")
		self.opq = []
		self.opt = None

	def cls(self):
		echo = self.query("*CLS")
		ecode = self.retval(echo)
		assert ecode == 0

	def err(self):
		echo = self.query("*ERR?")
		ecode = self.retval(echo)
		return ecode

	def arm(self, arm):
		echo = self.query("ROUTE ARM %d"%arm)
		ecode = self.retval(echo)
		assert ecode == 0

	def opc(self, arm):
		echo = self.query("*OPC?")
		opc = self.retval(echo)
		return opc

	def open(self, bus0, line0, line1=None):
		return self.switch("OPEN", bus0, line0, line1)

	def close(self, bus0, line0, line1=None):
		return self.switch("CLOS", bus0, line0, line1)

	def scan(self, bus0, line0, line1=None):
		return self.switch("SCAN", bus0, line0, line1)

	def switch(self, opt, bus0, line0, line1=None):
		ops = {"opt": opt, "bus0": bus0, "line0": line0, "bus1": bus0, "line1": line1}
		self.opq.append(ops)
		if self.pipeling_enable:
			if self.pipeling_simplify:
				self.simplify()
		else:
			#deadloop until matrix return "<+0", or error occurs
			ecode = self.execute()
			self.opq = []
			return ecode

	def simplify(self):
		index = 0
		ops_cur = None
		for ops in reversed(self.opq):
			index = index - 1
			if ops["opt"] == "SCAN" or ops["opt"] == "FSCN":
				break

			if ops_cur is None:
				ops_cur = ops
				continue

			#seq type simplify not supported yet
			if ops["line1"] is not None:
				continue

			if ops["bus0"] == ops_cur["bus0"] and ops["line0"] == ops_cur["line0"]:
				if ops["opt"] == ops_cur["opt"]:
					del(self.opq[index])
				else:
					del(self.opq[index])
					del(self.opq[-1])
				break

	def execute_once(self, wait=None):
		paras = []
		bytes = 16 #ROUTE CLOS (@)
		opt = None
		index = 0
		for ops in self.opq:
			index = index + 1
			if opt is None:
				opt = ops["opt"]

			if ops["opt"] == opt:
				cmdline = "%02d%04d"%(ops["bus0"], ops["line0"])
				if ops["line1"] is not None:
					cmdline = cmdline + ":%02d%04d"%(ops["bus1"], ops["line1"])

				paras.append(cmdline)
				bytes = bytes + len(cmdline) + 1 #+','
				if bytes > self.cmdline_max_bytes:
					break
			else:
				break

		cmdline = ",".join(paras)
		cmdline = "ROUTE %s (@%s)"%(opt, cmdline)
		while True:
			echo = self.query(cmdline)
			ecode = self.retval(echo)
			if ecode == 0: #success, remove from queue
				self.opq = self.opq[index:]
				return 0
			elif ecode == self.IRT_E_VM_OPQ_FULL:
				if wait is not None:
					wait()
				continue
			elif ecode == self.IRT_E_HV_UP:
				return ecode
			elif ecode == self.IRT_E_HV_DN:
				return ecode
			elif ecode == self.IRT_E_HV:
				return ecode
			else:
				raise MatrixIoError(echo)

	def execute(self, wait=None):
		while len(self.opq) > 0:
			ecode = self.execute_once(wait)
			if ecode is not None:
				return ecode

	def query(self, command, echo=True):
		self.uart.flushInput()
		self.uart.write(command+"\n\r")
		if echo:
			echo = self.uart.readline()
			#echo = "<+0"
			return echo

	def retval(self, echo):
		match = re.search("^<[+-]\d*", echo)
		if match is not None:
			match = match.group()
			if len(match) > 2:
				ecode = int(match[1:])
				return ecode
		raise MatrixIoError(echo)

if __name__ == '__main__':
	def cmd_query(matrix, argc, argv):
		command = " ".join(argv)
		echo = "query: "+command + "\n\r"
		echo = echo + matrix.query(command)
		return echo

	def cmd_switch(matrix, argc, argv):
		if argc >= 3:
			op = argv[0]
			bus = int(argv[1])
			line0 = int(argv[2])
			if argc == 4:
				line1 = int(argv[3])
			else:
				line1 = None

			func = {"open": matrix.open, "close": matrix.close, "scan": matrix.scan}[op]
			ecode = func(bus, line0, line1)
			return str(ecode)+"\n\r"
		return "open/close/scan bus line\n\r"

	def cmd_test(matrix, argc, argv):
		sdelay = 0.1
		loops = 1
		if argc > 1:
			sdelay = float(argv[1])
		if argc > 2:
			loops = int(argv[2])

		now = time.time()
		for i in range(0, loops):
			for line in range(0, 32):
				for bus in range(0, 4):
					matrix.close(bus, line)
					if sdelay != 0:
						time.sleep(sdelay)
					matrix.open(bus, line)
		now = time.time() - now
		now = now * 1000 / 160.0 / loops
		return "%.1fmS/operation\n\r"%now

	def signal_handler(signal, frame):
		sys.exit(0)

	from shell import Shell
	signal.signal(signal.SIGINT, signal_handler)
	matrix = Matrix("COM6", 115200)
	saddr = ('localhost', 10003)
	shell = Shell(saddr)
	shell.register("default", functools.partial(cmd_query, matrix), "query matrix")
	shell.register("open", functools.partial(cmd_switch, matrix), "relay open")
	shell.register("close", functools.partial(cmd_switch, matrix), "relay close")
	shell.register("scan", functools.partial(cmd_switch, matrix), "relay scan")
	shell.register("test", functools.partial(cmd_test, matrix), "test")

	while True:
		shell.update()