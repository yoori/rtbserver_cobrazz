import copy
import StringIO
import sys

typeIndent = 13
indexSuffix = '_'

class IndentStream:
    def __init__ (self, stream, level = 0):
        self.stream = stream
        self.level  = 2*level

    def addLevel (self):
        self.level = self.level + 2
    
    def delLevel (self):
        self.level = self.level - 2

    def newline (self):
        self.stream.write('\n')
    
    def write (self, string):
        """method need implemented"""
        self.stream.write(' '*self.level)
        self.stream.write(string)
    
    def rawwrite (self, string):
        self.stream.write(string)

    def writelines (self, string, indent = 0):
        for s in string.split('\n'):
            self.stream.write(' '*indent)            
            self.stream.write(s)
            self.newline()

class Index:

    name = 'Index'
    
    def __init__(self, fields, seqName = None):
        self.fields     = fields
        self.seqName    = seqName
        self.suffix     = indexSuffix
    
    def __copy__(self):
        return Index(copy.copy(self.fields), self.seqName)
    
    def getMakeStr (self):
        seq = ''
        if self.seqName:
            seq =  ", '%s'" % self.seqName
        if len(self.fields) == 1:
            return "%s([%s]%s)"%(self.name, self.fields[0].getRawMakeStr(), seq)
        out = StringIO.StringIO()
        print >>out, "["
        for field in self.fields:
            print >>out, "    %s," % field.getMakeStr()
        out.write("  ] %s" % seq)
        init = out.getvalue()
        out.close()
        return "%s(%s)" % (self.name, init)
    
    def merge (self, fromObject):
        my_fields = {}
        for field in self.fields:
            my_fields[field.dbName] = field
        for from_field in fromObject.fields:
            if my_fields.has_key(from_field.dbName):
                my_fields[from_field.dbName].merge(from_field)
        if self.seqName is None:
          self.seqName = fromObject.seqName
          
        
    def indexesCount (self):
        return len(self.fields)

    def varsList(self, start = 0):
        """declaration of self fields as args list"""
        fields = self.fields[start:]
        return reduce(lambda x, y: '%s, %s' % (x, y.name),
                      fields[1:], '%s' % (fields[0].name))
    
    def dbVarsList (self):
        """list of db vars"""
        return reduce(lambda x, y: '%s, %s ' % (x, y.dbName), self.fields[1:],
                     ' %s'%self.fields[0].dbName)

    def dbVarsIndexList (self):
        """list of db vars for assign in query but only indexes without assigment"""
        return reduce(lambda x, y: ('%s, :i%i' % (x[0], x[1]+1), x[1]+1),
                      self.fields[1:],
                      (':i1', 1))[0]

    def dbVarsAssignDeclList (self):
        """list of db vars for assign in query with assigment"""
        return reduce(lambda x, y: ('%s AND %s = :i%i' % (x[0], y.dbName, x[1]+1), x[1]+1),
                      self.fields[1:],
                      ('%s = :i%i' % (self.fields[0].dbName, 1), 1))[0]

    def dbVarsAssignValList (self):
        """List of db vars for assign in query with actual values"""
        return reduce(lambda x, y: '%s AND %s = " + strof(%s%s) + "' % (x, y.dbName, y.name, self.suffix),
                      self.fields[1:],
                      '%s = " + strof(%s%s) + "' % (self.fields[0].dbName, self.fields[0].name, self.suffix))

    def varsLShiftList (self, prefix = ''):
        """list of class members for stream lshift"""
        return reduce(lambda x, y: '%s << %s%s%s' %(x, prefix, y.name, self.suffix), self.fields, '')

    def varsRShiftList (self, prefix = ''):
        """list of class members for stream rshift"""
        return reduce(lambda x, y: '%s >> %s%s%s' %(x, prefix, y.name, self.suffix), self.fields, '')

    def varsInitList(self):
        """list of initialisation of class members, use connection if need"""
        return reduce(lambda x, y: '%s, %s%s(%s)' % (x, y.name, self.suffix,
                                                     y.getInit('connection', y.name)),
                      self.fields[1:],
                      '%s%s(%s)' % (self.fields[0].name, self.suffix,
                                    self.fields[0].getInit('connection', self.fields[0].name)))

    def fieldsInitList(self):
        """list of initialisation of class members, use connection if need"""
        return reduce(lambda x, y: '%s, %s(%s)' % (x, y.name,
                                                     y.getInit('connection', y.name)),
                      self.fields[1:],
                      '%s(%s)' % (self.fields[0].name,
                                    self.fields[0].getInit('connection', self.fields[0].name)))

    def varsCopyList(self, prefix):
        """list of copy initialisation of class members"""
        return reduce(lambda x, y: '%s, %s%s(%s)' % (x, y.name, self.suffix,
                                                     prefix+'.'+y.name+'_'),
                      self.fields[1:],
                      '%s%s(%s)' % (self.fields[0].name, self.suffix,
                                    prefix+'.'+self.fields[0].name+'_'))
        
    def varsDeclList(self, indent = '', start = 0):
        """declaration of self fields as args list"""
        fields = self.fields[start:]
        return reduce(lambda x, y: '%s,\n%sconst %s::value_type& %s' % (x, indent, y.type, y.name),
                      fields[1:],
                      'const %s::value_type& %s' % (fields[0].type, fields[0].name))

    def printUnusedVarsList(self, strm, defaults = False):
        """print declaration of self fields as args list"""
        strm.addLevel()
        for field in self.fields:
            print >>strm, 'Unused(%s);' % (field.name)
        if defaults:
            print >>strm, 'Unused(set_defaults);'
        strm.delLevel()
    
    def printVarsDeclList(self, strm):
        """print declaration of self fields as args list"""
        strm.addLevel()
        for field in self.fields[:len(self.fields)-1]:
            print >>strm, 'const %s::value_type& %s,' % (field.type, field.name)
        field = self.fields[len(self.fields)-1]
        print >>strm,  'const %s::value_type& %s' % (field.type, field.name)
        strm.delLevel()            
    
    def printVarsAssignList(self, strm, start = 0, prefix = ''):
        """print assigment of self protected fields from args list declared above"""
        strm.addLevel()
        for field in self.fields[start:]:
            print >>strm,  '%s%s%s = %s;' % (prefix, field.name, self.suffix, field.name)
        strm.delLevel()

    def printFreeVarsAssignList(self, strm, start = 0, prefix = ''):
        """print assigment of self protected fields from args list declared above"""
        strm.addLevel()
        for field in self.fields[start:]:
            print >>strm,  '%s%s = %s;' % (prefix, field.name, field.name)
        strm.delLevel()

    def printIndexVars (self, strm):
        """print declaration of self fields as class members"""
        for field in self.fields:
            field.printIDeclaration(strm)

    def printVars (self, strm):
        """print declaration of self fields as regular class members"""
        for field in self.fields:
            field.printDeclaration(strm)

    def printIndexAccess (self, strm):
        """print read access methods to self fields"""
        for field in self.fields:
            field.printReadAccess(strm)    

    def printKeyIndexAssignment (self, strm):
        """print in assignment operator"""
        if self.seqName:
            id = "%s%s" %  (self.fields[0].name, self.suffix)
            print >>strm, '  %s = from.%s;' % (id, id)

    def select_index(self, seqName):
        return "SELECT %s.nextval AS id FROM dual" % seqName

    def printInsertIndex (self, strm, table):
        """print use of db sequence for primary key"""
        if self.seqName:
            print >>strm, '  if (%s%s.is_null())' % (self.fields[0].name, self.suffix)
            strm.addLevel()
            print >>strm, '{'
            print >>strm, '  DB::Query  query(conn.get_query("%s"));' % self.select_index(self.seqName)
            print >>strm, '  DB::Result result(query.ask());'
            print >>strm, '  if(!result.next())'
            print >>strm, '    return false;'
            print >>strm, '  result >> %s%s;' % (self.fields[0].name, self.suffix)
            print >>strm, '}'
            strm.delLevel()
        else:
            print >>strm, '  if (%s%s.is_null())' % (self.fields[0].name, self.suffix)
            strm.addLevel()
            print >>strm, '{'
            print >>strm, '  DB::Query  query(conn.get_query("SELECT Max(%s) + 1 FROM %s"));' % \
                  (self.fields[0].dbName, table)
            print >>strm, '  DB::Result result(query.ask());'
            print >>strm, '  if(!result.next())'
            print >>strm, '    return false;'
            print >>strm, '  result >> %s%s;' % (self.fields[0].name, self.suffix)
            print >>strm, '}'
            strm.delLevel()

    def printIdSelectDecl (self, strm, name = 'select', one = False):
        """print select declaration with indexes"""
        if one:
            print >>strm, 'bool %s (%s)' % (name, self.varsDeclList(' ' * (strm.level+2)))
        else:
            print >>strm, 'bool %s (%s);' % (name, self.varsDeclList(' ' * (strm.level+2)))
    
    def printIdSelect (self, strm, name = 'select'):
        """print select with indexes"""
        strm.addLevel()
        self.printIdSelectDecl(strm, name, True)
        print >>strm, '{'
        self.printVarsAssignList(strm)
        print >>strm, '  return select();'
        print >>strm, '}\n'
        strm.delLevel()

    def printIdVarSelect (self, strm, name = 'select'):
        """print select with indexes"""
        strm.addLevel()
        self.printIdSelectDecl(strm, name, True)
        print >>strm, '{'
        self.printVarsAssignList(strm, 0, 'this->')
        print >>strm, '  bool ret = select();'
        self.printFreeVarsAssignList(strm, 0, 'this->')
        print >>strm, '  return ret;'
        print >>strm, '}\n'
        strm.delLevel()

    def printIdUpdateDecl (self, strm, name = 'update', one = False, set = True):
        """print update declaration with indexes"""
        defa = ''
        if set:
            defa = '= true'
        if one:
            print >>strm, 'bool %s (%s, bool set_defaults %s)' % (name, self.varsDeclList(' ' * (strm.level+2)), defa)
        else:
            print >>strm, 'bool %s (%s, bool set_defaults = true);' % (name, self.varsDeclList(' ' * (strm.level+2)))

    def printIdUpdate (self, strm):
        """print update with indexes"""
        strm.addLevel()
        self.printIdUpdateDecl(strm, 'update', True)
        print >>strm, '{'
        self.printVarsAssignList(strm)
        print >>strm, '  return update(set_defaults);'
        print >>strm, '}\n'
        strm.delLevel()

    def printIdVarUpdate (self, strm):
        """print update with indexes"""
        strm.addLevel()
        self.printIdUpdateDecl(strm, 'update', True)
        print >>strm, '{'
        self.printFreeVarsAssignList(strm, 0, 'this->')
        print >>strm, '  return update(set_defaults);'
        print >>strm, '}\n'
        strm.delLevel()

    def printIdInsertDecl (self, strm, name = 'insert', one = False, set = True):
        """print insert declaration with indexes"""
        defa = ''
        if set:
            defa = '= true'
        if one:
            print >>strm, 'bool %s (%s, bool set_defaults %s)' % (name, self.varsDeclList(' ' * (strm.level+2)), defa)
        else:
            print >>strm, 'bool %s (%s, bool set_defaults = true);' % (name, self.varsDeclList(' ' * (strm.level+2)))

    def printIdInsert (self, strm):
        """print insert with indexes"""
        def do (start):
            strm.addLevel()
            self.printIdInsertDecl(strm, 'insert', True)
            print >>strm, '{'
            self.printVarsAssignList(strm, start)
            print >>strm, '  return insert(set_defaults);'
            print >>strm, '}\n'
            strm.delLevel()
        if self.seqName:
            #pass first for use with sequence
            if len(self.fields) > 1:
                do(1)
        else:
            #use all indexes
            do(0)

    def printIdVarInsert (self, strm):
        """print insert with indexes"""
        def do (start):
            strm.addLevel()
            self.printIdInsertDecl(strm, 'insert', True)
            print >>strm, '{'
            self.printFreeVarsAssignList(strm, start, 'this->')
            print >>strm, '  return insert(set_defaults);'
            print >>strm, '}\n'
            strm.delLevel()
        if self.seqName:
            #pass first for use with sequence
            if len(self.fields) > 1:
                do(1)
        else:
            #use all indexes
            do(0)

    def printIdDel (self, strm):
        """print delete with indexes"""
        strm.addLevel()
        print >>strm, 'bool delet  (%s)' % self.varsDeclList(' ' * (strm.level+2))
        print >>strm, '{'
        self.printVarsAssignList(strm)
        print >>strm, '  return delet ();'
        print >>strm, '}'
        print >>strm, 'bool del  (%s)' % self.varsDeclList(' ' * (strm.level+2))
        print >>strm, '{'
        self.printVarsAssignList(strm)
        print >>strm, '  return del ();'
        print >>strm, '}'

        strm.delLevel()

    def printIdVarDel (self, strm):
        """print delete with indexes"""
        strm.addLevel()
        print >>strm, 'bool delet  (%s)' % self.varsDeclList(' ' * (strm.level+2))
        print >>strm, '{'
        self.printVarsAssignList(strm, 0, 'this->')
        self.printFreeVarsAssignList(strm, 0, 'this->')
        print >>strm, '  return delet ();'
        print >>strm, '}'
        print >>strm, 'bool del  (%s)' % self.varsDeclList(' ' * (strm.level+2))
        print >>strm, '{'
        self.printVarsAssignList(strm, 0, 'this->')        
        self.printFreeVarsAssignList(strm, 0, 'this->')
        print >>strm, '  return del ();'
        print >>strm, '}'

        strm.delLevel()

class PQIndex(Index):

  name = 'PQIndex'

  def select_index(self, seqName):
    return "SELECT  nextval('%s')" % seqName

class Field:
    def __init__ (self, dbName, name = None, link = None, default = None):
        self.index = None
        self.dbName    = dbName
        if name:
            self.name  = name
        else:
            self.name  = dbName.lower()
        self.link = link
        self.typeIndent = ' '*(typeIndent - len(self.type))
        self.default = default
    
    def __copy__ (self):
        ret = self.__class__(self.dbName, self.name, self.link)
        ret.index = self.index
        return ret
        
    def getMakeStr (self):
        link = ''
        if self.link:
            link = ", '%s'" % self.link
        default = ''
        if self.default:
            default = ", default = \"%s\"" % self.default
        return "%s%s('%s',%s'%s'%s%s)"%(self.__class__.__name__,
                                        ' '*(16-len(self.type)),
                                        self.dbName,
                                        ' '*(30-len(self.dbName)),
                                        self.name,
                                        link, default)
    
    def getRawMakeStr (self):
        """makeStr without additional formating"""
        return "%s('%s', '%s')"%(self.type,
                                 self.dbName,
                                 self.name)

    def merge (self, fromObject):
        self.name    = fromObject.name
        self.default = fromObject.default
        if not self.link:
            self.link = fromObject.link
    
    def printDeclaration (self, strm):
        """print declaration of self as class member"""
        strm.addLevel()
        print >>strm, '%s %s %s;'%(self.type, self.typeIndent, self.name)
        if self.link:
            print >>strm, '%s %s_object() const;'%(self.link, self.name)
        strm.delLevel()

    def printLink (self, strm, klass):
        """print self link method"""
        if self.link:
            print >>strm, 'inline %s %s::%s_object() const'%(self.link, klass, self.name)
            print >>strm, '{'
            print >>strm, '  return %s(conn, %s.value());' % (self.link, self.name)
            print >>strm, '}'
    
    def printImplementation (self, strm, table, id, klass):
        """implementation of self, if need"""
        pass
    
    def printIDeclaration (self, strm):
        """print declaration of self as class member, but protected member"""        
        strm.addLevel()
        print >>strm, '%s %s %s_;'%(self.type, self.typeIndent, self.name)
        strm.delLevel()
    
    def printReadAccess (self, strm):
        """print read accessor to protected self member"""
        strm.addLevel()
        print >>strm, 'const %s::value_type& %s () const { return *%s_;}' % (self.type,
                                                                             self.name,
                                                                             self.name)
        strm.delLevel()


    def printDef (self, strm, klass, suffix=''):
        """print definition of self (db name & shift) in members map of class"""
        strm.addLevel()
        default = '0'
        if self.default:
            default = '"%s"' % self.default
        print >>strm, '{ "%s", &(null<%s>()->%s%s), %s},' % (self.dbName,
                                                             klass,
                                                             self.name, suffix,
                                                             default)
        strm.delLevel()
    
    def getInit(self, conn, fromName):
        """copy from _initialization_ list"""
        return fromName
    
    def printInit (self, strm):
        """print init of self in 'simple' constructor (only connection)"""
        pass


class ORMString(Field):
    def __init__(self, dbName, name = None, link = None, default = None):
        self.type = 'ORMString'
        Field.__init__(self, dbName, name, link, default)
        
class ORMInt(Field):
    def __init__(self, dbName, name = None, link = None, default = None):
        self.type = 'ORMInt'
        Field.__init__(self, dbName, name, link, default)

class ORMBool(Field):
    def __init__(self, dbName, name = None, link = None, default = None):
        self.type = 'ORMBool'
        Field.__init__(self, dbName, name, link, default)

class ORMFloat(Field):
    def __init__(self, dbName, name = None, link = None, default = None):
        self.type = 'ORMFloat'        
        Field.__init__(self, dbName, name, link, default)

class FieldWithInit(Field):
    def __init__(self, dbName, name = None, link = None, default = None):
        Field.__init__(self, dbName, name, link, default)
    
    def printInit (self, strm):
        strm.rawwrite(',\n')
        strm.addLevel()
        strm.write(' %s(connection)' % self.name)
        strm.delLevel()

class ORMDate(Field):
    def __init__(self, dbName, name = None, link = None, default = None):
        self.type = 'ORMDate'
        Field.__init__(self, dbName, name, link, default)
    
class ORMTimestamp(Field):
    def __init__(self, dbName, name = None, link = None, default = None):
        self.type = 'ORMTimestamp'
        Field.__init__(self, dbName, name, link, default)
    

class ORMBlob(Field):
    def __init__(self, dbName, name = None):
        self.type = 'ORMText'
        Field.__init__(self, dbName, name)


class ORMClob(Field):
    def __init__(self, dbName, name = None):
        self.type = 'ORMText'
        Field.__init__(self, dbName, name)
    
class AskSingle:
    def __init__(self, name, query, type):
        self.name      = name
        self.query     = query
        self.type      = type

    def __copy__(self):
        return AskSingle(self.name, self.query, self.type)

    def getMakeStr (self):
        return "AskSingle('%s', '%s', '%s')"%(self.name, self.query, self.type)

    def printDeclaration (self, strm, klass):
        strm.addLevel()
        print >>strm, 'bool %s (%s arg);'%(self.name, self.type)
        strm.delLevel()

    def printImplementation(self, strm, klass):
        print >>strm, 'bool %s::%s (%s arg)'%(klass.name, self.name, self.type)
        print >>strm, '{'
        print >>strm, '  DB::Query  query(conn.get_query(\"%s\"));' % self.query
        print >>strm, '  query.set(arg);'
        print >>strm, '  bool ret = query.update() > 0;'
        print >>strm, '  conn.commit();'
        print >>strm, '  return ret;'
        print >>strm, '}\n'

class Select:
    def __init__(self, name, names):
        self.name  = name
        self.names = names
        self.fields = []
        self.id = None
    
    def __copy__(self):
        return Select(self.name, self.names)
    
    def getMakeStr (self):
        return "Select('%s', [%s])" % (self.name,
                                       reduce(lambda x, y: "%s, '%s' " % (x, y), self.names[1:],
                                              "'%s'"%self.names[0]))
    
    def __makeSelf (self, klass):
        index  = []
        fields_from = {}
        for field in klass.fields:
            fields_from[field.dbName] = field
        for field in klass.id.fields:
            f = copy.copy(field)
            f.name = f.name + indexSuffix
            fields_from[field.dbName] = f
        for name in self.names:
            if fields_from.has_key(name):
                index.append(fields_from[name])
                del fields_from[name]
            else:
                print 'ERROR!', 'BAD Select ' + self.name + ' in ' + klass.name
                sys.exit(0)
        self.id = Index(index)
        self.id.suffix = ''
        self.fields = fields_from.values()
        self.fields.sort(lambda x, y: cmp(x.index, y.index))
    
    def printDeclaration (self, strm, klass):
        self.__makeSelf(klass)
        strm.addLevel()
        self.id.printIdSelectDecl(strm, 'has_'+self.name)
        self.id.printIdSelectDecl(strm, 'select_'+self.name)
        self.id.printIdUpdateDecl(strm, 'update_'+self.name)
        self.id.printIdSelectDecl(strm, 'delet_'+self.name)
        self.id.printIdSelectDecl(strm, 'del_'+self.name)
        self.id.printIdInsertDecl(strm, 'insert_'+self.name, True)
        print >>strm, '{'
        self.id.printVarsAssignList(strm, 0, 'this->')
        print >>strm, '  return insert(set_defaults);'
        print >>strm, '}'
        strm.delLevel()

    def __printHasImplementation(self, strm, klass):
        self.id.printIdSelectDecl(strm, "%s::has_%s" % (klass.name, self.name), True)
        print >>strm, '{'
        self.id.printUnusedVarsList(strm)
        self.id.printVarsAssignList(strm, 0, 'this->')
        print >>strm, '  DB::Query query(conn.get_query("SELECT "'
        for i in xrange(0, len(klass.id.fields)-1):
            print >>strm, '      "%s,"' % klass.id.fields[i].dbName
        print >>strm, '      "%s "' % klass.id.fields[len(klass.id.fields)-1].dbName
        print >>strm, '    "FROM %s "' % klass.dbName
        print >>strm, '    "WHERE %s"));' % self.id.dbVarsAssignDeclList()
        for field in self.id.fields:
            print >>strm, '  query.set(%s);' % field.name
        print >>strm, '  DB::Result result(query.ask());'
        print >>strm, '  if(result.next())'
        print >>strm, '  {'
        if len(klass.id.fields) > 1:
            print >>strm, '    result >> this->%s' %  (klass.id.fields[0].name + indexSuffix)
            for i in xrange(1, len(klass.id.fields)-1):
                print >>strm, '           >> this->%s' % (klass.id.fields[i].name + indexSuffix)
            print >>strm, '           >> this->%s;' % (klass.id.fields[len(klass.id.fields)-1].name + indexSuffix)
        else:
            print >>strm, '    result >> this->%s;' %  (klass.id.fields[0].name + indexSuffix)
        print >>strm, '    return true;'
        print >>strm, '  }'
        print >>strm, '  return false;'
        print >>strm, '}\n'

    def __printSelectImplementation(self, strm, klass):
        self.id.printIdSelectDecl(strm, "%s::select_%s" % (klass.name, self.name), True)
        print >>strm, '{'
        self.id.printUnusedVarsList(strm)
        self.id.printVarsAssignList(strm, 0, 'this->')
        print >>strm, '  DB::Query query(conn.get_query("SELECT "'
        for i in xrange(0, len(self.fields)-1):
            print >>strm, '      "%s,"' % self.fields[i].dbName
        print >>strm, '      "%s "' % self.fields[len(self.fields)-1].dbName
        print >>strm, '    "FROM %s "' % klass.dbName
        print >>strm, '    "WHERE %s"));' % self.id.dbVarsAssignDeclList()
        for field in self.id.fields:
            print >>strm, '  query.set(%s);' % field.name
        print >>strm, '  DB::Result result(query.ask());'
        print >>strm, '  if(result.next())'
        print >>strm, '  {'
        if len(self.fields) > 1:
            print >>strm, '    result >> %s' %  self.fields[0].name
            for i in xrange(1, len(self.fields)-1):
                print >>strm, '           >> %s' % self.fields[i].name
            print >>strm, '           >> %s;' % self.fields[len(self.fields)-1].name
        else:
            print >>strm, '    result >> %s;' %  self.fields[0].name
        print >>strm, '    return true;'
        print >>strm, '  }'
        print >>strm, '  return false;'
        print >>strm, '}\n'

    def __printUpdateImplementation(self, strm, klass):
        self.id.printIdUpdateDecl(strm, "%s::update_%s" % (klass.name, self.name), True, False)
        print >>strm, '{'
        self.id.printUnusedVarsList(strm, True)
        self.id.printVarsAssignList(strm, 0, 'this->')
        print >>strm, '  std::ostringstream strm;'
        print >>strm, '  strm << "UPDATE %s SET ";' % klass.dbName
        print >>strm, '  int counter = 1;'
        for field in self.fields:
            if field.index < klass.fieldsCount():
                print >>strm, '    counter = update_(strm, counter, this, members_[%i], set_defaults);'%field.index
        print >>strm, '  if(counter > 1)'
        print >>strm, '  {'
        print >>strm, '    strm << " WHERE %s";' % self.id.dbVarsAssignDeclList()
        print >>strm, '    DB::Query  query(conn.get_query(strm.str()));'
        for field in self.fields:
            if field.index < klass.fieldsCount():
                print >>strm, '    setin_(query, %s);' %  field.name
        for field in self.id.fields:
            print >>strm, '    query.set(%s);' % field.name
        if  klass.haveBlob():
            print >>strm, '    query.update();'
            print >>strm, '    query.flush();'
            print >>strm, '    conn.commit();'
            print >>strm, '    return true;'
        else:
            print >>strm, '    bool ret = query.update() > 0;'
            print >>strm, '    conn.commit();'
            if klass.pgsync:
                print >>strm, '    pgsync_();'
            print >>strm, '    return ret;'
        print >>strm, '  }'
        print >>strm, '  return false;'
        print >>strm, '}\n'
    
    def __printDelImplementation(self, strm, klass):
        self.id.printIdSelectDecl(strm, "%s::del_%s" % (klass.name, self.name), True)
        print >>strm, '{'
        self.id.printUnusedVarsList(strm)
        if klass.hasField('NAME') and klass.hasField('STATUS'):
            self.id.printVarsAssignList(strm, 0, 'this->')
            print >>strm, '  DB::Query  query(conn.get_query("UPDATE %s SET" ' % klass.dbName
            comm = ' '
            if klass.uniqueName().upper() != 'CHANNEL':
                print >>strm, '    "    STATUS = \'D\' "'
                comm = ','
            print >>strm, '    "  %s NAME = concat(NAME, CONCAT(\'-D-\', TO_CHAR(now(), \'YYMMDDhhmmss\'))) "' % comm
            print >>strm, '    "WHERE %s"));'%self.id.dbVarsAssignDeclList()
            for field in self.id.fields:
                print >>strm, '  query.set(%s);' % field.name
            print >>strm, '  bool ret = query.update() > 0;'
            print >>strm, '  conn.commit();'
            if klass.pgsync:
                print >>strm, '  pgsync_();'
            print >>strm, '  return ret;'
        else:
            print >>strm, '  return delet_%s(%s);' % (self.name, self.id.varsList())
        print >>strm, '}\n'

    def __printDeleteImplementation(self, strm, klass):
        self.id.printIdSelectDecl(strm, "%s::delet_%s" % (klass.name, self.name), True)
        print >>strm, '{'
        self.id.printUnusedVarsList(strm)
        self.id.printVarsAssignList(strm, 0, 'this->')
        print >>strm, '  DB::Query  query(conn.get_query("DELETE FROM %s WHERE %s"));'%(klass.dbName, self.id.dbVarsAssignDeclList())
        for field in self.id.fields:
            print >>strm, '  query.set(%s);' % field.name
        print >>strm, '  bool ret = query.update() > 0;'
        print >>strm, '  conn.commit();'
        if klass.pgsync:
            print >>strm, '  pgsync_();'
        print >>strm, '  return ret;'
        print >>strm, '}\n'
    
    def printImplementation(self, strm, klass):
        self.__printHasImplementation(strm, klass)
        self.__printSelectImplementation(strm, klass)
        self.__printUpdateImplementation(strm, klass)
        self.__printDeleteImplementation(strm, klass)
        self.__printDelImplementation(strm, klass)
        

class Object:
    def __init__(self, dbName, name = None, used = False):
        self.dbName    = dbName
        if name:
            self.name  = name
        else:
            name = dbName.lower()
            self.name = name[0].upper() + name[1:]
        self.id        = None
        self.fields    = []
        self.asks      = []
        self.fmap      = {}
        self.used      = used
        self.pgsync    = False
    
    def __copy__(self):
        ret = Object(self.dbName, self.name)
        ret.id     = copy.copy(self.id)
        ret.asks   = copy.copy(self.asks)
        ret.fields = copy.copy(self.fields)
        ret.pgsync = copy.copy(self.pgsync)
        return ret

    def hasField(self, dbName):
        if len(self.fmap) == 0:
            for field in self.fields:
                self.fmap[field.dbName.upper()] = field
        return self.fmap.has_key(dbName)
    
    def uniqueName (self):
        return self.dbName.lower()

    def fieldsCount (self):
        return len(self.fields)

    def haveBlob (self):
        return len(filter(
            lambda x: isinstance(x, ORMBlob),
            self.fields)) != 0
    
    
    def getMakeStr (self):
        out = StringIO.StringIO()
        varname = self.uniqueName()
        print >>out, "%s = Object('%s', '%s', %s)" % (varname, self.dbName, self.name, self.used)
        if self.id != None:
            print >>out, "%s.id = %s" % (varname, self.id.getMakeStr())
        if self.fieldsCount() > 0:
            print >>out, "%s.fields = [" % varname
            for field in self.fields:
                print >>out, "    %s," % field.getMakeStr()
            print >>out, "  ]"
        if len(self.asks) > 0:
            print >>out, "%s.asks = [" % varname
            for ask in self.asks:
                print >>out, "    %s," % ask.getMakeStr()
            print >>out, "  ]"
        if self.pgsync:
            print >>out, "%s.pgsync = %s" % (varname, self.pgsync)
        ret = out.getvalue()
        out.close()
        return ret
    
    def merge (self, fromObject):
        self.name = fromObject.name
        self.id.merge(fromObject.id)
        my_fields = {}
        for field in self.fields:
            my_fields[field.dbName] = field
        for from_field in fromObject.fields:
            if my_fields.has_key(from_field.dbName):
                my_fields[from_field.dbName].merge(from_field)
        self.asks = copy.copy(fromObject.asks)
        self.used = fromObject.used
        self.pgsync = fromObject.pgsync
    
    def constructorName (self):
        ret = "%s::%s"%(self.name, self.name)
        return ret

    def printPGSyncImplementation(self, strm):
        print >>strm, 'void %s::pgsync_ ()' % self.name
        print >>strm, '{'
        print >>strm, '  DB::Conn pg_conn('
        print >>strm, '    DB::Conn::open_pq('
        print >>strm, '      AutoTest::GlobalSettings::instance().config().PGDBConnection()[0].user(),'
        print >>strm, '      AutoTest::GlobalSettings::instance().config().PGDBConnection()[0].password(),'
        print >>strm, '      AutoTest::GlobalSettings::instance().config().PGDBConnection()[0].host(),'
        print >>strm, '      AutoTest::GlobalSettings::instance().config().PGDBConnection()[0].db()));'
        print >>strm, '  DB::Query query(pg_conn.query('
        print >>strm, '    "SELECT replication.refill_one_table(\'%s\', \'%s\');"));'%(self.dbName, self.id.dbVarsAssignValList())
        print >>strm, '  ORM::SerializeQueryManager::instance().execute(pg_conn, query);'
        print >>strm, '}\n'

    def printSelectImplementation(self, strm):
        print >>strm, 'bool %s::select ()' % self.name
        print >>strm, '{'
        print >>strm, '  DB::Query query(conn.get_query("SELECT "'
        for i in xrange(0, self.fieldsCount()-1):
            print >>strm, '      "%s,"' % self.fields[i].dbName
        print >>strm, '      "%s "' % self.fields[self.fieldsCount()-1].dbName
        print >>strm, '    "FROM %s "' % self.dbName
        print >>strm, '    "WHERE %s"));' % self.id.dbVarsAssignDeclList()
        print >>strm, '  query %s;' % self.id.varsLShiftList()
        print >>strm, '  DB::Result result(query.ask());'
        print >>strm, '  if(result.next())'
        print >>strm, '  {'
        print >>strm, '    for(unsigned int i = 0; i < %i; ++i)' % self.fieldsCount()
        print >>strm, '    {'
        print >>strm, '      result >> members_[i].value(this);'
        print >>strm, '    }'
        print >>strm, '    return true;'
        print >>strm, '  }'
        print >>strm, '  return false;'
        print >>strm, '}\n'

    def printSimpleSelectImplementation(self, strm):
        print >>strm, 'bool %s::select ()' % self.name
        print >>strm, '{'
        print >>strm, '  DB::Query query(conn.get_query("SELECT "'
        for i in xrange(0, len(self.id.fields) - 1):
            print >>strm, '      "%s,"' % self.id.fields[i].dbName
        print >>strm, '      "%s "' % self.id.fields[-1].dbName
        print >>strm, '    "FROM %s "' % self.dbName
        print >>strm, '    "WHERE %s"));' % self.id.dbVarsAssignDeclList()
        print >>strm, '  query %s;' % self.id.varsLShiftList()
        print >>strm, '  DB::Result result(query.ask());'
        print >>strm, '  if(result.next())'
        print >>strm, '  {'
        print >>strm, '    return true;'
        print >>strm, '  }'
        print >>strm, '  return false;'
        print >>strm, '}\n'

    def printLogImplementation(self, strm):
        print >>strm, 'void %s::log_in (Logger& log, unsigned long severity)' % self.name
        print >>strm, '{'
        print >>strm, '  if(select())'
        print >>strm, '  {'
        print >>strm, '    std::ostringstream out;'
        print >>strm, '    out << "Data record: %s" << std::endl;' % self.name
        print >>strm, '    out << "{" << std::endl;'
        for i in xrange(0, self.id.indexesCount()):
            print >>strm, '    out << "* %s = " << strof(%s%s) << ";" << std::endl;'%(self.id.fields[i].name,
                                                                                      self.id.fields[i].name, self.id.suffix)        
        for i in xrange(0, self.fieldsCount()-1):
            print >>strm, '    out << "  %s = " << strof(%s) << ";" << std::endl;'%(self.fields[i].name, self.fields[i].name)
        print >>strm, '    out << "}" << std::endl;'
        print >>strm, '    log.log(out.str(), severity);'
        print >>strm, '  }'
        print >>strm, '  else'
        print >>strm, '  {'
        print >>strm, '    throw Exception("error to log %s because not select");' % self.name
        print >>strm, '  }'
        print >>strm, '}\n'
    
    def printUpdateImplementation(self, strm):
        print >>strm, 'bool %s::update (bool set_defaults)' % self.name
        print >>strm, '{'
        print >>strm, '  Unused(set_defaults);'
        print >>strm, '  std::ostringstream strm;'
        print >>strm, '  strm << "UPDATE %s SET ";' % self.dbName
        print >>strm, '  int counter = 1;'
        print >>strm, '  for(unsigned int i = 0; i < %i; ++i)' % self.fieldsCount()
        print >>strm, '  {'
        print >>strm, '    counter = update_(strm, counter, this, members_[i], set_defaults);'
        print >>strm, '  }'
        print >>strm, '  if(counter > 1)'
        print >>strm, '  {'
        print >>strm, '    strm << " WHERE %s";' % self.id.dbVarsAssignDeclList()
        print >>strm, '    DB::Query  query(conn.get_query(strm.str()));'
        print >>strm, '    for(unsigned int i = 0; i < %i; ++i)' % self.fieldsCount()
        print >>strm, '    {'
        print >>strm, '      setin_(query, members_[i].value(this));'
        print >>strm, '    }'
        print >>strm, '    query %s;' % self.id.varsLShiftList()
        if  self.haveBlob():
            print >>strm, '    query.update();'
            print >>strm, '    query.flush();'
            print >>strm, '    conn.commit();'
            if self.pgsync:
                print >>strm, '    pgsync_();'
            print >>strm, '    return true;'
        else:
            print >>strm, '    bool ret = query.update() > 0;'
            print >>strm, '    conn.commit();'
            if self.pgsync:
                print >>strm, '    pgsync_();'
            print >>strm, '    return ret;'
        print >>strm, '  }'
        print >>strm, '  return false;'
        print >>strm, '}\n'
    
    def printInsertImplementation(self, strm):
        print >>strm, 'bool %s::insert (bool set_defaults)' % self.name
        print >>strm, '{'
        print >>strm, '  Unused(set_defaults);'
        self.id.printInsertIndex (strm, self.dbName)
        if self.fieldsCount() > 0:
            print >>strm, '  std::ostringstream strm;'
            print >>strm, '  strm << "INSERT INTO %s (";' % self.dbName
            print >>strm, '  int counter = 1;'
            print >>strm, '  for(unsigned int i = 0; i < %i; ++i)' % self.fieldsCount()
            print >>strm, '  {'
            print >>strm, '    counter = insert_(strm, counter, this, members_[i], set_defaults);'
            print >>strm, '  }'
            print >>strm, '  if(counter >= 1)'
            print >>strm, '  {'
            print >>strm, '    strm << ((counter > 1)? ",": " ") << "%s)";' % self.id.dbVarsList()
            print >>strm, '    strm <<" VALUES (";'
            print >>strm, '    for(unsigned int i = 0; i < %i; ++i)' % self.fieldsCount()
            print >>strm, '    {'
            print >>strm, '      put_var_ (strm, i,  this, members_[i], set_defaults);'
            print >>strm, '    }'
            print >>strm, '    strm << "%s)";' % self.id.dbVarsIndexList()
            print >>strm, '    DB::Query  query(conn.get_query(strm.str()));'
            print >>strm, '    for(unsigned int i = 0; i < %i; ++i)' % self.fieldsCount()
            print >>strm, '    {'
            print >>strm, '      insertin_(query, members_[i].value(this));'
            print >>strm, '    }'
        else:
            print >>strm, '  {'
            print >>strm, '    DB::Query  query(conn.get_query(" INSERT INTO %s (%s)"' % (self.dbName, self.id.dbVarsList())
            print >>strm, '                       "VALUES (%s) "));' % self.id.dbVarsIndexList()
        print >>strm, '    query %s;' % self.id.varsLShiftList()
        if  self.haveBlob():
            print >>strm, '    query.update();'
            print >>strm, '    query.flush();'
            print >>strm, '    conn.commit();'
            if self.pgsync:
                print >>strm, '    pgsync_();'
            print >>strm, '    return true;'
        else:
            print >>strm, '    bool ret = query.update() > 0;'
            print >>strm, '    conn.commit();'
            if self.pgsync:
                print >>strm, '    pgsync_();'
            print >>strm, '    return ret;'
        print >>strm, '  }'
        print >>strm, '  return false;'
        print >>strm, '}\n'
    
    def printDelImplementation(self, strm):
        print >>strm, 'bool %s::del ()' % self.name
        print >>strm, '{'
        if self.hasField('NAME') and self.hasField('STATUS'):
            if self.hasField('DISPLAY_STATUS_ID'):
              print >>strm, '  int status_id = get_display_status_id(conn, "%s", DS_DELETED);' % self.dbName
            print >>strm, '  DB::Query  query(conn.get_query("UPDATE %s SET"' % self.dbName
            comm = ' '
            if self.uniqueName().upper() != 'CHANNEL':
                print >>strm, '    "    STATUS = \'D\' "'
                comm = ','
            if self.hasField('DISPLAY_STATUS_ID'):
                print >>strm, '    "  %s DISPLAY_STATUS_ID = :var1 "'%comm
                comm = ','
            print >>strm, '    "  %s NAME = concat(NAME, CONCAT(\'-D-\', TO_CHAR(now(), \'YYMMDDhhmmss\'))) "'%comm
            print >>strm, '    "WHERE %s"));'%self.id.dbVarsAssignDeclList()
            if self.hasField('DISPLAY_STATUS_ID'):
                print >>strm, '  query.set(status_id);'
            print >>strm, '  query %s;' % self.id.varsLShiftList()
            print >>strm, '  bool ret = query.update() > 0;'
            print >>strm, '  conn.commit();'
            print >>strm, '  this->select();'
            if self.pgsync:
                print >>strm, '  pgsync_();'
            print >>strm, '  return ret;'
        else:
            print >>strm, '  return delet();'
        print >>strm, '}\n'
    
    def printDeleteImplementation(self, strm):
        print >>strm, 'bool %s::delet ()' % self.name
        print >>strm, '{'
        print >>strm, '  DB::Query  query(conn.get_query("DELETE FROM %s WHERE %s"));'%(self.dbName, self.id.dbVarsAssignDeclList())
        print >>strm, '  query %s;' % self.id.varsLShiftList()
        print >>strm, '  bool ret = query.update() > 0;'
        print >>strm, '  conn.commit();'
        if self.pgsync:
            print >>strm, '  pgsync_();'
        print >>strm, '  return ret;'
        print >>strm, '}\n'

    def printStatusImplementation(self, strm):
        print >>strm, 'bool %s::set_display_status (DisplayStatus status)' % self.name
        print >>strm, '{'
        print >>strm, '  int status_id = get_display_status_id(conn, "%s", status);' % self.dbName
        print >>strm, '  DB::Query  query(conn.get_query("UPDATE %s SET"' % self.dbName
        print >>strm, '    "  DISPLAY_STATUS_ID = :var1 "'
        print >>strm, '    "WHERE %s"));'% self.id.dbVarsAssignDeclList()
        print >>strm, '  query.set(status_id);'
        print >>strm, '  query %s;' % self.id.varsLShiftList()
        print >>strm, '  bool ret = query.update() > 0;'
        print >>strm, '  conn.commit();'
        print >>strm, '  display_status_id = status_id;'
        if self.pgsync:
            print >>strm, '  pgsync_();'
        print >>strm, '  return ret;'
        print >>strm, '}\n'

    def printTouchImplementation(self, strm):
        print >>strm, 'bool %s::touch ()' % self.name
        print >>strm, '{'
        print >>strm, '  std::ostringstream strm;'
        print >>strm, '  strm << "UPDATE %s SET VERSION = now()";' % self.dbName
        print >>strm, '  {'
        print >>strm, '    strm << " WHERE %s";' % self.id.dbVarsAssignDeclList()
        print >>strm, '    DB::Query  query(conn.get_query(strm.str()));'
        print >>strm, '    query %s;' % self.id.varsLShiftList()
        print >>strm, '    bool ret = query.update() > 0;'
        print >>strm, '    conn.commit();'
        if self.pgsync:
            print >>strm, '    pgsync_();'
        print >>strm, '    return ret;'
        print >>strm, '  }'
        print >>strm, '  return false;'
        print >>strm, '}\n'
    
    def printConstructDeclaration (self, strm, name, end = True):
        def printme (sep):
            print >>strm, '%s (DB::IConn& connection,' % name
            self.id.printVarsDeclList(strm)
            print >>strm, ')%s' % sep
        if end:
            strm.addLevel()
            printme(';')
            strm.delLevel()
        else:
            printme('')
    
    def printConstructImplementation (self, strm, connection_type):
        #constructor
        self.printConstructDeclaration(strm, self.constructorName(), False)
        #print members initialisation
        strm.write('  :ORMObject<%s>(connection), %s' % \
                   (connection_type,  self.id.varsInitList()))
        for field in self.fields:
            field.printInit(strm)
        strm.newline()
        print >>strm, '{}\n'
        #copy constructor
        print >>strm, '%s::%s(const %s& from)' % (self.name, self.name, self.name)
        #print members initialisation
        strm.write('  :ORMObject<%s>(from), %s' % \
                   (connection_type, self.id.varsCopyList('from')))
        if self.fieldsCount() > 0:
            strm.rawwrite(',\n')
            for field in self.fields[:self.fieldsCount()-1]:
                print >>strm, '  %s(from.%s),' % (field.name, field.name)
            print >>strm, '  %s(from.%s)' % (self.fields[self.fieldsCount()-1].name, self.fields[self.fieldsCount()-1].name)
        else:
            strm.newline()
        print >>strm, '{'
        print >>strm, '}\n'
        #assigment
        print >>strm, '%s& %s::operator=(const %s& from)' % (self.name, self.name, self.name)
        print >>strm, '{'
        print >>strm, '  Unused(from);'
        self.id.printKeyIndexAssignment(strm)
        for field in self.fields:
             print >>strm, '  %s = from.%s;' % (field.name, field.name)
        print >>strm, '  return *this;'
        print >>strm, '}\n'
    
    def printImplementation(self, strm, connection_type):
        if not self.used:
            return
        # print fields map
        print >>strm, 'ORMObjectMember %s::members_[%i] = {' % (self.name, self.fieldsCount() + self.id.indexesCount())
        # print fields definitions for map and mumerate them
        for field in self.fields:
            field.printDef(strm, self.name)
        for field in self.id.fields:
            field.printDef(strm, self.name, indexSuffix)
        print >>strm, '};\n'
        # print constructors
        print >>strm, '%s (DB::IConn& connection)' % self.constructorName()
        strm.write('  :ORMObject<%s>(connection)' % connection_type)
        for field in self.fields:
            field.printInit(strm)
        strm.newline()
        print >>strm, '{}\n'
        self.printConstructImplementation (strm, connection_type)
        # print fields methods if need
        for field in self.fields:
            field.printImplementation(strm, self.dbName, self.id, self.name)
        #print methods
        if self.hasField('VERSION'):
            self.printTouchImplementation(strm)
        if self.fieldsCount() > 0:
            self.printSelectImplementation(strm)
            self.printUpdateImplementation(strm)
        elif len(self.id.fields):
            self.printSimpleSelectImplementation(strm)
        self.printInsertImplementation(strm)
        self.printDeleteImplementation(strm)
        self.printDelImplementation(strm)
        if self.hasField('DISPLAY_STATUS_ID'):
            self.printStatusImplementation(strm)
        self.printLogImplementation(strm)
        #print asks
        for ask in self.asks:
            ask.printImplementation(strm, self)
        #print pgsync_
        if self.pgsync:
           self.printPGSyncImplementation(strm)
        print >>strm, 'std::ostream& operator<< (std::ostream& out, const %s& val)' % self.name
        print >>strm, '{'
        print >>strm, '  out << "%s:" << std::endl;' % self.name
        for field in self.id.fields:
            print >>strm, '  out << " *%s: " << val.%s_ << std::endl;' % (field.name, field.name)
        for field in self.fields:
            print >>strm, '  out << "  %s: " << val.%s << std::endl;' % (field.name, field.name)
        print >>strm, '  return out;'
        print >>strm, '}'

    def __makeSelf (self):
        index = 0
        for field in self.fields:
            field.index = index
            index = index +1
        for field in self.id.fields:
            field.index = index
            index = index +1

    def printDeclaration (self, strm, connection_type):
        self.__makeSelf()
        if not self.used:
            return
        print >>strm, 'class %s:' % self.name
        print >>strm, '  public ORMObject<%s>' % connection_type
        print >>strm, '{'
        print >>strm, 'protected:'
        print >>strm, '  static ORMObjectMember members_[%i];' % (self.fieldsCount() + self.id.indexesCount())
        strm.newline()
        self.id.printIndexVars(strm)
        strm.newline()
        print >>strm, 'public:'
        strm.newline()
        #print fields
        for field in self.fields:
            field.printDeclaration(strm)
        strm.newline()
        #print readonly access to indexes
        self.id.printIndexAccess(strm)
        strm.newline()
        #basic constructors
        print >>strm, '  %s (DB::IConn& connection);' % self.name
        print >>strm, '  %s (const %s& from);' % (self.name, self.name)
        print >>strm, '  %s& operator=(const %s& from);' % (self.name, self.name)
        strm.newline()
        #indexed constructors
        self.printConstructDeclaration (strm, self.name)
        if self.hasField('VERSION'):
            strm.newline()
            print >>strm, '  virtual bool touch (); //!< touch version'
        strm.newline()
        #methods
        if len(self.id.fields):
            print >>strm, '  virtual bool select (); //!< get exists'
            self.id.printIdSelect (strm)
        if self.fieldsCount() > 0:
            print >>strm, '  virtual bool update (bool set_defaults = true); //!< update exists'
            self.id.printIdUpdate (strm)
        print >>strm, '  virtual bool insert (bool set_defaults = true); //!< create new'
        self.id.printIdInsert (strm)
        print >>strm, '  virtual bool delet  (); //!< delete exists'
        print >>strm, '  virtual bool del    (); //!< set status D and name->name+timestamp'
        print >>strm, '  virtual void log_in  (Logger&, unsigned long severity = Logging::Logger::INFO); //!< log record'
        if self.hasField('DISPLAY_STATUS_ID'):
            print >>strm, '  virtual bool set_display_status (DisplayStatus); //!< set display status'
        self.id.printIdDel (strm)
        #asks
        if len(self.asks) > 0:
            strm.newline()
            for ask in self.asks:
                ask.printDeclaration(strm, self)
        print >>strm, 'friend std::ostream& operator<< (std::ostream& out, const %s& val);' % self.name
        if self.pgsync:
            print >>strm, 'void pgsync_();'
        print >>strm, '};'
        print >>strm, 'std::ostream& operator<< (std::ostream& out, const %s& val);\n' % self.name        
    
    def printSelf (self, strm):
        if not self.used:
            return        
        print >>strm, 'class %s;' % self.name

    def printLinks (self, strm):
        if not self.used:
            return
        for field in self.fields:
            field.printLink(strm, self.name)
                        

class DualObject(Object):
    def __init__(self, dbName, name = None, used = False):
        Object.__init__(self, dbName, name, used)
    
    def getMakeStr (self):
        out = StringIO.StringIO()
        varname = self.uniqueName()
        print >>out, "%s = DualObject('%s', '%s', %s)" % (varname, self.dbName, self.name, self.used)
        if self.id != None:
            print >>out, "%s.id = %s" % (varname, self.id.getMakeStr())
        if self.fieldsCount() > 0:
            print >>out, "%s.fields = [" % varname
            for field in self.fields:
                print >>out, "    %s," % field.getMakeStr()
            print >>out, "  ]"
        if len(self.asks) > 0:
            print >>out, "%s.asks = [" % varname
            for ask in self.asks:
                print >>out, "    %s," % ask.getMakeStr()
            print >>out, "  ]"
        ret = out.getvalue()
        out.close()
        return ret
    
    def printSelectImplementation(self, strm):
        print >>strm, 'bool %s::select ()' % self.name
        print >>strm, '{'
        print >>strm, '  DB::Query query(conn.get_query("SELECT "'
        for i in xrange(0, self.fieldsCount()-1):
            print >>strm, '      "%s,"' % self.fields[i].dbName
        print >>strm, '      "%s "' % self.fields[self.fieldsCount()-1].dbName
        print >>strm, '    "FROM %s "' % self.dbName
        print >>strm, '    "WHERE %s"));' % self.id.dbVarsAssignDeclList()
        print >>strm, '  query %s;' % self.id.varsLShiftList()
        print >>strm, '  DB::Result result(query.ask());'
        print >>strm, '  if(result.next())'
        print >>strm, '  {'
        print >>strm, '    for(unsigned int i = 0; i < %i; ++i)' % self.fieldsCount()
        print >>strm, '    {'
        print >>strm, '      result >> members_[i].value(this);'
        print >>strm, '    }'
        print >>strm, '    return true;'
        print >>strm, '  }'
        print >>strm, '  return false;'
        print >>strm, '}\n'
    
    def printUpdateImplementation(self, strm):
        print >>strm, 'bool %s::update (bool set_defaults)' % self.name
        print >>strm, '{'
        print >>strm, '  Unused(set_defaults);'
        print >>strm, '  std::ostringstream strm;'
        print >>strm, '  strm << "UPDATE %s SET ";' % self.dbName
        print >>strm, '  int counter = 1;'
        print >>strm, '  for(unsigned int i = 0; i < %i; ++i)' % (self.fieldsCount() + self.id.indexesCount())
        print >>strm, '  {'
        print >>strm, '    counter = update_(strm, counter, this, members_[i], set_defaults);'
        print >>strm, '  }'
        print >>strm, '  if(counter > 1)'
        print >>strm, '  {'
        print >>strm, '    strm << " WHERE %s";' % self.id.dbVarsAssignDeclList()
        print >>strm, '    DB::Query  query(conn.get_query(strm.str()));'
        print >>strm, '    for(unsigned int i = 0; i < %i; ++i)' % (self.fieldsCount() + self.id.indexesCount())
        print >>strm, '    {'
        print >>strm, '      setin_(query, members_[i].value(this));'
        print >>strm, '    }'
        print >>strm, '    query %s;' % self.id.varsLShiftList()
        print >>strm, '    bool ret = query.update() > 0;'
        print >>strm, '    conn.commit();'
        print >>strm, '    if(ret)'
        print >>strm, '    {'
        for field in self.id.fields:
            print >>strm, '      %s%s = %s;' % (field.name, indexSuffix, field.name)
        print >>strm, '    }'
        print >>strm, '    return ret;'
        print >>strm, '  }'
        print >>strm, '  return false;'
        print >>strm, '}\n'

    def printInsertImplementation(self, strm):
        print >>strm, 'bool %s::insert (bool set_defaults)' % self.name
        print >>strm, '{'
        print >>strm, '  Unused(set_defaults);'
        self.id.printInsertIndex (strm, self.dbName)
        for field in self.id.fields:
            print >>strm, '  %s%s = %s;' % (field.name, indexSuffix, field.name)
        print >>strm, '  int counter = 1;'
        print >>strm, '  for(unsigned int i = 0; i < %i; ++i)' % (self.fieldsCount() + self.id.indexesCount())
        print >>strm, '  {'
        print >>strm, '    counter = count_(counter, this, members_[i], set_defaults);'
        print >>strm, '  }'            
        print >>strm, '  if(counter >= 1)'
        print >>strm, '  {'
        if self.fieldsCount() > 0:
            print >>strm, '    counter = 1;'            
            print >>strm, '    std::ostringstream strm;'
            print >>strm, '    strm << "INSERT INTO %s (";' % self.dbName
            print >>strm, '    for(unsigned int i = 0; i < %i; ++i)' % self.fieldsCount()
            print >>strm, '    {'
            print >>strm, '      counter = insert_(strm, counter, this, members_[i], set_defaults);'
            print >>strm, '    }'            
            print >>strm, '    strm << ((counter > 1)? ",": " ") << "%s)";' % self.id.dbVarsList()
            print >>strm, '    strm <<" VALUES ( ";'
            print >>strm, '    for(unsigned int i = 0; i < %i; ++i)' % self.fieldsCount()
            print >>strm, '    {'
            print >>strm, '      put_var_ (strm, i,  this, members_[i], set_defaults);'
            print >>strm, '    }'
            print >>strm, '    strm << "%s )";' % self.id.dbVarsIndexList()
            print >>strm, '    DB::Query  query(conn.get_query(strm.str()));'
            print >>strm, '    for(unsigned int i = 0; i < %i; ++i)' % self.fieldsCount()
            print >>strm, '    {'
            print >>strm, '      insertin_(query, members_[i].value(this));'
            print >>strm, '    }'
        else:
            print >>strm, '    DB::Query  query(conn.get_query(" INSERT INTO %s (%s)"' % (self.dbName, self.id.dbVarsList())
            print >>strm, '                       "VALUES (%s) "));' % self.id.dbVarsIndexList()
        print >>strm, '    query %s;' % self.id.varsLShiftList()
        if  self.haveBlob():
            print >>strm, '    query.update();'
            print >>strm, '    query.flush();'
            print >>strm, '    conn.commit();'
            print >>strm, '    return true;'
        else:
            print >>strm, '    bool ret = query.update() > 0;'
            print >>strm, '    conn.commit();'
            print >>strm, '    return ret;'
        print >>strm, '  }'
        print >>strm, '  return false;'
        print >>strm, '}\n'
    
    def printDelImplementation(self, strm):
        print >>strm, 'bool %s::del ()' % self.name
        print >>strm, '{'
        if self.hasField('NAME') and self.hasField('STATUS'):
            print >>strm, '  DB::Query  query(conn.get_query("UPDATE %s SET"' % self.dbName
            comm = ' '
            if self.uniqueName().upper() != 'CHANNEL':
                print >>strm, '    "    STATUS = \'D\' "'
                comm = ','
            print >>strm, '    "  %s NAME = concat(NAME, CONCAT(\'-D-\', TO_CHAR(now(), \'YYMMDDhhmmss\'))) "'%comm
            print >>strm, '    "WHERE %s"));'%self.id.dbVarsAssignDeclList()
            print >>strm, '  query %s;' % self.id.varsLShiftList()
            print >>strm, '  bool ret = query.update() > 0;'
            print >>strm, '  conn.commit();'
            print >>strm, '  this->select();'
            print >>strm, '  return ret;'
        else:
            print >>strm, '  return delet();'
        print >>strm, '}\n'
    
    def printDeleteImplementation(self, strm):
        print >>strm, 'bool %s::delet ()' % self.name
        print >>strm, '{'
        print >>strm, '  DB::Query  query(conn.get_query("DELETE FROM %s WHERE %s"));'%(self.dbName, self.id.dbVarsAssignDeclList())
        print >>strm, '  query %s;' % self.id.varsLShiftList()
        print >>strm, '  bool ret = query.update() > 0;'
        print >>strm, '  conn.commit();'
        print >>strm, '  return ret;'
        print >>strm, '}\n'
    
    def printTouchImplementation(self, strm):
        print >>strm, 'bool %s::touch ()' % self.name
        print >>strm, '{'
        print >>strm, '  std::ostringstream strm;'
        print >>strm, '  strm << "UPDATE %s SET VERSION = now()";' % self.dbName
        print >>strm, '  {'
        print >>strm, '    strm << " WHERE %s";' % self.id.dbVarsAssignDeclList()
        print >>strm, '    DB::Query  query(conn.get_query(strm.str()));'
        print >>strm, '    query %s;' % self.id.varsLShiftList()
        print >>strm, '    bool ret = query.update() > 0;'
        print >>strm, '    conn.commit();'
        print >>strm, '    return ret;'
        print >>strm, '  }'
        print >>strm, '  return false;'
        print >>strm, '}\n'
    
    def printConstructDeclaration (self, strm, name, end = True):
        def printme (sep):
            print >>strm, '%s (DB::IConn& connection,' % name
            self.id.printVarsDeclList(strm)
            print >>strm, ')%s' % sep
        if end:
            strm.addLevel()
            printme(';')
            strm.delLevel()
        else:
            printme('')
    
    def printConstructImplementation (self, strm, connection_type):
        #constructor
        self.printConstructDeclaration(strm, self.constructorName(), False)
        #print members initialisation
        strm.write('  :ORMObject<%s>(connection),\n' % connection_type)
        strm.write('  %s\n'% self.id.varsInitList())
        if len(self.fields) > 0:
            for field in self.fields:
                field.printInit(strm)
            strm.newline()
        print >>strm, '{\n'
        self.id.printFreeVarsAssignList(strm, 0, 'this->')
        #strm.write('  %s'% self.id.fieldsInitList())
        print >>strm, '}\n'
        #copy constructor
        print >>strm, '%s::%s(const %s& from)' % (self.name, self.name, self.name)
        #print members initialisation
        strm.write('  :ORMObject<%s>(from), %s' % \
                   (connection_type, self.id.varsCopyList('from')))
        if self.id.indexesCount() > 0:
            strm.rawwrite(',\n')
            last = self.id.indexesCount()-1
            for field in self.id.fields[:last]:
                print >>strm, '  %s(from.%s),' % (field.name, field.name)
            print >>strm, '  %s(from.%s)' % (self.id.fields[last].name, self.id.fields[last].name)
        else:
            strm.newline()
        if self.fieldsCount() > 0:
            strm.rawwrite(',\n')
            last = self.fieldsCount() - 1
            for field in self.fields[:last]:
                print >>strm, '  %s(from.%s),' % (field.name, field.name)
            print >>strm, '  %s(from.%s)' % (self.fields[last].name, self.fields[last].name)
        else:
            strm.newline()
        print >>strm, '{'
        print >>strm, '}\n'
        #assigment
        print >>strm, '%s& %s::operator=(const %s& from)' % (self.name, self.name, self.name)
        print >>strm, '{'
        print >>strm, '  Unused(from);'
        for field in self.fields:
            print >>strm, '  %s = from.%s;' % (field.name, field.name)
        for field in self.id.fields:
            print >>strm, '  %s = from.%s;' % (field.name, field.name)
        print >>strm, '  return *this;'
        print >>strm, '}\n'
    
    def printImplementation(self, strm, connection_type):
        if not self.used:
            return        
        # print fields map
        print >>strm, 'ORMObjectMember %s::members_[%i] = {' % (self.name, self.fieldsCount() + 2*self.id.indexesCount())
        # print fields definitions for map and mumerate them
        for field in self.fields:
            field.printDef(strm, self.name)
        for field in self.id.fields:
            field.printDef(strm, self.name)
        for field in self.id.fields:
            field.printDef(strm, self.name, indexSuffix)
        print >>strm, '};\n'
        # print constructors
        print >>strm, '%s (DB::IConn& connection)' % self.constructorName()
        strm.write('  :ORMObject<%s>(connection)' % connection_type)
        if len(self.id.fields) > 0:
            for field in self.id.fields:
                field.printInit(strm)
            strm.newline()
        if len(self.fields) > 0:
            for field in self.fields:
                field.printInit(strm)
            strm.newline()
        print >>strm, '{}\n'
        self.printConstructImplementation (strm, connection_type)
        # print fields methods if need
        for field in self.fields:
            field.printImplementation(strm, self.dbName, self.id, self.name)
        #print methods
        if self.hasField('VERSION'):
            self.printTouchImplementation(strm)
        if self.fieldsCount() > 0:
            self.printSelectImplementation(strm)
            self.printUpdateImplementation(strm)
        self.printInsertImplementation(strm)
        self.printDeleteImplementation(strm)
        self.printDelImplementation(strm)
        #print asks
        for ask in self.asks:
            ask.printImplementation(strm, self)
    
    def __makeSelf (self):
        index = 0
        for field in self.fields:
            field.index = index
            index = index +1
        for field in self.id.fields:
            field.index = index
            index = index +1
    
    def printDeclaration (self, strm, connection_type):
        self.__makeSelf()
        if not self.used:
            return        
        print >>strm, 'class %s:' % self.name
        print >>strm, '  public ORMObject<%s>' % connection_type
        print >>strm, '{'
        print >>strm, 'protected:'
        print >>strm, '  static ORMObjectMember members_[%i];' % (self.fieldsCount() + 2*self.id.indexesCount())
        strm.newline()
        self.id.printIndexVars(strm)
        strm.newline()
        print >>strm, 'public:'
        strm.newline()
        self.id.printVars(strm)
        #print fields
        for field in self.fields:
            field.printDeclaration(strm)
        strm.newline()
        #print readonly access to indexes
        self.id.printIndexAccess(strm)
        strm.newline()
        #basic constructors
        print >>strm, '  %s (DB::IConn& connection);' % self.name
        print >>strm, '  %s (const %s& from);' % (self.name, self.name)
        print >>strm, '  %s& operator=(const %s& from);' % (self.name, self.name)
        strm.newline()
        #indexed constructors
        self.printConstructDeclaration (strm, self.name)
        if self.hasField('VERSION'):
            strm.newline()
            print >>strm, '  virtual bool touch (); //!< touch version'
        strm.newline()
        #methods
        if self.fieldsCount() > 0:
            print >>strm, '  virtual bool select (); //!< get exists'
            self.id.printIdVarSelect (strm)
            print >>strm, '  virtual bool update (bool set_defaults = true); //!< update exists'
            self.id.printIdVarUpdate (strm)
        print >>strm, '  virtual bool insert (bool set_defaults = true); //!< create new'
        self.id.printIdVarInsert (strm)
        print >>strm, '  virtual bool delet   (); //!< delete exists'
        print >>strm, '  virtual bool del     (); //!< set status D and name->name+timestamp'
        self.id.printIdVarDel (strm)
        #asks
        if len(self.asks) > 0:
            strm.newline()
            for ask in self.asks:
                ask.printDeclaration(strm, self)
        print >>strm, '};\n'

