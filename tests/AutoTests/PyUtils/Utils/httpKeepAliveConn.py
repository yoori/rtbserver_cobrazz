#!/usr/bin/env python

import os, sys, signal, httplib
from optparse import OptionParser
from Shutdown import shutdown, ShuttingDown
from Logger import log, initLog, setLogLevel, logException

PyCommons = os.path.abspath('../../../../PyCommons')
sys.path.insert(0, PyCommons)
sys.path.insert(0, '..')

DEFAULT_PORT=10080
DEFAULT_CONNECTIONS=2048

class HTTPKeepAliveConnection:

  DEFAULT_REQUEST='/templates/UnitTests/simple.html'
# DEFAULT_REQUEST='/services/nslookup?xinfopsid=0&random=0&v=1.3.0-3.ssv1&app=PS&require-debug-info=body'

  def __init__( self, num, host, port ):
    self.num = num
    self.conn = httplib.HTTPConnection('%s:%d' % (host, port))

  def send_request( self ):
    headers = {"Connection": "Keep-Alive",
               "Accept": "text/html"}
    try:
      self.conn.request("GET", HTTPKeepAliveConnection.DEFAULT_REQUEST, headers = headers)
      response = self.conn.getresponse()
      if response.status != 200 :
        log(3, "Connection#%d got unexpected status %d (%s)" % \
            (self.num, response.status, response.reason))
        shutdown.set()
        return False
      response.read()
      return True
    except:
      log(3, "Connection#%d got error" % self.num)
      logException(1)
      shutdown.set()
      return False

  def close( self ):
    self.conn.close()

def terminate(sigNum, frame):
  shutdown.set()

def check_defined(parser, options):
  for opt, value in options.__dict__.items():
    if not value:
      parser.error("Required argument '%s' is undefined" % opt)

def main():
  signal.signal(signal.SIGTERM,  terminate)
  initLog(100)
  parser = OptionParser(add_help_option=False)
  parser.add_option(
    "--help", action="help", help="show this help message and exit")
  parser.add_option(
    "-h", "--host", dest="host", help="Apache host")
  parser.add_option(
    "-p", "--port", dest="port", help="Apache port", type="int", default=DEFAULT_PORT)
  parser.add_option(
    "-c", "--connections", dest="connections", type="int",
    help="HTTP connections count", default=DEFAULT_CONNECTIONS)
  (options, args) = parser.parse_args()
  check_defined(parser, options)

  conns = [HTTPKeepAliveConnection(i, options.host, options.port) \
           for i in range(options.connections)]

  for conn in conns:
    if not conn.send_request():
      break

  try:
    shutdown.wait()
  except KeyboardInterrupt:
    pass

  # Clear connections
  for conn in conns:
    conn.close()
  conns = []

main()





