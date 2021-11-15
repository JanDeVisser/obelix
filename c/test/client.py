#
# test/client.py - Copyright (c) 2015 Jan de Visser <jan@finiandarcy.com>  
#  
# This file is part of Obelix.
#
# Obelix is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# Obelix is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Obelix.  If not, see <http://www.gnu.org/licenses/>.
#

import socket
import sys

class ObelixClient(object):

    def __init__(self, host, port, script):
        self.host = host
        self.port = port
        self.script = script
        self.lines = None
        self.cookie = None

    def connect(self):
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.connect( (self.host, self.port) )
        (_, banner) = self.readline(100)
        (_, banner) = banner.split(" ", 1)
        (_, ready) = self.readline(200)
        print "Connected to %s" % banner
        return ready == "READY"
        
    def readline(self, expected = None):
        line = self.nextline()
        if line:
            (code, msg) = line.split(" ", 1)
            if expected and (int(code) != expected):
                raise Exception("Unexpected code %s" % code)
            else:
                return (code, msg.strip())
        else:
            return None
                
    
    def nextline(self):
        ret = None
        while not ret:
            if not self.lines or (len(self.lines) <= self.ix):
                buf = self.socket.recv(8192)
                self.lines = buf.splitlines()
                self.ix = 0
            if len(self.lines) > self.ix:
                ret = self.lines[self.ix]
                self.ix += 1
        return ret
    
    def readdata(self, meta):
        (_, size, t) = meta.split(" ", 2)
        data = self.socket.recv(int(size))
        return (t, data)
    
    def sendcmd(self, cmd):
        print "cmd: '%s'" % cmd
        self.socket.sendall(cmd + "\n")
        (code, msg) = self.readline()
        code = int(code)
        if code == 300:
            (t, data) = self.readdata(msg)
            print "%s [%s] %s" % (code, t, data)
            (code, msg) = self.readline(200)
        elif code == 800:
            (_, self.cookie) = msg.split(" ", 1)
            print "%s %s" % (code, msg)
            (code, msg) = self.readline(900)
        print "%s %s" % (code, msg)
    
    def loop(self, script = None):
        if not script:
           script = self.script
        if self.connect():
            print "-- running script"
            if self.cookie:
                self.sendcmd("ATTACH %s" % self.cookie)
            for cmd in script:
                self.sendcmd(cmd)
            self.sendcmd("DETACH")
            self.socket.close()
            print "-- script finished"
        else:
            print "Connect failed"
        
if __name__ == "__main__":
    client = ObelixClient('localhost', 14400, 
      (
        "PATH C:/Users/jan/projects/obelix/test", 
        "RUN helloclient",
        "EVAL x = 2",
        "EVAL f = lambda(phi) return phi(2) end return f(lambda(x) return 2*2 end)",
        "EVAL return x"
      )
    )
    client.loop()
    
    client.loop( ( "EVAL return x", ) )
