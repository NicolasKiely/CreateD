import sys
print "Hello World"

for i, arg in enumerate(sys.argv):
    print str(i) + ": "+ arg

for line in sys.stdin:
    print line
