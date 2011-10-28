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
                # print "Receive %d bytes" %n
                if n:
                    data = data + self.l_serial.read(n)
                    #for i in xrange(len(data)):
                     #   print '%s'  % data[i]
            except Exception,ex:
                print str(ex)
        return data
            

if __name__ == '__main__':
    rt = ComThread(3,115200)
    #rt.set_option(11,115200)
    try:
        if rt.start():
            while True:
                pass
            rt.stop()
        else:
            pass        
    except Exception,se:
        print 'sssssssss'
        print str(se)


    if rt.alive:
        rt.stop()
    print ''
    print 'End OK .'
    del rt
