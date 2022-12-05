
import sys, time, string, os.path, os

cmdparams = {}

def normCmdName(cmdname):
  cmdnamelen = len(cmdname)
  if cmdnamelen > 0:
    if cmdname[0] == '-':
      return normCmdName(cmdname[1:])
  return cmdname

def parseCmds():
  ret = {}
  for arg in sys.argv:
    if arg.find('=') != -1:
      parts = arg.split('=')
      value = ''
      if len(parts) > 1:
        value = parts[1]
      if len(parts) > 0:
        name = normCmdName(parts[0])
        ret[name] = value
  return ret

cmdparams = parseCmds()

def getVar(envname, cmdname, default):
  cmd = normCmdName(cmdname)
  if cmdparams.has_key(cmd) and len(cmdparams[cmd]) > 0:
    return cmdparams[cmd]
  elif os.environ.has_key(envname) and len(os.environ[envname]) > 0:
    return os.environ[envname]
  else:
    return default


# use command line parameter as:
# --name=value or -name=value or name=value or [-[-]]name='value value'

HOME = getVar('HOME', 'home', '~')
USER = getVar('USER', 'user', '')
HOST = getVar('HOSTNAME', 'host', 'xen')

RUNDIR = getVar('RUNDIR' , 'rundir', HOME + '/projects/foros/run/trunk')
BINDIR = getVar('BINDIR' , 'bindir', os.path.abspath('../../../../build/bin'))
XSDDIR = getVar('XSDDIR', 'xsddir', os.path.abspath('../../../../xsd'))
PORT_BASE = int(getVar('PORT_BASE', 'portbase', 28400))

# Oracle connection
ORA_DBSERVER = getVar('ORA_DBSERVER', 'ora_dbserver', '//oraclept/addbpt.ocslab.com')
ORA_DB = getVar('ORA_DB', 'ora_db', 'ads_1')
ORA_DBPWD = getVar('ORA_DBPWD', 'ora_dbpwd', 'adserver')

# Postgres connection
PQ_HOST = getVar('PQ_HOST', 'pq_host', 'stat-dev1')
PQ_DB = getVar('PQ_DB', 'pq_db', 'ads_1')
PQ_PORT = getVar('PQ_PORT', 'pq_port', 5432)
PQ_USER = getVar('PQ_USER', 'pq_user', 'foros')
PQ_DBPWD = getVar('PQ_DBPWD', 'pq_dbpwd', 'adserver')


