#!/usr/bin/python

import sys

print("Hello %s!" % "Asterix")

print >> sys.stdout, "Hello from stdout, %s!" % "Asterix"
print >> sys.stderr, "Hello from stderr, %s!" % "Asterix"

