#! /usr/bin/env python

import serial
import time

class ComThread:
    def __init__(self, Port ,Baudrate, value = None):
        self.l_serial = None
        self.alive = False
        self.port = Port
        self.baudrate = Baudrate
        self.date = value

    def send(self ,data):
        self.l_serial.write(data)
        pass
    def get_option(self):
        return self.alive

    def start(self):
        self.l_serial = serial.Serial()
        self.l_serial.port = self.port
        self.l_serial.baudrate = self.baudrate
        self.l_serial.timeout = 0.5   #read time out 
        self.l_serial.open()
        if self.l_serial.isOpen():
            self.alive = True
            return True
        else:
            return False
    def set_option(self, Port ,Baudrate):
        self.port = Port
        self.baudrate = Baudrate

    def stop(self):
        self.alive = False
        if self.l_serial.isOpen():
            self.l_serial.close()
    def read(self):
        if self.alive:
            time.sleep(0.2)
            try:
                data = ''
                n = self.l_serial.inWaiting()
                if n:
                    data = data + self.l_serial.read(n)
            except Exception,ex:
                print str(ex)
        return data
