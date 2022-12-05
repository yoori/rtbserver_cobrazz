
from types import *
import copy

stats = []

BasicStats = "BasicStats"

class Field:
  # named field for admin command
  def __init__(self, name, dbname, do_not_init = False):
    self.name = name
    self.print_name = name
    self.default = None
    self.required = True
    self.do_not_init = do_not_init
    if dbname:
      self.dbname = dbname
    else:
      self.dbname = name
    self.dbname = self.dbname.replace('\n', '').strip()

  def dbset(self, key):
    suffix = " = :\" << %s++;" % key
    return '\"' + self.dbname + suffix

  def access(self, name):
    return name + self.name + "()"

  def print_field(self, key):
    return  """
        out << \"%s = \";
        if (%s.%s_is_null())
          out << \"null\";
        else
          out << %s.%s();""" % \
  (self.name, key, self.name, key, self.name)
  
  def get_ask(self):
    return self.dbname

class Base:
  def __init__(self, name, sub = None):
    self.name = name
    self.sub = sub

  def __call__(self, sub):
    self.sub = sub
    return self

class SubBase(Base):
  def __init__(self, name):
    Base.__init__(self, name)

class Stats:

  def __init__(self, name, table, tbase, keys = None, fields = None):
    self.name = name
    self.table = table
    self.table_enum = False
    self.multi_table = False
    self.fields = fields and fields or []
    if isinstance(table, list):
      if isinstance(table[0], StringType):
        self.table_enum = True
      else:
        self.multi_table = True
        for stat in self.table:
          for field in stat.fields:
            field.print_name = stat.name + '.' + field.name
            field.name = stat.name.lower() + '_' + field.name
            self.fields.append(field)
    self.tbase = tbase
    self.subbase = self.tbase.sub
    self.keys = keys
    if not fields:
      stats.append(self)

  def get_template( self, base ):
    return "%s<%s, %i>" % (base, self.name, len(self.fields))

  # sort fields
  def postprocess(self):
    self.template = self.get_template(self.tbase.name)

  #def process_keyset_args(self, strm):
    
  def process_declaration(self, strm):
    fieldsCount = len(self.fields) - 1
    keysCount = len(self.keys)
    print >>strm, "class %s:" % self.name
    print >>strm, "  public %s" % self.template
    print >>strm, "{"
    strm.addLevel()    
    print >>strm, "protected:"
    strm.addLevel()
    # query_select_
    print >>strm, "bool query_select_ (StatsDB::IConn& connection);"
    strm.delLevel()
    print >>strm, "public:"
    # fields enum
    strm.addLevel()
    print >>strm, 'enum FieldName'
    print >>strm, '{'
    strm.addLevel()
    if fieldsCount > 0:
      suffix = " = 0,"
    else:
      suffix = " = 0"
    for i in xrange(0, fieldsCount):
      field = self.fields[i]
      print >>strm, "%s%s" % (field.name.upper(), suffix)
      suffix = ","
    print >>strm, "%s" % self.fields[fieldsCount].name.upper()
    strm.delLevel()
    print >>strm, '};'
    if self.table_enum:
      print >>strm, 'enum Table'
      print >>strm, '{'
      strm.addLevel()
      suffix = " = 0,"
      count = len(self.table) - 1
      for i in xrange(0, count):
        table = self.table[i]
        print >>strm, "%s%s" % (table, suffix)
        suffix = ","
      print >>strm, "%s" % self.table[count]
      strm.delLevel()
      print >>strm, '};'
    # Diffs
    print >>strm, "typedef %s Base;" % self.template    
    print >>strm, 'typedef Base::Diffs Diffs;'
    # class declaration
    strm.delLevel()
    print >>strm, "public:"
    strm.addLevel()
    # key
    if self.subbase:
      print >>strm, "class Key:"
      print >>strm, "  public %s" % self.subbase.name
    else:
      print >>strm, "class Key"      
    print >>strm, "{"
    strm.addLevel()
    print >>strm, "protected:"
    strm.addLevel()
    for i in xrange(0, keysCount):
      print >>strm, "%s %s_;" % (self.keys[i].type, self.keys[i].name)
      print >>strm, "bool %s_used_;" % self.keys[i].name
      print >>strm, "bool %s_null_;" % self.keys[i].name
    strm.delLevel()
    print >>strm, "public:"
    strm.addLevel()
    for i in xrange(0, keysCount):
      print >>strm, "Key& %s(const %s& value);" % (self.keys[i].name, self.keys[i].type)
      print >>strm, "Key& %s_set_null(bool is_null = true);" % self.keys[i].name
      print >>strm, "const %s& %s() const;" % (self.keys[i].type, self.keys[i].name)
      print >>strm, "bool %s_used() const;" % self.keys[i].name
      if (self.keys[i].do_not_init):
          print >>strm, "Key& %s_set_used(bool used);" % self.keys[i].name          
      print >>strm, "bool %s_is_null() const;" % self.keys[i].name
    print >>strm, "Key ();"
    print >>strm, "Key ("
    strm.addLevel()
    suffix = ''
    if keysCount > 1:
      suffix = ','
    for i in xrange(0, keysCount-1):
      print >>strm, "const %s& %s%s" % (self.keys[i].type, self.keys[i].name, suffix)
    i = keysCount-1
    print >>strm, "const %s& %s" % (self.keys[i].type, self.keys[i].name)
    strm.delLevel()
    print >>strm, ");"
    strm.delLevel()
    strm.delLevel()
    print >>strm, "};"
    # values
    for i in xrange(0, len(self.fields)):
      field = self.fields[i]
      print >>strm, "stats_value_type %s () const;" % field.name
    # print_idname
    print >>strm, "void print_idname (std::ostream& out) const;"
    print >>strm, ""
    # constructors
    print >>strm, "%s (const Key& value);" % self.name
    print >>strm, "%s (" % self.name
    strm.addLevel()
    suffix = ''
    if keysCount > 1:
      suffix = ','
    for i in xrange(0, keysCount-1):
      print >>strm, "const %s& %s%s" % (self.keys[i].type, self.keys[i].name, suffix)
    i = keysCount-1
    print >>strm, "const %s& %s" % (self.keys[i].type, self.keys[i].name)
    strm.delLevel()
    print >>strm, ");"
    print >>strm, "%s ();" % self.name
    if self.table_enum:
      print >>strm, "%s& table(const Table&);" % self.name
    # key method
    print >>strm, "Key& key ();"
    print >>strm, "Key& key (const Key& value);"
    print >>strm, "Key& key ("
    strm.addLevel()
    suffix = ''
    if keysCount > 1:
      suffix = ','
    for i in xrange(0, keysCount-1):
      print >>strm, "const %s& %s%s" % (self.keys[i].type, self.keys[i].name, suffix)
    i = keysCount-1
    print >>strm, "const %s& %s" % (self.keys[i].type, self.keys[i].name)
    strm.delLevel()
    print >>strm, ");"
    print >>strm, ""
    strm.delLevel()
    print >>strm, "protected:"
    strm.addLevel()
    # key value
    print >>strm, "Key key_;"
    if self.table_enum:
      print >>strm, "Table table_;"
      print >>strm, "static const char* tables[%i];" % len(self.table)
    strm.delLevel()
    strm.delLevel()
    print >>strm, '};'
    print >>strm, ""
    print >>strm, "template<>"
    print >>strm, 'class %s::Diffs' % self.template
    print >>strm, '{'
    strm.addLevel()
    print >>strm, "public:"
    strm.addLevel()
    print >>strm, "typedef stats_diff_type array_type[%i];" % len(self.fields)
    print >>strm, "typedef const stats_diff_type const_array_type[%i];" % len(self.fields)
    print >>strm, "typedef const const_array_type& const_array_type_ref;"
    print >>strm, "typedef const stats_diff_type* const_iterator;"
    print >>strm, ""    
    print >>strm, "Diffs();"
    print >>strm, "Diffs(const stats_diff_type& value);"
    print >>strm, "Diffs(const const_array_type& array);"
    print >>strm, "operator const_array_type_ref () const;"    
    print >>strm, "Diffs& operator= (const const_array_type& array);"
    print >>strm, "Diffs& operator+= (const const_array_type& array);"
    print >>strm, "Diffs& operator-= (const const_array_type& array);"
    print >>strm, "Diffs operator+ (const const_array_type& array);"
    print >>strm, "Diffs operator- (const const_array_type& array);"    
    print >>strm, ""
    print >>strm, "stats_diff_type& operator[] (int i);"
    print >>strm, "const stats_diff_type& operator[] (int i) const;"
    print >>strm, ''
    print >>strm, "const_iterator begin() const;"
    print >>strm, "const_iterator end() const;"
    print >>strm, "int size() const;"
    print >>strm, "void clear();"
    print >>strm, ''
    # Diffs accessors
    for i in xrange(0, len(self.fields)):
      field = self.fields[i]
      print >>strm, "Diffs& %s (const stats_diff_type& value);" % field.name
      print >>strm, "const stats_diff_type& %s () const;" % field.name
    strm.delLevel()
    print >>strm, 'protected:'
    print >>strm, "  stats_diff_type diffs[%i];" % len(self.fields)
    strm.delLevel()
    print >>strm, "};"
    print >>strm, ""    

  def process_implementation(self, strm):
    keysCount = len(self.keys)    
    expdiff_name = "%s::Diffs" % self.template
    print >>strm, "///////////////////////////////// %s" % self.name    
    # constructor
    print >>strm, "inline"
    print >>strm, "%s::%s ()" % (self.name, self.name)
    print >>strm, "  :Base(\"%s\")" % self.name
    if self.table_enum:
      print >>strm, '{ table_ = %s; }' % self.table[0];
    else:
      print >>strm, '{}'
    #
    print >>strm, "inline"
    print >>strm, "%s::%s (const %s::Key& value)" % (self.name, self.name, self.name)
    print >>strm, "  :Base(\"%s\")" % self.name
    print >>strm, '{'
    strm.addLevel()
    if self.table_enum:
      print >>strm, 'table_ = %s;' % self.table[0];
    print >>strm, 'key_ = value;'
    strm.delLevel()
    print >>strm, '}'
    #
    print >>strm, "inline"
    print >>strm, "%s::%s (" % (self.name, self.name)
    strm.addLevel()
    suffix = ''
    if keysCount > 1:
      suffix = ','
    for i in xrange(0, keysCount-1):
      print >>strm, "const %s& %s%s" % (self.keys[i].type, self.keys[i].name, suffix)
    i = keysCount-1
    print >>strm, "const %s& %s" % (self.keys[i].type, self.keys[i].name)
    strm.delLevel()
    print >>strm, ')'
    print >>strm, "  :Base(\"%s\")" % self.name
    print >>strm, '{'
    strm.addLevel()
    if self.table_enum:
      print >>strm, 'table_ = %s;' % self.table[0];
    print >>strm, 'key_ = Key ('
    strm.addLevel()
    for i in xrange(0, keysCount-1):
      print >>strm, "%s," % self.keys[i].name
    i = keysCount-1
    print >>strm, self.keys[i].name
    strm.delLevel()
    print >>strm, ');'
    strm.delLevel()
    print >>strm, '}'
    # Table
    if self.table_enum:
      print >>strm, "inline"
      print >>strm, "%s&" % self.name
      print >>strm, "%s::table(const Table& value)" % self.name
      print >>strm, '{'
      strm.addLevel()
      print >>strm, 'table_ = value;'
      print >>strm, 'return *this;'
      strm.delLevel()
      print >>strm, '}'
    # Key
    print >>strm, "inline"
    init = ''
    sep = ''
    for i in xrange(0, keysCount):
      if self.keys[i].default:
        init = init + "%s%s_(%s)" % (sep, self.keys[i].name, self.keys[i].default)
        sep = ','
    print >>strm, "%s::Key::Key ()" % self.name
    if init:
      print >>strm, '  :' + init
    print >>strm, '{'
    strm.addLevel()
    for i in xrange(0, keysCount):
      print >>strm, "%s_used_ = false;" % self.keys[i].name
      print >>strm, "%s_null_ = false;" % self.keys[i].name
    strm.delLevel()
    print >>strm, '}'
    init = ''
    sep = ''
    for i in xrange(0, keysCount):
      if self.keys[i].default:
        init = init + "%s%s_(%s)" % (sep, self.keys[i].name, self.keys[i].name)
        sep = ','    
    print >>strm, "inline"
    print >>strm, "%s::Key::Key (" % self.name
    strm.addLevel()
    suffix = ''
    if keysCount > 1:
      suffix = ','
    for i in xrange(0, keysCount-1):
      print >>strm, "const %s& %s%s" % (self.keys[i].type, self.keys[i].name, suffix)
    i = keysCount-1
    print >>strm, "const %s& %s" % (self.keys[i].type, self.keys[i].name)
    strm.delLevel()
    print >>strm, ')'
    if init:
      print >>strm, '  :' + init    
    print >>strm, '{'
    strm.addLevel()
    for i in xrange(0, keysCount):
      if not self.keys[i].default:
        print >>strm, "%s_ = %s;" % (self.keys[i].name, self.keys[i].name)
      print >>strm, "%s_used_ = true;" % self.keys[i].name
      print >>strm, "%s_null_ = false;" % self.keys[i].name
    strm.delLevel()
    print >>strm, '}'
    #Keys accessors
    for key in self.keys:
      print >>strm, "inline"
      print >>strm, "%s::Key&" % self.name
      print >>strm, "%s::Key::%s(const %s& value)" % (self.name, key.name, key.type)
      print >>strm, '{'
      strm.addLevel()
      print >>strm, "%s_ = value;" % key.name
      print >>strm, "%s_used_ = true;" % key.name
      print >>strm, "%s_null_ = false;" % key.name
      print >>strm, 'return *this;'
      strm.delLevel()
      print >>strm, '}'
      print >>strm, "inline"
      print >>strm, "%s::Key&" % self.name
      print >>strm, "%s::Key::%s_set_null(bool is_null)" % (self.name, key.name)
      print >>strm, '{'
      strm.addLevel()
      print >>strm, "%s_used_ = true;" % key.name
      print >>strm, "%s_null_ = is_null;" % key.name
      print >>strm, 'return *this;'
      strm.delLevel()
      print >>strm, '}'
      print >>strm, "inline"
      print >>strm, "const %s&" % key.type
      print >>strm, "%s::Key::%s() const" % (self.name, key.name)
      print >>strm, '{'
      strm.addLevel()
      print >>strm, 'return %s_;' % key.name
      strm.delLevel()
      print >>strm, '}'
      print >>strm, "inline"
      print >>strm, "bool"
      print >>strm, "%s::Key::%s_used() const" % (self.name, key.name)
      print >>strm, '{'
      strm.addLevel()
      print >>strm, 'return %s_used_;' % key.name
      strm.delLevel()
      print >>strm, '}'
      if (key.do_not_init):
          print >>strm, "inline"
          print >>strm, "%s::Key&" % self.name
          print >>strm, "%s::Key::%s_set_used(bool used)" % (self.name, key.name)
          print >>strm, '{'
          strm.addLevel()
          print >>strm, '%s_used_ = used;' % key.name
          print >>strm, 'return *this;'
          strm.delLevel()
          print >>strm, '}'
      print >>strm, "inline"
      print >>strm, "bool"
      print >>strm, "%s::Key::%s_is_null() const" % (self.name, key.name)
      print >>strm, '{'
      strm.addLevel()
      print >>strm, 'return %s_null_;' % key.name
      strm.delLevel()
      print >>strm, '}'
    # key methods
    print >>strm, "inline"
    print >>strm, "%s::Key&" % self.name
    print >>strm, "%s::key (" % self.name
    strm.addLevel()
    suffix = ''
    if keysCount > 1:
      suffix = ','
    for i in xrange(0, keysCount-1):
      print >>strm, "const %s& %s%s" % (self.keys[i].type, self.keys[i].name, suffix)
    i = keysCount-1
    print >>strm, "const %s& %s" % (self.keys[i].type, self.keys[i].name)
    strm.delLevel()
    print >>strm, ')'
    print >>strm, '{'
    strm.addLevel()
    print >>strm, 'key_ = Key ('
    strm.addLevel()
    for i in xrange(0, keysCount-1):
      print >>strm, "%s," % self.keys[i].name
    i = keysCount-1
    print >>strm, self.keys[i].name
    strm.delLevel()
    print >>strm, ');'
    print >>strm, 'return key_;'
    strm.delLevel()
    print >>strm, '}'    
    #
    print >>strm, "inline"
    print >>strm, "%s::Key&" % self.name
    print >>strm, "%s::key ()" % self.name
    print >>strm, '{'
    strm.addLevel()
    print >>strm, 'return key_;'
    strm.delLevel()
    print >>strm, '}'
    print >>strm, "inline"
    print >>strm, "%s::Key&" % self.name
    print >>strm, "%s::key (const %s::Key& value)" % (self.name, self.name)
    print >>strm, '{'
    strm.addLevel()
    print >>strm, 'key_ = value;'
    print >>strm, 'return key_;'
    strm.delLevel()
    print >>strm, '}'
    # Diff constructors
    print >>strm, "inline"
    print >>strm, "%s::Diffs()" % expdiff_name
    print >>strm, "{"
    strm.addLevel()
    print >>strm, "for(int i = 0; i < %i; ++i)" % len(self.fields)
    print >>strm, "  diffs[i] = any_stats_diff;"
    strm.delLevel()
    print >>strm, "}"
    print >>strm, "inline"
    print >>strm, "%s::Diffs(const stats_diff_type& value)" % expdiff_name
    print >>strm, "{"
    strm.addLevel()
    print >>strm, "for(int i = 0; i < %i; ++i)" % len(self.fields)
    print >>strm, "  diffs[i] = value;"
    strm.delLevel()
    print >>strm, "}"
    print >>strm, "inline"
    print >>strm, "%s::Diffs(const const_array_type& array)" % expdiff_name
    print >>strm, "{"
    strm.addLevel()
    print >>strm, "for(int i = 0; i < %i; ++i)" % len(self.fields)
    print >>strm, "  diffs[i] = array[i];"
    strm.delLevel()
    print >>strm, "}"
    #
    print >>strm, "inline"
    print >>strm, "%s::operator const_array_type_ref () const" % expdiff_name
    print >>strm, "{"
    strm.addLevel()
    print >>strm, 'return diffs;'
    strm.delLevel()
    print >>strm, "}"    
    print >>strm, "inline"
    print >>strm, "%s& " % expdiff_name    
    print >>strm, "%s::operator=(const const_array_type& array)" % expdiff_name
    print >>strm, "{"
    strm.addLevel()
    print >>strm, "for(int i = 0; i < %i; ++i)" % len(self.fields)
    print >>strm, "  diffs[i] = array[i];"
    print >>strm, 'return *this;'
    strm.delLevel()
    print >>strm, "}"
    #
    print >>strm, "inline"
    print >>strm, "%s& " % expdiff_name    
    print >>strm, "%s::operator+=(const const_array_type& array)" % expdiff_name
    print >>strm, "{"
    strm.addLevel()
    print >>strm, "for(int i = 0; i < %i; ++i)" % len(self.fields)
    print >>strm, "  diffs[i] += array[i];"
    print >>strm, 'return *this;'
    strm.delLevel()
    print >>strm, "}"
    print >>strm, "inline"
    print >>strm, "%s& " % expdiff_name    
    print >>strm, "%s::operator-=(const const_array_type& array)" % expdiff_name
    print >>strm, "{"
    strm.addLevel()
    print >>strm, "for(int i = 0; i < %i; ++i)" % len(self.fields)
    print >>strm, "  diffs[i] -= array[i];"
    print >>strm, 'return *this;'
    strm.delLevel()
    print >>strm, "}"
    #
    print >>strm, "inline"
    print >>strm, "%s " % expdiff_name    
    print >>strm, "%s::operator+(const const_array_type& array)" % expdiff_name
    print >>strm, "{"
    strm.addLevel()
    print >>strm, "%s ret = *this;" % expdiff_name
    print >>strm, "ret += array;"
    print >>strm, 'return ret;'
    strm.delLevel()
    print >>strm, "}"
    print >>strm, "inline"
    print >>strm, "%s " % expdiff_name    
    print >>strm, "%s::operator-(const const_array_type& array)" % expdiff_name
    print >>strm, "{"
    strm.addLevel()
    print >>strm, "%s ret = *this;" % expdiff_name
    print >>strm, "ret -= array;"
    print >>strm, 'return ret;'
    strm.delLevel()
    print >>strm, "}"    
    # iterators
    print >>strm, "inline"
    print >>strm, "%s::const_iterator" % expdiff_name
    print >>strm, "%s::begin () const" % expdiff_name
    print >>strm, "{"
    strm.addLevel()
    print >>strm, "return diffs;"
    strm.delLevel()
    print >>strm, "}"
    print >>strm, "inline"
    print >>strm, "%s::const_iterator" % expdiff_name
    print >>strm, "%s::end () const" % expdiff_name
    print >>strm, "{"
    strm.addLevel()
    print >>strm, "return diffs + %i;" % len(self.fields)
    strm.delLevel()
    print >>strm, "}"
    # Diff index accessors
    print >>strm, "inline"
    print >>strm, "stats_diff_type&"
    print >>strm, "%s::operator[] (int i)" % expdiff_name
    print >>strm, "{"
    strm.addLevel()
    print >>strm, "return diffs[i];"
    strm.delLevel()
    print >>strm, "}"
    print >>strm, "inline"
    print >>strm, "const stats_diff_type&"
    print >>strm, "%s::operator[] (int i) const" % expdiff_name
    print >>strm, "{"
    strm.addLevel()
    print >>strm, "return diffs[i];"
    strm.delLevel()
    print >>strm, "}"
    #print >>strm, "unsigned int size() const;"
    #print >>strm, "void clear();"
    print >>strm, "inline"
    print >>strm, "int"
    print >>strm, "%s::size () const" % expdiff_name
    print >>strm, "{"
    strm.addLevel()
    print >>strm, "return %i;" % len(self.fields)
    strm.delLevel()
    print >>strm, "}"
    print >>strm, "inline"
    print >>strm, "void"
    print >>strm, "%s::clear ()" % expdiff_name
    print >>strm, "{"
    strm.addLevel()
    print >>strm, "for (unsigned int i = 0; i < %i; ++i)" % len(self.fields)
    print >>strm, "{"
    strm.addLevel()
    print >>strm, "diffs[i] = stats_diff_type();"
    strm.delLevel()
    print >>strm, "}"
    strm.delLevel()
    print >>strm, "}"
    # values accessors
    for i in xrange(0, len(self.fields)):
      field = self.fields[i]
      print >>strm, "inline"
      print >>strm, "stats_value_type"
      print >>strm, "%s::%s () const" % (self.name, field.name)
      print >>strm, "{"
      strm.addLevel()      
      print >>strm, "return values[%s::%s];"  % (self.name, field.name.upper())
      strm.delLevel()
      print >>strm, "}"
      print >>strm, "inline"
      print >>strm, "%s& " % expdiff_name
      print >>strm, "%s::%s (const stats_diff_type& value)" % (expdiff_name, field.name)
      print >>strm, '{'
      strm.addLevel()
      print >>strm, 'diffs[%s::%s] = value;' % (self.name, field.name.upper())
      print >>strm, 'return *this;'
      strm.delLevel()
      print >>strm, '}'
      print >>strm, "inline"
      print >>strm, "const stats_diff_type& "
      print >>strm, "%s::%s () const" % (expdiff_name, field.name)
      print >>strm, '{'
      strm.addLevel()
      print >>strm, 'return diffs[%s::%s];' % (self.name, field.name.upper())
      strm.delLevel()
      print >>strm, '}'

  def process_db_select(self, strm, dbtable, stat):
    fieldsCount = len(stat.fields) - 1
    print >>strm, "\"SELECT \""
    strm.addLevel()
    for i in xrange(0, fieldsCount):
      print >>strm, "\"%s, \"" % stat.fields[i].get_ask()
    print >>strm, "\"%s \"" % stat.fields[fieldsCount].get_ask()
    strm.delLevel()
    if self.table_enum:
      print >>strm, "\"FROM \" << tables[table_] << \" \""
    else:
      table_list = dbtable.split('\n')
      print >>strm, "\"FROM %s \"" % table_list[0]
      for table in table_list[1:]:
        print >>strm, "\"%s \"" % table
    print >>strm, "\"WHERE \";"

  def process_keys(self, strm, stat):
    for key in stat.keys:
      additional_cond = ""
      if isinstance(key, PgDateField):
        additional_cond = " &&\n%s  key_.%s() != Generics::Time::ZERO" % \
                          (' '*strm.level,  key.name) 
      print >>strm, "if (key_.%s_used() && !key_.%s_is_null()%s)" % \
            (key.name, key.name, additional_cond)
      strm.addLevel()
      print >>strm, "query.set(%s);" % key.access("key_.")
      strm.delLevel()

  def process_stat(self, strm, stat, sep):
    strm.addLevel()
    print >>strm, "qstr << \"(\""
    self.process_db_select(strm, stat.table, stat)
    if stat.keys:
      print >>strm, "qstr << key_%s << \")%s\";" % (stat.name, sep)
    else:
      print >>strm, "qstr << key_%s << \")%s\";" % (self.name, sep)
    strm.delLevel()

  def process_key_str(self, strm, stat):
    print >>strm, "std::string key_%s;" % stat.name
    print >>strm, "{"    
    strm.addLevel()
    print >>strm, "std::ostringstream kstr;"
    print >>strm, "int ckey = 1;"
    print >>strm, "const char* sep = \"\";"
    print >>strm, "static const char* fsep = \" and \";"
    # db select keys
    for key in stat.keys:
      print >>strm, "if (key_.%s_used())" % key.name
      print >>strm, "{"
      strm.addLevel()
      print >>strm, "if (key_.%s_is_null())" % key.name
      print >>strm, "{"
      strm.addLevel()
      print >>strm, "kstr << sep << \"%s is null\";" % key.dbname
      print >>strm, "sep = fsep;"
      strm.delLevel()
      print >>strm, "}"
      if isinstance(key, PgDateField):
        print >>strm, "else if (key_.%s() == Generics::Time::ZERO)" % key.name
        print >>strm, "{"
        strm.addLevel()
        print >>strm, "kstr << sep << \"%s = '-infinity'\";" % key.dbname
        print >>strm, "sep = fsep;"
        strm.delLevel()
        print >>strm, "}"
      print >>strm, "else"
      print >>strm, "{"
      strm.addLevel()
      print >>strm, "kstr << sep << %s" % key.dbset('ckey')
      print >>strm, "sep = fsep;"
      strm.delLevel()
      print >>strm, "}"
      strm.delLevel()
      print >>strm, "}"
      prefix = "and "
    print >>strm, "key_%s = kstr.str();" % stat.name      
    strm.delLevel()
    print >>strm, "}"
    
  def process_data(self, strm):
    fieldsCount = len(self.fields) - 1
    keysCount = len(self.keys)
    # field names
    print >>strm, "///////////////////////////////// %s" % self.name
    print >>strm, "template<>"
    print >>strm, "%s::names_type const" % self.get_template(BasicStats)
    print >>strm, "%s::field_names = {" % self.get_template(BasicStats)
    strm.addLevel()
    for i in xrange(0, fieldsCount):
      print >>strm, "\".%s\"," % self.fields[i].print_name
    print >>strm, "\".%s\"" % self.fields[fieldsCount].print_name
    strm.delLevel()
    print >>strm, "};"
    # tables
    if self.table_enum:
      print >>strm, "const char*"
      print >>strm, "%s::tables[%i] = {" % (self.name, len(self.table))
      strm.addLevel()
      count = len(self.table) -1
      for i in xrange(0, count):
        print >>strm, "\"%s\"," % self.table[i]
      print >>strm, "\"%s\"" % self.table[count]
      strm.delLevel()
      print >>strm, "};"    
    # print_idname
    print >>strm, "void"
    print >>strm, "%s::print_idname (std::ostream& out) const" % self.name
    print >>strm, "{"
    strm.addLevel()
    print >>strm, "const char* sep = \" \";"
    print >>strm, "static const char* fsep = \", \";"
    print >>strm, "out << description_ << \" <\";"
    for key in self.keys:
      print >>strm, "if (key_.%s_used())" % key.name
      print >>strm, "{"
      strm.addLevel()
      print >>strm, "out << sep; %s" % key.print_field('key_')
      print >>strm, "sep = fsep;"
      strm.delLevel()
      print >>strm, "}"
    print >>strm, "out << \" >\" << std::endl;"
    strm.delLevel()
    print >>strm, "}"
    # query_select_
    print >>strm, "bool"
    print >>strm, "%s::query_select_(StatsDB::IConn& connection)" % self.name
    print >>strm, "{"
       
    strm.addLevel()
    not_inited = filter(lambda x: x.do_not_init, self.keys)
    if not_inited:
        print >>strm, "Key stored_key(key_);"
        for k in not_inited:
            print >>strm, "if (key_.%s_used() && initial_)" % key.name
            print >>strm, "{"
            print >>strm, "   key_.%s_set_used(false);" % key.name
            print >>strm, "}"
    if self.multi_table:
      self.process_key_str(strm, self)
      for stat in self.table:
        if stat.keys:
          self.process_key_str(strm, stat)
    else:
      self.process_key_str(strm, self)
    print >>strm, "std::ostringstream qstr;"
    print >>strm, "qstr <<"
    strm.addLevel()
    # db select
    if self.multi_table:
      print >>strm, "\"SELECT * FROM \";"
      statsCount = len(self.table) - 1
      for i in xrange(0, statsCount):
        stat = self.table[i]
        self.process_stat(strm, stat, ' AS %s,' % stat.table)
      self.process_stat(strm, self.table[statsCount], ' AS %s' % self.table[statsCount].table)
      strm.delLevel()
      print >>strm, "StatsDB::Query query(connection.get_query(qstr.str()));"
      # set keys
      for stat in self.table:
        if stat.keys:
          self.process_keys(strm, stat)
        else:
          self.process_keys(strm, self)
    else:
      self.process_db_select(strm, self.table, self)
      strm.delLevel()
      print >>strm, "qstr << key_%s;" % self.name
      print >>strm, "StatsDB::Query query(connection.get_query(qstr.str()));"      
      # set keys
      self.process_keys(strm, self)
      if not_inited:
        print >>strm, "key_ = stored_key;"
    print >>strm, "return ask(query);"
    strm.delLevel()
    print >>strm, "}"

class StrField(Field):
  def __init__(self, name, dbname, do_not_init = False):
    Field.__init__(self, name, dbname, do_not_init)
    self.type = 'std::string'
    
  def get_ask(self):
    return self.dbname

  def print_field(self, key):
    return  """
        out << \"%s = '\";
        if (%s.%s_is_null())
          out << \"null'\";
        else
          out << %s.%s() << '\\'';""" % \
  (self.name, key, self.name, key, self.name)

  def process_implementation(self, strm, klass):
    pass
    
class IntNumber:
  def __init__(self):
    self.type = 'int'

  def dbset(self, dbname, name):
    suffix = " = :\" << %s++;" % name
    return '\"' + dbname + suffix


class FloatNumber:
  def __init__(self, precision = 2):
    self.type = 'double'
    self.precision = precision

  def __call__(self, precision = 2):
    self.precision = precision
    return self

  def dbset(self, dbname, name):
    return '"TRUNC(%s, %s) = TRUNC(:" << %s++ << ", %s)";' % (dbname, self.precision, name, self.precision)

integer = IntNumber()
float = FloatNumber()

class NumberFiled(Field):
  def __init__(self, name, dbname, format = integer):
    Field.__init__(self, name, dbname)
    self.type = format.type
    self.default = '0'
    self.format = format

  def dbset(self, name):
    return self.format.dbset(self.dbname, name)

  def get_ask(self):
    return self.dbname

  def process_implementation(self, strm, klass):
    pass

class BooleanFiled(Field):
  def __init__(self, name, dbname, format = integer):
    Field.__init__(self, name, dbname)
    self.type = 'bool'
    self.default = 'false'

  def print_field(self, key):
    return  """
        out << \"%s = \";
        if (%s.%s_is_null())
          out << \"null\";
        else
          out << (%s.%s() ? \"true\" : \"false\");""" % \
  (self.name, key, self.name, key, self.name)

class YesNoField(StrField):
  def __init__(self, name, dbname):
    StrField.__init__(self, name, dbname)
    self.type = 'bool'
    self.default = 'false'
  
  def get_ask(self):
    return self.dbname
  
  def access(self, name):
    return name + self.name + "() ? \"Y\" : \"N\""

  def print_field(self, key):
    return  """
        out << \"%s = \";
        if (%s.%s_is_null())
          out << \"null\";
        else
          out << (%s.%s() ? 'Y' : 'N');""" % \
  (self.name, key, self.name, key, self.name)

  def process_implementation(self, strm, klass):
    StrField.process_implementation(self, strm, klass)
  

class Hourly:
  def __init__(self):
    self.format = '%Y-%m-%d:%H'
    
  def dbset(self, dbname, name):
    suffix = "TRUNC(:\" << %s++ << \", 'HH24')\";" % name
    return '\"TRUNC(' + dbname + ', \'HH24\') = ' + suffix

  def access(self, name):
    return "trunc_hourly(" + name + "().get_gm_time())"

class Daily:
  def __init__(self):
    self.format = '%Y-%m-%d'
    
  def dbset(self, dbname, name):
    suffix = "TRUNC(:\" << %s++ << \")\";" % name
    return '\"TRUNC(' + dbname + ') = ' + suffix

  def access(self, name):
    return name + "().get_gm_time().get_date()"

class NativeDate:
  def __init__(self):
    self.format = '%Y-%m-%d:%H-%M-%S'
    
  def dbset(self, dbname, name):
    suffix = ":\" << %s++;" % name
    return '\"' + dbname + ' = ' + suffix

  def access(self, name):
    return name + "().get_gm_time()"

hourly = Hourly()
daily = Daily()
NativeDate = NativeDate()

class OraDateField(StrField):
  def __init__(self, name, dbname, format = daily):
    StrField.__init__(self, name, dbname)
    self.type = 'AutoTest::Time'
    self.default = 'default_date()'
    self.format = format
  
  def dbset(self, name):
    return self.format.dbset(self.dbname, name)
    
  def access(self, name):
    return name + self.name + "().get_gm_time()"

  def print_field(self, key):
    return """
        out << \"%s = '\";
        if (%s.%s_is_null())
          out << \"null\";
        else
          out <<  %s.%s().get_gm_time().format(\"%s\") << '\\'';""" % (
      self.name, key, self.name, key, self.name, self.format.format)
  
  def process_implementation(self, strm, klass):
    StrField.process_implementation(self, strm, klass)

class PgDateField(StrField):
  def __init__(self, name, dbname, format = daily, do_not_init = False):
    StrField.__init__(self, name, dbname, do_not_init)
    self.type = 'AutoTest::Time'
    self.default = 'default_date()'
    self.format = format
  
  def dbset(self, name):
    suffix = "<< %s++;" %name
    return '\"' + self.dbname + ' = :\"' + suffix

  def access(self, name):
    return self.format.access(name + self.name)

  def print_field(self, key):
    return """
        out << \"%s = '\";
        if (%s.%s_is_null())
          out << \"null\";
        else
          out <<  %s.%s().get_gm_time().format(\"%s\") << '\\'';""" % (
      self.name, key, self.name, key, self.name, self.format.format)
 
  def process_implementation(self, strm, klass):
    StrField.process_implementation(self, strm, klass)

class CountField(StrField):

  def __init__(self, field_name = '*'):
    StrField.__init__(self, 'count', 'count')
    self.field_name = field_name
  
  def get_ask(self):
    return "COUNT(%s)" % self.field_name


class SumField(StrField):
 
  def get_ask(self):
    return "SUM(%s)" % self.dbname

class ExcludeField(Field):
  
  def __init__(self, name, dbname, format = integer):
    Field.__init__(self, name, dbname)
    self.type = format.type
    self.format = format
    
  def get_ask(self):
    return self.dbname

  def dbset(self, key):
    suffix = " != :\" << %s++;" % key
    return '\"' + self.dbname + suffix

class MinField(StrField):

  def get_ask(self):
    return "MIN(%s)" % self.dbname

class MaxField(StrField):

  def get_ask(self):
    return "MAX(%s)" % self.dbname

class SomeField(StrField):
  def __init__(self, name, dbname):
    StrField.__init__(self, name, dbname)
  
  def process_implementation(self, strm, klass):
    StrField.process_implementation(self, strm, klass)

def string(name, dbname = None):
    return StrField(name, dbname)

def number(name, dbname = None, format = integer):
  return NumberFiled(name, dbname, format)

def boolean(name, dbname = None):
  return BooleanFiled(name, dbname)

def yesno(name, dbname = None):
  return YesNoField(name, dbname)

def sum(name, dbname = None):
  return SumField(name, dbname)

def count(name = '*'):
  return CountField(name)

def min(name, dbname = None):
  return MinField(name, dbname)

def max(name, dbname = None):
  return MaxField(name, dbname)

def exclude(name, dbname = None, format = integer):
  return ExcludeField(name, dbname, format)

def field(name, dbname = None):
  return SomeField(name, dbname)

def oradate(name, dbname = None, format = daily):
  return OraDateField(name, dbname, format)

def pgdate(name, dbname = None, format = daily, do_not_init = False):
  return PgDateField(name, dbname, format, do_not_init)

class IndentStream:
    def __init__ (self, stream, level = 0):
        self.stream = stream
        self.level  = 2*level

    def close(self):
      self.stream.close()
    
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

def process_namespace(strm, processor):
  print >>strm, "namespace AutoTest"
  print >>strm, "{"
  strm.addLevel()
  print >>strm, "namespace ORM"
  print >>strm, "{"
  strm.addLevel()
  for stat in stats:
    processor(stat, strm)
  strm.delLevel()
  print >>strm, "}"
  strm.delLevel()  
  print >>strm, "}"

def process_stats_namespace(strm, processor, stat):
  print >>strm, "namespace AutoTest"
  print >>strm, "{"
  strm.addLevel()
  print >>strm, "namespace ORM"
  print >>strm, "{"
  strm.addLevel()
  processor(stat, strm)
  strm.delLevel()    
  print >>strm, "}"
  strm.delLevel()  
  print >>strm, "}"

def process_header_header(header, name, includes = []):
  print >>header, "/** $" + "Id" + "$"
  print >>header, "* @file %s" % name
  print >>header, "* Stats access objects"
  print >>header, "* THIS FILE IS AUTOMATICALY GENERATED, ! DON'T EDIT ! "
  print >>header, "*/"
  print >>header, ""
  guard = name.upper().replace('.','_')
  print >>header, "#ifndef __AUTOTESTS_COMMONS_STATS_" + guard
  print >>header, "#define __AUTOTESTS_COMMONS_STATS_" + guard
  print >>header, ""
  for include in includes:
    inc = include[0] == "<" and include or ("\"%s\"" % include)
    print >>header, "#include %s" % inc
  print >>header, ""

def process_header_footer(header, name):
  guard = name.upper().replace('.','_')
  print >>header, "#endif  // __AUTOTESTS_COMMONS_STATS_" + guard
  print >>header, ""

def process_header(header_):
  header = None
  if type(header_) is StringType:
    header = IndentStream(file(header_, 'w+'))
  else:
    header = header_
  process_header_header(header, header_,
    ["<tests/AutoTests/Commons/ORM/ORMStats.hpp>",
     "<tests/AutoTests/Commons/Stats/DiffStats.hpp>"])
  process_namespace(header, Stats.process_declaration)
  process_namespace(header, Stats.process_implementation)
  process_header_footer(header, header_)
  if header != header_:
    header.close()

def process_header_unit(
  name, process_fns, admin, collector,
  if_not_esists = False, includes = []):
  header_name = name + ".hpp"
  print >>collector, "#include <tests/AutoTests/Commons/Stats/%s>" % header_name
  if (not (if_not_esists and os.path.exists(header_name))):
    header = IndentStream(file(header_name, 'w+'))
    process_header_header(header, header_name, includes)
    for fn in process_fns:
      process_stats_namespace(header, fn, admin)
    process_header_footer(header, header_name)
    header.close()

def process_headers():
  collector_name = "Common.hpp"
  collector = IndentStream(file(collector_name, 'w+'))
  process_header_header(collector, collector_name,
    includes = ["<tests/AutoTests/Commons/Stats/ORMStats.hpp>",
                "<tests/AutoTests/Commons/Stats/StatsContainers.hpp>",
                "<tests/AutoTests/Commons/Stats/DiffStats.hpp>"])
  for stat in stats:
    process_header_unit(
      stat.name,
      [Stats.process_declaration, Stats.process_implementation],
      stat, collector,
      includes = ["<tests/AutoTests/Commons/Common.hpp>"])
  print >>collector, ""
  process_header_footer(
    collector, collector_name)
  collector.close()
    
def process_source_header(source, name, includes = []):
  print >>source, "/** $" + "Id" + "$"
  print >>source, "* @file %s" % name
  print >>source, "* Stats access objects"
  print >>source, "* THIS FILE IS AUTOMATICALY GENERATED, ! DON'T EDIT ! "
  print >>source, "*/"
  print >>source, ""
  for include in includes:
    print >>source, "#include \"%s\"" % include
  if not includes:
    print >>source, "#include <tests/AutoTests/Commons/Common.hpp>"
  print >>source, ""

def process_source(source_):
  source = None
  if type(source_) is StringType:
    source = IndentStream(file(source_, 'w+'))
  else:
    source = source_
  process_source_header(source, source_)
  process_namespace(source, Stats.process_data)
  if source != source_:
    source.close()

def process_source_unit(
  name, process_fn, stat,
  if_not_esists = False,
  includes = []) :
  header = name + ".hpp"
  source_name = name + ".cpp"
  if (not (if_not_esists and os.path.exists(source_name))):
    source = IndentStream(file(source_name, 'w+'))
    process_source_header(source, source_name,
                          includes + [header])
    process_stats_namespace(source, process_fn, stat)
    source.close()

def process_sources():
  for stat in stats:
    process_source_unit(stat.name, Stats.process_data, stat)

def process_stats(header_ = None, source_ = None):
  for stat in stats:
    stat.postprocess()
  if header_:
    process_header(header_)
  else:
    process_headers()
  if source_:
    process_source(source_)
  else:
    process_sources()



