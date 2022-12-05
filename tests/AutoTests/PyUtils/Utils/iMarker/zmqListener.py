#!/usr/bin/env python

import sys, zmq, Util, Logger, msgpack

def usage():
  print 'Usage: %s <socket>' % sys.argv[0]
  exit(1)

def main():
  argc = len(sys.argv)
  if argc != 2:
    usage()

  Logger.initLog(100)
  context = zmq.Context()
  socket = context.socket(zmq.PULL)
  socket.bind(sys.argv[1])

  Logger.log(2, "Connect to '%s'" % sys.argv[1])
  gotSession = 0
  while True:
    try:
      msg = socket.recv()
      Logger.log(4, "Session#%d start\n %s" % (gotSession, msg))
      try:
        unpacked = msgpack.unpackb(msg)
        Logger.log(4, "Session#%d '%s': %s" % (gotSession, unpacked.get('url', None), unpacked))
        gotSession += 1        
      except:
        # skip other formats
        pass
      Logger.log(4, "Session#%d finish" % gotSession)
    except zmq.ZMQError, x:
      Logger.logException(1)
      exit(1)
main()
  
  

