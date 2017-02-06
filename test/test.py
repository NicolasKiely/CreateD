#!/usr/bin/python
''' Prints command line arguments and striped input '''
import sys

for i, arg in enumerate(sys.argv):
    print str(i) + ": "+ arg

for line in sys.stdin:
    sline = line.strip()
    if len(sline) > 0:
        print sline
