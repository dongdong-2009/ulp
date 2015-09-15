#!/usr/bin/env python
#coding:utf8

import socket
import select
import Queue
from time import sleep
import sys

class Server:
	imsg = {}
	omsg = {}

	def __init__(self, saddr):
		print 'irt server init .. %s:%d '%saddr
		self._server = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
		self._server.setblocking(False)
		self._server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
		self._server.bind(saddr)
		self._server.listen(100)
		self._inputs = [self._server]
		self._outputs = []

	def __del__(self):
		for s in self._inputs:
			s.close()

	def _update(self):
		rs,ws,es = select.select(self._inputs, self._outputs, self._inputs, 0)
		if not (rs or ws or es):
			return

		for s in rs:
			if s is self._server:
				conn,addr = s.accept()
				#print 'connect by',addr
				conn.setblocking(False)
				self._inputs.append(conn)
				self.imsg[conn] = Queue.Queue()
				self.omsg[conn] = Queue.Queue()
			else:
				#print 'recv ok...'
				try:
					data = s.recv(1024)
				except socket.error:
					data = None

				if data:
					#print data
					self.imsg[s].put(data)
				else:
					#socket close
					if s in self._outputs:
						self._outputs.remove(s)
					#print 'remove rs: %d'%s.fileno()
					self._inputs.remove(s)

					s.close()
					del self.imsg[s]
					del self.omsg[s]
					if s in ws:
						ws.remove(s)

		for s in ws:
			try:
				msg = self.omsg[s].get_nowait()
			except Queue.Empty:
				self._outputs.remove(s)
			else:
				s.send(msg)

		for s in es:
			#print 'except ',s.getpeername()
			if s in self._inputs:
				#print 'remove es: %d'%s.fileno()
				self._inputs.remove(s)
			if s in self._outputs:
				self._outputs.remove(s)
			s.close()
			del self.imsg[s]
			del self.omsg[s]

	def recv(self):
		self._update()
		for s in self.imsg.keys():
			try:
				msg = self.imsg[s].get_nowait()
			except Queue.Empty:
				continue
			else:
				req = {}
				req["sock"] = s
				req["data"] = msg
				return req

	def send(self, req):
		s = req["sock"]
		self.omsg[s].put(req["data"])
		if s not in self._outputs:
			self._outputs.append(s)


