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

    def connect(self):
        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.socket.connect( (self.host, self.port) )
        (_, banner) = self.readline(100)
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
    
    def loop(self):
        if self.connect():
            print "Running script"
            for cmd in self.script:
                print "cmd: '%s'" % cmd
                self.socket.sendall(cmd + "\n")
                (code, msg) = self.readline()
                code = int(code)
                if code == 300:
                    (t, data) = self.readdata(msg)
                    print "type: %s" % t
                    print data
                    (code, msg) = self.readline(200)
                elif code != 200:
                    print "?? %s %s" % (code, msg)
                    break
            self.socket.sendall("QUIT\n");
            self.socket.close()
        else:
            print "Connect failed"
        
if __name__ == "__main__":
    client = ObelixClient('localhost', 14400, 
      ( "PATH /home/jan/projects/obelix/test", "RUN helloclient" )
    )
    client.loop()
