#!/usr/bin/env python
#coding:utf8

import os, sys, signal
# from nbstreamreader import NonBlockingStreamReader
from server import Server
import json
import shlex
import time

class Cmd:
	def __init__(self):
		self.cmd_list = {}
		self.register("help", self.cmd_help, "list all commands")

	def register(self, name, func, help=""):
		if hasattr(self.cmd_list, name):
			ref = self.cmd_list[name]["ref"]
			self.cmd_list[name]["ref"] = ref + 1;
			return

		entry = {"func": func, "help": help, "ref": 1}
		self.cmd_list[name] = entry

	def unregister(self, name):
		if hasattr(self.cmd_list, name):
			ref = self.cmd_list[name]["ref"]
			self.cmd_list[name]["ref"] = ref - 1
			if ref < 1:
				del(self.cmd_list[name])

	def run(self, cmdline):
		try:
			argv = shlex.split(cmdline)
		except ValueError as e:
			print str(e)
		else:
			argc = len(argv)
			if argc > 0:
				name = argv[0]
				if name in self.cmd_list:
					func = self.cmd_list[name]["func"]
					return func(argc, argv)
				elif "default" in self.cmd_list:
					func = self.cmd_list["default"]["func"]
					return func(argc, argv)

	def cmd_help(self, argc, argv):
		result = []
		for name,cmd in self.cmd_list.items():
			result.append("%s		%s\n\r"%(name, cmd["help"]))
		result = ''.join(result)
		return result

class Shell(Cmd):
	mode_auto = True
	cmdline = {}
	def __init__(self, saddr):
		#self._nbsr = NonBlockingStreamReader(sys.stdin)
		self._server = Server(saddr)
		Cmd.__init__(self)

	def update(self):
		time.sleep(0.01)
		req = {}
		# line = self._nbsr.readline()
		# if line:
			# req["sock"] = 0
			# req["data"] = line
			# self.process(req)
		# else:
		req = self._server.recv()
		if req:
			self.process(req)

	def process(self, req):
		sock = req['sock']
		if sock not in self.cmdline:
			self.cmdline[sock] = req['data']
		else:
			self.cmdline[sock] = self.cmdline[sock] + req['data']

		cmdline = self.cmdline[sock]
		tail = cmdline[-1]
		if (tail != '\n') and (tail != '\r'):
			self.mode_auto = False
			return

		del(self.cmdline[sock])
		result = self.run(cmdline)
		self.response(sock, result)

		if not self.mode_auto:
			self.response(sock, "> ")

	def response(self, sock, result):
		if result is None:
			return

		if not isinstance(result, basestring):
			if self.mode_auto:
				data = json.dumps(result)
			else:
				data = []
				for key, val in result.items():
					data.append("%s\t\t: %s\n\r"%(key, str(val)))
				data = ''.join(data)
		else:
			data = result

		req = {"sock": sock, "data": data}
		if sock != 0:
			self._server.send(req)
		else:
			print req["data"]

def signal_handler(signal, frame):
	print 'user abort'
	sys.exit(0)

#module self test
if __name__ == '__main__':
	signal.signal(signal.SIGINT, signal_handler)
	saddr = ('localhost', 10003)
	shell = Shell(saddr)
	while True:
		shell.update()

