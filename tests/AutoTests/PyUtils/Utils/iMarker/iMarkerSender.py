#!/usr/bin/env python

# 

import zmq, dpkt, struct, optparse, Util, Logger, socket, os, re

# Default values
DEFAULT_SOCK='ipc:///tmp/backend.ipc'
DEFAULT_LOG_LEVEL = 5

def check_defined(parser, options):
  for opt, value in options.__dict__.items():
    if not value:
      parser.error("Required argument '%s' is undefined" % opt)

def request(data):
  try:
    http=dpkt.http.Request(data)
    Logger.log(20, '> %s %s' % (http.method, http.uri))
    return True
  except:
    return False

def response(data):
  try:
    http=dpkt.http.Response(data)
    content_type = http.headers['content-type']
    Logger.log(20, "< %s %s (Content-Type: '%s')" % (http.status, http.reason, content_type))
    return True
  except:
    return False


class SessionKey:

  def __init__(self, saddr, daddr, sport, dport):
    self.saddr = saddr
    self.daddr = daddr
    self.sport = sport
    self.dport = dport

  def invert( self ):
    return SessionKey(self.daddr, self.saddr, self.dport, self.sport)
 
  def get( self ):
    return self.saddr, self.daddr, self.sport, self.dport

  def unique( self ):
    return max(self.saddr, self.daddr), \
           min(self.saddr, self.daddr), \
           max(self.sport, self.dport), \
           min(self.sport, self.dport)

  def __str__( self ):
    return "%s:%s <-> %s:%s" % \
      (socket.inet_ntop(socket.AF_INET, self.saddr), self.sport, \
       socket.inet_ntop(socket.AF_INET, self.daddr), self.dport)

  def __eq__( self, other ):
    return self.unique() == other.unique()
  
  def __hash__( self ):
    return hash(self.unique())


class HttpSession:

  def __init__(self, key, data):
    self.key = key
    self.requests = []
    self.responses = []
    self.merge(key, data)

  def merge(self, key, data):
    if request(data):
      m = re.finditer(r'[A-Z]+ \S+ HTTP/1', data, re.MULTILINE | re.DOTALL)
      starts = map(lambda x: x.start(), m)
      ends = starts[1:] + [len(data)]
      msgs = map(lambda x: data[x[0]:x[1]], zip(starts, ends))
      for msg in msgs:
        self.requests.append(msg)
    elif response(data):
      for msg in data.split('HTTP/1'):
        if msg:
          self.responses.append('HTTP/1' + msg)

  def is_complete( self ):
    def flt(data):
      return True
      http=dpkt.http.Response(data)
      types = []
      content_type = http.headers.get('content-type', '')
      if isinstance(content_type, basestring):
        types.append(content_type)
      else:
        types+=content_type
      return http.status == '200' and len(filter(lambda x: 'text/html' in x, types))
    return len(self.requests) != 0 and len(filter(flt, self.responses)) != 0

  def data( self ):
    if self.is_complete():
      stream = map(lambda x: (x[0] or '')+ (x[1] or ''), zip(self.requests, self.responses))
      return ''.join(stream)
    return ''.join(self.requests) + ''.join(self.responses)

  def get(self, sessNum):
    return \
           struct.pack('=HBBBBQ', 1, \
                       ord(self.key.saddr[0]), \
                       ord(self.key.saddr[1]), ord(self.key.saddr[2]), \
                       ord(self.key.saddr[3]), sessNum) +  \
                       self.data()

def main():
  parser = optparse.OptionParser(add_help_option=False)
  parser.add_option(
    "--help", action="help", help="show this help message and exit")
  parser.add_option(
    "-d", "--tcp-flow-dir", dest="dir", help="Tcpfow directory")
  parser.add_option(
    "-s", "--socket", dest="socket", help="IMarker socket", default=DEFAULT_SOCK)
  parser.add_option(
    "-l", "--log_level", dest="log_level", help="log level", type="int", default=DEFAULT_LOG_LEVEL)
  (options, args) = parser.parse_args()
  check_defined(parser, options)

  Logger.initLog(options.log_level)

  # Prepare sessions from pcap file
  Logger.log(2, "Prepare HTTP sessions...")
  http_sessions = {}
  for filename in os.listdir(options.dir):
    m = re.search('([0-9]{3})\.([0-9]{3})\.([0-9]{3})\.([0-9]{3})\.([0-9]{5})-([0-9]{3})\.([0-9]{3})\.([0-9]{3})\.([0-9]{3})\.([0-9]{5})', filename)
    src = struct.pack('BBBB', int(m.group(1)), int(m.group(2)), int(m.group(3)), int(m.group(4)))
    sport = int(m.group(5))
    dst = struct.pack('BBBB', int(m.group(6)), int(m.group(7)), int(m.group(8)), int(m.group(9)))
    dport = int(m.group(10))
    key = SessionKey(src, dst, sport, dport)
    f = open(os.path.join(options.dir, filename), "rb")
    if http_sessions.has_key(key):
      http_sessions[key].merge(key, f.read())
    else:
      http_sessions[key] = HttpSession(key, f.read())

  Logger.log(2, "Prepare HTTP %d sessions done." % len(http_sessions))

  # Send to 0MQ socket
  Logger.log(2, "Send sessions...")
  context = zmq.Context()
  socket = context.socket(zmq.PUSH)
  socket.bind(options.socket)
  Logger.log(2, "Connect to '%s'" % options.socket)
  sessionNum = 0
  notComplete = 0
  requestNum = 0
  responseNum = 0
  for key, session in http_sessions.iteritems():
    try:
      if session.is_complete():
        Logger.log(4, "Send session#%d '%s' (%d, %d):\n'%s'" % \
                   (sessionNum, session.key, len(session.requests),
                    len(session.responses), session.data()))
        status = socket.send(session.get(sessionNum))
        sessionNum+=1
        requestNum+=len(session.requests)
        responseNum+=len(session.responses)
      else:
        # Logger.log(4, "Incomplete '%s':\n'%s'" % (session.key, session.data()))
        notComplete+=1
    except zmq.ZMQError, x:
      Logger.logException(1)
  Logger.log(2, "Send sessions: %d (%d/%d),  incomplete %d, total sessions %d" % \
             (sessionNum, requestNum, responseNum, notComplete, len(http_sessions)))
  

main()
