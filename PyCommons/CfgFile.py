
import re, os, os.path, shlex, copy
from Rec import Rec
from Util import *


class NoDefVal: pass
noDefVal = NoDefVal()


class CfgGrp:
  
  def __init__( self, defVal = noDefVal ):
    self._defVal = defVal

  def defVal( self ):
    return self._defVal

  def canBeDef( self ):
    return self._defVal is not noDefVal

  def error( self, lex, msg ):
    raise Exception('%s%s' % (lex.error_leader(), msg))

  def getToken( self, lex ):
    token = lex.get_token()
    if token and token[0] == token[-1]:
      if token[0] in ['"', "'"]:
        return token[1:-1]
    return token


class CfgStr(CfgGrp):

  def read( self, lex ):
    token = self.getToken(lex)
    #if token == '': self.error(lex, 'String value expected')
    return token

  def write( self, f, val, ofs ):
    f.write(val)


class CfgQuotedStr(CfgStr):

  def write( self, f, val, ofs ):
    f.write('"%s"' % val)
  

# expand env vars in readed str, such as $HOME
class CfgEnvStr(CfgStr):

  def read( self, lex ):
    return os.path.expandvars(CfgStr.read(self, lex))
    

class CfgInt(CfgGrp):

  def read( self, lex ):
    token = self.getToken(lex)
    try:
      if token == 'none': return None
      return int(token)
    except Exception, x:
      self.error(lex, 'Invalid integer: "%s"' % token)

  def write( self, f, val, ofs ):
    if val is None:
      f.write('none')
    else:
      f.write('%d' % val)


class CfgBool(CfgGrp):

  def read( self, lex ):
    token = self.getToken(lex)
    if token == 'true' : return True
    if token == 'false': return False
    self.error(lex, '"true" or "false" expected')

  def write( self, f, val, ofs ):
    if val:
      f.write('true')
    else:
      f.write('false')


class CfgAddr(CfgGrp):

  def read( self, lex ):
    token = self.getToken(lex)
    try:
      return str2addr(token)
    except Exception, x:
      self.error(lex, str(x))

  def write( self, f, val, ofs ):
    f.write(addr2str(val))


class CfgTime(CfgGrp):

  def read( self, lex ):
    token = self.getToken(lex)
    try:
      return str2time(token)
    except Exception, x:
      self.error(lex, str(x))

  def write( self, f, val, ofs ):
    f.write('"%s"' % time2str(val))


class CfgList(CfgGrp):

  def __init__( self, type, defVal = [] ):
    CfgGrp.__init__(self, defVal)
    self._type = type

  def read( self, lex ):
    list = []
    token = lex.get_token()
    if token != '[': self.error(lex, 'Expected "[", but got "%s"' % token)
    token = lex.get_token()
    if token == ']': return list
    lex.push_token(token)
    while True:
      list.append(self._type.read(lex))
      token = lex.get_token()
      if token == ']': return list
      if token == '': self.error(lex, 'Expected "]"')
      if token != ';': self.error(lex, 'Expected "]", but got "%s"' % token)

  def write( self, f, val, ofs ):
    f.write('[')
    for isLast, v in withLastTag(val):
      self._type.write(f, v, ofs + 1)
      if not isLast: f.write(';')
    f.write(']')


class CfgTuple(CfgGrp):

  def __init__( self, fields, defVal = noDefVal ):
    CfgGrp.__init__(self, defVal)
    self._fields = fields

  def read( self, lex ):
    val = []
    token = lex.get_token()
    if token != '(': self.error(lex, 'Expected "(", but got "%s"' % token)
    for isLast, t in withLastTag(self._fields):
      val.append(t.read(lex))
      token = lex.get_token()
      if isLast:
        if token != ')': self.error(lex, 'Expected ")", bug got "%s"' % token)
      else:
        if token != ',': self.error(lex, 'Expected ",", bug got "%s"' % token)
    return tuple(val)

  def write( self, f, val, ofs ):
    f.write('(')
    for isLast, (t, v) in withLastTag(zip(self._fields, val)):
      t.write(f, v, ofs)
      if not isLast: f.write(',')
    f.write(')')


class CfgRec(CfgGrp):

  def __init__( self, fields, defVal = noDefVal ):
    CfgGrp.__init__(self, defVal)
    self._fldNames = []
    self._fields = {}
    for (name, t) in fields:
      self._fldNames.append(name)
      self._fields[name] = t

  def defVal( self ):
    value = {}
    for name, t in self._fields.items():
      value[name] = t.defVal()
    return Rec(value)

  def readValues( self, lex ):
    record = {}
    token = self.getToken(lex)
    while token not in ['}', '']:
      if token == ';': token = self.getToken(lex)
      name = token
      t = self._fields.get(name)
      if not t: self.error(lex, 'Unknown parameter "%s"' % name)
      token = lex.get_token()
      if token != '=': self.error(lex, 'Expected "=", but got "%s"' % token)
      val = t.read(lex)
      record[name] = val
      token = self.getToken(lex)
    lex.push_token(token)
    for name, t in self._fields.items():
      if name not in record:
        if t.canBeDef():
          record[name] = t.defVal()
        else:
          self.error(lex, 'Parameter "%s" is not defined' % name)
    return Rec(record)

  def read( self, lex ):
    token = lex.get_token()
    if token != '{': self.error(lex, 'Expected "{", but got "%s"' % token)
    val = self.readValues(lex)
    token = lex.get_token()
    if token != '}': self.error(lex, 'Expected "}", but got "%s"' % token)
    return val

  def writeValues( self, f, val, ofs ):
    if isinstance(val, Rec): val = val.get()
    for name in self._fldNames:
      v = val[name]
      f.write('%s%s = ' % (' ' * ofs, name))
      self._fields[name].write(f, v, ofs + 1)
      if name != self._fldNames[-1]: f.write(';\n')

  def write( self, f, val, ofs ):
    f.write('{\n')
    self.writeValues(f, val, ofs)
    f.write('\n%s}' % (' ' * ofs))


# represent (save and load) dictionary as list of records, keys can be tuples
class CfgMultiSet(CfgGrp):

  # keys and fields must be lists of (name, t) pairs, where t is instance if CfgGrp subclass
  def __init__( self, keys, fields, defVal = {} ):
    CfgGrp.__init__(self, defVal)
    self.keys = keys
    self.list = CfgList(CfgRec(keys + fields))

  def read( self, lex ):
    list = self.list.read(lex)
    d = {}
    for r in list:
      key = []
      for name, t in self.keys:
        key.append(r.get(name))
        delattr(r, name)
      if len(key) > 1: key = tuple(key)  # list to tuple
      else: key = key[0]   # single value
      d[key] = r
    return d

  def write( self, f, val, ofs ):
    l = []
    for key, r in val.items():
      v = copy.deepcopy(r)
      if len(self.keys) == 1:
        setattr(v, self.keys[0][0], key)   # single key
      else:
        assert len(key) == len(self.keys)  # check key len
        for k, (name, t) in zip(key, self.keys):
          setattr(v, name, k)
      l.append(v)
    self.list.write(f, l, ofs)


class CfgSet(CfgMultiSet):

  def __init__( self, keyName, keyT, fields, defVal = {} ):
    CfgMultiSet.__init__( self, [(keyName, keyT)], fields, defVal)

                
class CfgFile:

  # params must be list of tuple (name, type) where type is CfgGrp subclass instance
  def __init__( self, params, path, load=True, failIfAbsent=True, value = None ):
    self._rec = CfgRec(params)
    self._path = path
    self._value = self._rec.defVal()
    if value is not None:
      assert isinstance(value, Rec)
      for name, val in value.get().items():
        self.set(name, val)
    if load: self.load(failIfAbsent)

  def get( self, name ):
    return getattr(self._value, name)
    
  def set( self, name, val ):
    assert name in self._rec._fields
    setattr(self._value, name, val)
    
  def __getattr__( self, name ):
    return self.get(name)

  def load( self, failIfAbsent = True, failIfInvalid = True ):
    if not os.access(self._path, os.F_OK):
      if failIfAbsent:
        raise Exception('Error opening config file "%s"' % self._path)
      else:
        return False
    f = open(self._path)
    try:
      try:
        lex = shlex.shlex(f, self._path)
        lex.wordchars += '/*:-.$'
        self._value = self._rec.readValues(lex)
      except:
        if failIfInvalid: raise
        return False
    finally:
      f.close()
    return True

  def save( self ):
    f = open(self._path, 'w')
    try:
      print >> f, '# Automatically generated file. Do not edit!'
      print >> f
      self._rec.writeValues(f, self._value, 0)
      f.write('\n')
    finally:
      f.close()
