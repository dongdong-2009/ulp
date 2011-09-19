#! /usr/bin/env python

import serial

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
        self.l_serial.timeout = 2
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

if __name__ == '__main__':
    rt = ComThread(2,115200)
    rt.set_option(3,9600)
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
