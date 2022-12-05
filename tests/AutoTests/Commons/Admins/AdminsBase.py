
from types import *
import copy, os.path

admins = []
checkers_dir_path = "../Checkers/GeneratedAdminsCheckers"

#type tranclators

def null_translator(str):
  return str

def cstr_translator(str):
  return "%s.c_str()" % str

def strof_translator(str):
  return "strof(%s)" % str

def cstrof_translator(str):
  return "strof(%s).c_str()" % str

def none_translator(str):
    return ''

def eq_translator(val):
  if val:
    return " = %s" % val
  return ''

strtranslators = {'const char*' : null_translator,
                  'const std::string&' : cstr_translator,
                  'unsigned long' : strof_translator,
                  'unsigned int' : strof_translator}

cstrtranslators = {'const char*' : null_translator,
                   'const std::string&' : cstr_translator,
                   'unsigned long' : cstrof_translator,
                   'unsigned int' : cstrof_translator}

deftranslators = {'const char*' : none_translator,
                  'const std::string&' : none_translator,
                  'unsigned long' : eq_translator,
                  'unsigned int' : eq_translator}

class Type:
  def __init__(self, type_name, param_type = None):
    self.type_name = type_name
    self.param_type = param_type or type_name

class Field:
  # named field for admin command
  def __init__(self, name, getter = False, default = None, switch = None, may_empty = False):
    self.key = name
    self.getter = getter
    self.var = name.lower().replace(' ','_').replace('-','_')
    self.enum = self.var.upper().lstrip('_')
    self.var = self.var.replace('virtual', 'wirtual')
    self.types = ['const char*']
    self.default = default
    self.switch = switch
    self.may_empty = may_empty

  def var_value(self):
    return self.var
  
  def process_method(self, strm):
    pass

class Cmd:
  def __init__(self, admin = '', aspect = '', fields = [], modificators = [], skip = 0, options = []):
    """ admin = admin name
    aspect = not valued parameter
    fields = parameters to construct admin command
    modificators = additional aspects to construct with
    skip = how many lines skip from admin output
    options = additional options (aspects) 
    """
    self.aspect = aspect
    self.fields = []
    self.params = []
    for field in fields:
      self.params.append(field)
    if isinstance(admin, tuple):
      self.admin = 'service'
      self.params.append(SomeField('service', admin[1], admin[0]))
    else:
      self.admin = admin
    self.modificators = []
    for modificator in modificators:
      self.modificators.append(Field(modificator))
    self.skip_lines = skip      
    self.base = 'BaseAdminCmd'
    self.options = options
    self.no_expected = False
  
  #modificators is enum for command not valued parameters
  
  def modificator_arg(self, scope = ''):
    if self.modificators:
      return ", %sModificator mod = %s%s" % (scope, scope, self.modificators[0].enum)
    return ''
  
  def modificator_set(self):
    if self.modificators:
      return " modificator_ = mod;"
    return ''

  def process_modificators(self, strm):
    print >>strm, self.modificators[0].enum + " = 0" + \
      (len(self.modificators) > 1 and "," or "")
    if len(self.modificators) > 1:
      for mod in self.modificators[1:-1]:
        print >>strm, mod.enum + ','
      print >>strm, self.modificators[-1].enum

  def modificators_array(self, strm):
    last = len(self.modificators) - 1
    for i in xrange(0, last):
      print >>strm, '"' + self.modificators[i].key + '"' + ','
    print >>strm, '"' + self.modificators[last].key + '"'

  def modificators_enum(self, strm):
    if self.modificators:
      print >>strm, "enum Modificator"
      print >>strm, "{"
      strm.addLevel()
      self.process_modificators(strm)
      strm.delLevel()
      print >>strm, "};"
  
  def modificators_field(self, strm):
    if self.modificators:
      print >>strm, "protected:"
      strm.addLevel()
      print >>strm, "Modificator modificator_;"
      print >>strm, "static const char* modificator_names_[%s];" % (
        len(self.modificators))
      strm.delLevel()

  def modificators_data(self, name, strm):
    if self.modificators:
      print >>strm, "const char*"
      print >>strm, "%s::modificator_names_[%s] = {" % (name, len(self.modificators))
      strm.addLevel()
      self.modificators_array(strm)
      strm.delLevel()
      print >>strm, "};"

  def get_options(self, strm):
    for option in self.options:
      print >>strm, "  add_cmd_i(\"%s\");" % option

  def get_getters(self, getters, var, strm):
    strm.addLevel()
    for item in getters:
      getter = isinstance(item, str) and item or ""
      print >>strm, "params.push_back(AdminParamPair(\"%s\", %s));" % (
        getter, var)
    strm.delLevel()
      
  def make(self, strm):
    print >>strm, "void make_cmd (const char* address%s" % \
       (len(self.params) != 0 and "," or "")
    for i in range(len(self.params)):
      param = self.params[i]
      print >>strm, "  %s %s%s" % \
            (param.type, param.var, i != len(self.params) - 1 and "," or "") 
    print >>strm, ")"
    print >>strm, "{"
    strm.addLevel()
    if not self.no_expected:
      print >>strm, "address_ = address;"
    print >>strm, "AdminParams params;"
    for param in self.params:
      if param.getter:
        getters = isinstance(param.getter, list) and param.getter or [param.getter]
        if param.may_empty:
          print >>strm, "{"
          self.get_getters(getters, "%s ? %s : \"\"" % (param.var, param.var), strm)
          print >>strm, "}"
        else:
          print >>strm, "if (%s)" % param.var
          print >>strm, "{"
          if param.switch:
            strm.addLevel()
            print >>strm, "if (%s)" % param.switch[0]
            print >>strm, "{"
            strm.addLevel()
            print >>strm, "params.push_back(AdminParamPair(\"%s\", %s));" % (
              param.switch[1], param.var)
            strm.delLevel()
            print >>strm, "}"
            print >>strm, "else"
            print >>strm, "{"
            self.get_getters(getters, param.var, strm)
            print >>strm, "}"
            strm.delLevel()
          else:
            self.get_getters(getters, param.var_value(), strm)
          print >>strm, "}"
    strm.delLevel()
    print >>strm, "  make_admin_cmd(*this, \"%s\", address, params, static_cast<size_t>(%s));" % \
          (self.aspect, self.admin)
    if self.modificators:
       print >>strm, "  add_cmd_i(modificator_names_[modificator_]);"
    self.get_options(strm)
    print >>strm, "}"

  def process_params(self, strm, tp, atype, scope = ''):
    """strm = output stream
    tp = list of types of parameters
    atype = type of address parameter
    scope = name scope of construction specific types (Admin class)
    """
    modificator_arg = self.modificator_arg(scope)
    print >>strm, "  %s address%s" % \
          (atype, len(self.params) != 0 and "," or "")
    for i in xrange(0, len(self.params)):
      default = self.params[i].default and " = " + str(self.params[i].default) or ""
      print >>strm, "  %s %s%s%s" % \
            (tp[i], self.params[i].var, default,
             (i != len(self.params) - 1 or modificator_arg) and "," or "")
    if modificator_arg:
      print >>strm, "  %s" % modificator_arg[2:]
    
  def constructors(self, name, strm, redirect = None, scope = ''):
    """name = name of class to construct,
    strm = output stream,
    redirect = variable name to redirect construction,
    scope = name scope of construction specific types (Admin class)
    """
    modificator_arg = self.modificator_arg()
    modificator_set = self.modificator_set()
    atypes = ['const char*', 'const std::string&']
    types = [[]]
    for param in self.params:
      new_types = []
      for t in param.types:
        new_items = copy.deepcopy(types)
        for item in new_items:
          item.append(t)
        new_types = new_types + new_items
      types = new_types
    for tp in types:
      for atype in atypes:
        print >>strm, "%s(" % name
        self.process_params(strm, tp, atype, scope)
        if redirect:
          params = ''
          for i in xrange(0, len(self.params)):
            params = params + ", %s" % self.params[i].var
          if modificator_arg:
            params = params + ", mod";
          print >>strm, "  ): %s(address%s) {}" % (redirect, params)
        else:
          print >>strm, ")"
          if self.skip_lines:
            print >>strm, "  :Base(DEFAULT_SKIP)"
          print >>strm, "{"
          if modificator_set:
            print >>strm, " %s" % modificator_set
          params = ''
          for i in xrange(0, len(self.params)):
            param = cstrtranslators.has_key(tp[i]) and cstrtranslators[tp[i]](self.params[i].var) or self.params[i].var
            params = params + ", %s" % param
          print >>strm, "  make_cmd(%s%s);" % \
                (strtranslators[atype]('address'), params)
          print >>strm, "}"

class Admin:

  def __init__(self, name, cmd, checker_name = None):
    self.name = name
    self.cmd = cmd
    self.fields = []
    self.selffields = []
    self.enumfields = []
    admins.append(self)
    self.checker_name = checker_name

  # sort fields
  def postprocess(self):
    fields = []
    for i in self.fields:
      fields.append(i)
      if i.enum:
        self.enumfields.append(i)
      else:
        self.selffields.append(i)
    self.fields = fields
    if not self.enumfields:
      self.cmd.no_expected = True
  
  def template_name(self):
    return "%s<%s, %s>" % (self.cmd.base, self.name, len(self.enumfields))
  
  def template_name2(self):
    return "%s<%s, FIELDS_COUNT>" % (self.cmd.base, self.name)

  # emum of fields
  def process_enum(self, strm):
    if not self.enumfields:
      return
    last = len(self.enumfields) - 1
    if last > 0:
      print >>strm, self.enumfields[0].enum + " = 0,"
      for i in xrange(1, last):
        print >>strm, self.enumfields[i].enum + ','
      print >>strm, self.enumfields[last].enum
    else:
      print >>strm, self.enumfields[0].enum + " = 0"
  
  def process_name_array(self, strm):
    if not self.enumfields:
      return
    last = len(self.enumfields) - 1
    for i in xrange(0, last):
      print >>strm, '"' + self.enumfields[i].key + '"' + ','
    print >>strm, '"' + self.enumfields[last].key + '"'

  def process_methods(self, strm):
    for field in self.fields:
      field.process_method(strm)

  def process_accessors(self, strm):
    for field in self.enumfields:
      field.process_accessor(strm)

  def process_initialization(self):
    fields = []
    for field in self.selffields:
      if field.default:
        fields.append(field)
    if not fields:
      return ""
    str = ": "
    last = len(fields) - 1
    for i in xrange(0, last):
      str = str + fields[i].var + "_(" + fields[i].default +  "), "
    str = str + fields[last].var + "_(" + fields[last].default +  ")"
    return str

  def process_expected(self, strm):
    print >>strm, "class Expected"
    print >>strm, "{"
    strm.addLevel()
    print >>strm, "friend class %s;" % self.name
    print >>strm, "public:"
    strm.addLevel()
    print >>strm, "Expected()%s {}" % self.process_initialization()
    self.process_methods(strm)
    strm.delLevel()
    #print >>strm, "protected:"
    print >>strm, ""
    strm.addLevel()
    print >>strm, "enum FieldName"
    print >>strm, "{"
    strm.addLevel()
    self.process_enum(strm)
    strm.delLevel()
    print >>strm, "};"
    strm.delLevel()
    print >>strm, "protected:"
    strm.addLevel()
    print >>strm, "FieldIndexMap values_;"
    for field in self.selffields:
      print >>strm, "%s %s_;" % (field.type, field.var)
    strm.delLevel()
    strm.delLevel()
    print >>strm, "};"

  def process_checker_header(self, strm):
    if self.enumfields and self.checker_name:
      print >>strm, "template<typename ExpectedType = %s::Expected>" % self.name
      print >>strm, "class %s_:" % self.checker_name
      print >>strm, "  public AutoTest::Checker"
      print >>strm, "{"
      strm.addLevel()
      print >>strm, "public:"
      strm.newline()
      strm.addLevel()
      print >>strm, "typedef ExpectedType Expected;"
      strm.newline()
      params = self.cmd.params
      print >>strm, "%s_("  % (self.checker_name)
      print >>strm, "  BaseUnit* test,"
      for i in xrange(0, len(params)):
        print >>strm, "  %s %s," % \
          (not isinstance(params[i], StrField) and \
           params[i].type or \
           params[i].base_type().param_type,
           params[i].var)
      print >>strm, "  const Expected& expected,"
      print >>strm, "  AdminExistCheck exists = AEC_EXISTS) :"
      print >>strm, "  test_(test),"
      for i in xrange(0, len(params)):
        print >>strm, "  %s_(%s)," % \
         (params[i].var, params[i].var)
      print >>strm, "  expected_(expected),"
      print >>strm, "  exists_(exists)"
      print >>strm, "  {}"
      strm.newline()
      print >>strm, "virtual ~%s_() throw() {}" % self.checker_name
      strm.newline()
      print >>strm, "bool check(bool throw_error = true) throw (CheckFailed, eh::Exception);"
      strm.newline()
      strm.delLevel()
      print >>strm, "private:"
      strm.addLevel()
      strm.newline()
      print >>strm, "BaseUnit* test_;"
      for i in xrange(0, len(params)):
        print >>strm, "%s %s_;" % \
          (not isinstance(params[i], StrField) and \
           params[i].type or \
           params[i].base_type().type_name,
           params[i].var)
      print >>strm, "Expected expected_;"
      print >>strm, "AdminExistCheck exists_;"
      strm.newline()
      strm.delLevel()
      strm.delLevel()
      print >>strm, "};"
      strm.newline()
      print >>strm, "typedef %s_<%s::Expected> %s;" % \
            (self.checker_name, self.name, self.checker_name)
      print >>strm, "typedef %s_<std::string> %sSimple;" % \
            (self.checker_name, self.checker_name)
      strm.newline()


  def process_declaration(self, strm):
    if self.enumfields:
      print >>strm, "class %s:" % self.name
      print >>strm, "  public %s" % self.template_name()
    else:
      print >>strm, "class %s:" % self.name
      print >>strm, "  public ShellCmd"
    print >>strm, "{"
    if self.cmd.skip_lines:
      print >>strm, "  static const unsigned long DEFAULT_SKIP = %i;" % self.cmd.skip_lines
    if self.enumfields:
      strm.addLevel()
      print >>strm, "public:"
      strm.addLevel()
      self.process_expected(strm)
      strm.delLevel()
      print >>strm, ""      
      print >>strm, "public:"
      print >>strm, "  typedef %s Base;" % self.template_name2()
      print >>strm, ""
    else:
      strm.addLevel()
      print >>strm, "public:"
    strm.addLevel()
    self.cmd.modificators_enum(strm)
    self.cmd.make(strm)
    self.cmd.constructors(self.name, strm)
    self.process_accessors(strm)
    if self.enumfields:      
      print >>strm, "bool check(const Expected& expected, bool exist = true)"
      print >>strm, "  throw(eh::Exception);"    
      print >>strm, "bool check(const std::string& expected, bool exist = true)"
      print >>strm, "  throw(eh::Exception);"
    strm.delLevel()
    self.cmd.modificators_field(strm)
    strm.delLevel()
    print >>strm, "};"
    print >>strm, ""

  def process_implementation(self, strm):
    for field in self.fields:
      field.process_implementation(strm, self.name)

  # fields arrays
  def process_data(self, strm):
    if self.enumfields:
      print >>strm, "template<>"
      print >>strm, "%s::names_type const" % self.template_name()
      print >>strm, "%s::field_names = {" % self.template_name()
      strm.addLevel()
      self.process_name_array(strm)
      strm.delLevel()
      print >>strm, "};"
      print >>strm, "bool"
      print >>strm, "%s::check(const Expected& expected, bool exist)" % self.name
      print >>strm, "  throw(eh::Exception)"
      print >>strm, "{"
      print >>strm, "  return exist? check_(expected.values_):"
      print >>strm, "    !check_(expected.values_);"
      print >>strm, "}"
      print >>strm, "bool"
      print >>strm, "%s::check(const std::string& expected, bool exist)" % self.name
      print >>strm, "  throw(eh::Exception)"
      print >>strm, "{"
      print >>strm, "  return exist? check_(expected):"
      print >>strm, "    !check_(expected);"
      print >>strm, "}"
    self.cmd.modificators_data(self.name, strm)

  def process_checker_data(self, strm):
    params = self.cmd.params
    if self.enumfields and self.checker_name:
      print >>strm, "template<typename ExpectedType>"
      print >>strm, "bool"
      print >>strm, "%s_<ExpectedType>::check(bool throw_error)" % self.checker_name
      print >>strm, "  throw (CheckFailed, eh::Exception)"
      print >>strm, "{"
      strm.addLevel()
      print >>strm, "AdminsArray<%s, CT_ALL> admins;" % self.name;
      strm.newline()
      print >>strm, "admins.initialize("
      print >>strm, "  test_,"
      print >>strm, "  CTE_ALL,"
      print >>strm, "  srv_type_by_index("
      print >>strm, "    static_cast<size_t>(%s%s))%s" % \
        (self.cmd.admin,
         self.cmd.admin == 'service' and "_" or "",
         len(params) and "," or "")
      for i in xrange(0, len(params)):
        print >>strm, "  %s_%s" % \
          (params[i].var,
           i != len(params) - 1 and "," or "")
      print >>strm, ");"
      strm.newline()
      print >>strm, "return admin_checker("
      print >>strm, "  admins,"
      print >>strm, "  expected_,"
      print >>strm, "  exists_).check(throw_error);"
      strm.newline()
      strm.delLevel()
      print >>strm, "}"
                  

class StrField(Field):
  def __init__(self, name, getter = False, default = None, switch = None, may_empty = False):
    Field.__init__(self, name, getter, default, switch, may_empty)
    self.type = 'const char*'
    self.types = ['const char*', 'const std::string&']

  def base_type(self):
    return Type('std::string', 'const std::string&')
  
  def process_method(self, strm):
    print >>strm, "Expected& %s (const std::string& val);" % self.var
    if self.getter:
      print >>strm, "bool has_%s () const;" % self.var
      print >>strm, "std::string %s () const;" % self.var

  def process_accessor(self, strm):
    print >>strm, "const char* %s (unsigned int i = 0);" % self.var

  def process_implementation(self, strm, klass):
    print >>strm, "inline"
    print >>strm, "%s::Expected&" % klass
    print >>strm, "%s::Expected::%s(const std::string& val)" % (
      klass, self.var)
    print >>strm, "{"
    print >>strm, "   values_.insert(std::make_pair(%s, ComparableRegExp(val)));" % self.enum
    print >>strm, "   return *this;"
    print >>strm, "}"
    if self.getter:
      print >>strm, "inline"
      print >>strm, "bool"
      print >>strm, "%s::Expected::has_%s() const" % (klass, self.var)
      print >>strm, "{"
      print >>strm, "  FieldIndexMap::const_iterator it = values_.find(%s);" % self.enum
      print >>strm, "  return it != values_.end();"
      print >>strm, "}"
      print >>strm, "inline"
      print >>strm, "std::string"
      print >>strm, "%s::Expected::%s() const" % (klass, self.var)
      print >>strm, "{"
      print >>strm, "  FieldIndexMap::const_iterator it = values_.find(%s);" % self.enum
      print >>strm, "  return it->second->str();"
      print >>strm, "}"
    print >>strm, "inline"       
    print >>strm, "const char*"
    print >>strm, "%s::%s (unsigned int i)" % (klass, self.var)
    print >>strm, "{"
    print >>strm, "  if (empty() || values_[0].size() != FIELDS_COUNT) return 0;"
    print >>strm, "  return values_[i][Expected::%s].c_str();" % self.enum
    print >>strm, "}"
    
class IntField(StrField):
  def __init__(self, name, getter = False, default = None, switch = None):
    StrField.__init__(self, name, getter, default, switch)
    self.types = ['const char*', 'const std::string&', 'unsigned long']
    self.default = default

  def base_type(self):
    return Type('unsigned long')

  def process_method(self, strm):
    StrField.process_method(self, strm)
    print >>strm, "Expected& %s(unsigned long val);" % self.var
  
  def process_implementation(self, strm, klass):
    StrField.process_implementation(self, strm, klass)
    print >>strm, "inline"
    print >>strm, "%s::Expected&" % klass
    print >>strm, "%s::Expected::%s(unsigned long val)" % (klass, self.var)
    print >>strm, "{"
    print >>strm, "   values_.insert(std::make_pair(%s, ComparableRegExp(strof(val))));" % self.enum
    print >>strm, "   return *this;"
    print >>strm, "}"

class MoneyField(StrField):
  def __init__(self, name, getter = False, default = None, switch = None):
    StrField.__init__(self, name, getter, default, switch)
    self.types = ['const char*', 'const std::string&', 'Money']
    self.default = default

  def base_type(self):
    return Type('Money')

  def process_method(self, strm):
    StrField.process_method(self, strm)
    print >>strm, "Expected& %s (const Money& val);" % self.var
  
  def process_implementation(self, strm, klass):
    StrField.process_implementation(self, strm, klass)
    print >>strm, "inline"
    print >>strm, "%s::Expected&" % klass
    print >>strm, "%s::Expected::%s (const Money& val)" % (klass, self.var)
    print >>strm, "{"
    #print >>strm, "   values_.insert(std::make_pair(%s, \"\\aM\" + strof(val)));" % self.enum
    print >>strm, "   values_.insert(std::make_pair(%s, val));" % self.enum
    print >>strm, "   return *this;"
    print >>strm, "}"

class PrecisionField(StrField):
  def __init__(self, name, getter = False, default = None, switch = None):
    StrField.__init__(self, name, getter, default, switch)
    self.types = ['const char*', 'const std::string&', 'precisely_number']
    self.default = default

  def base_type(self):
    return Type('precisely_number')

  def process_method(self, strm):
    StrField.process_method(self, strm)
    print >>strm, "Expected& %s (const precisely_number& val);" % self.var
  
  def process_implementation(self, strm, klass):
    StrField.process_implementation(self, strm, klass)
    print >>strm, "inline"
    print >>strm, "%s::Expected&" % klass
    print >>strm, "%s::Expected::%s (const precisely_number& val)" % (klass, self.var)
    print >>strm, "{"
    #print >>strm, "   values_.insert(std::make_pair(%s, \"\\aM\" + strof(val)));" % self.enum
    print >>strm, "   values_.insert(std::make_pair(%s, val));" % self.enum
    print >>strm, "   return *this;"
    print >>strm, "}"

class StringListField(StrField):
  def __init__(self, name, getter = False, default = None, switch = None):
    StrField.__init__(self, name, getter, default, switch)
    self.types = ['const char*', 'const std::string&', 'ComparableStringList']
    self.default = default

  def base_type(self):
    return Type('ComparableStringList')

  def process_method(self, strm):
    StrField.process_method(self, strm)
    print >>strm, "Expected& %s (const ComparableStringList& val);" % self.var
  
  def process_implementation(self, strm, klass):
    StrField.process_implementation(self, strm, klass)
    print >>strm, "inline"
    print >>strm, "%s::Expected&" % klass
    print >>strm, "%s::Expected::%s (const ComparableStringList& val)" % (klass, self.var)
    print >>strm, "{"
    #print >>strm, "   values_.insert(std::make_pair(%s, \"\\aM\" + strof(val)));" % self.enum
    print >>strm, "   values_.insert(std::make_pair(%s, val));" % self.enum
    print >>strm, "   return *this;"
    print >>strm, "}"

class UIntField(StrField):
  def __init__(self, name, getter = False, default = None, switch = None):
    StrField.__init__(self, name, getter, default, switch)
    self.types = ['unsigned int']
    self.default = default

  def base_type(self):
    return Type('unsigned int')
    
  def process_method(self, strm):
    StrField.process_method(self, strm)
    print >>strm, "Expected& %s(unsigned int val);" % self.var
  
  def process_implementation(self, strm, klass):
    StrField.process_implementation(self, strm, klass)
    print >>strm, "inline"
    print >>strm, "%s::Expected&" % klass
    print >>strm, "%s::Expected::%s(unsigned int val)" % (klass, self.var)
    print >>strm, "{"
    print >>strm, "   values_.insert(std::make_pair(%s, ComparableRegExp(strof(val))));" % self.enum
    print >>strm, "   return *this;"
    print >>strm, "}"

class BoolField(Field):
  def __init__(self, name, getter = False, default = None):
    Field.__init__(self, name, getter, default, None, False)
    self.type = 'bool'
    self.types = ['bool']
    self.default = default

  def base_type(self):
    return Type('bool')

  def var_value(self):
    return "\"\"";

  def process_method(self, strm):
    StrField.process_method(self, strm)
    print >>strm, "Expected& %s(bool val);" % self.var
  
  def process_implementation(self, strm, klass):
    StrField.process_implementation(self, strm, klass)
    print >>strm, "inline"
    print >>strm, "%s::Expected&" % klass
    print >>strm, "%s::Expected::%s(bool val)" % (klass, self.var)
    print >>strm, "{"
    print >>strm, "   values_.insert(std::make_pair(%s, ComparableString(val ? \"T\" : \"F\"));" % self.enum
    print >>strm, "   return *this;"
    print >>strm, "}"

class SomeField:
  def __init__(self, name, type, default, switch = None):
    self.var = name
    self.enum = None
    self.getter = None    
    self.type = type
    self.types = [type]
    self.default = default
    self.switch = switch

  def base_type(self):
    return Type('std::string', 'const char*')
    
  def process_method(self, strm):
    print >>strm, "Expected& %s(%s val);" % (self.var, self.type)
    print >>strm, "%s %s() const;" % (self.type, self.var)
  
  def process_implementation(self, strm, klass):
    print >>strm, "inline"
    print >>strm, "%s::Expected&" % klass
    print >>strm, "%s::Expected::%s(%s val)" % (klass, self.var, self.type)
    print >>strm, "{"
    print >>strm, "   %s_ = val;" % (self.var)
    print >>strm, "   return *this;"
    print >>strm, "}"
    print >>strm, "inline"
    print >>strm, "%s" % self.type
    print >>strm, "%s::Expected::%s() const" % (klass, self.var)
    print >>strm, "{"
    print >>strm, "  return %s_;" % self.var
    print >>strm, "}"

def string(name, getter = False, default = None, switch = None, may_empty = False):
  return StrField(name, getter, default, switch, may_empty)

def number(name, getter = False, default = None, switch = None):
  return IntField(name, getter, default, switch)

def unumber(name, getter = False, default = None, switch = None):
  return UIntField(name, getter, default, switch)

def flag(name, getter = False, default = None):
  return BoolField(name, getter, default)

def field(name, type, default = None, switch = None):
  return SomeField(name, type, default, switch)

def money(name, getter = False, default = None, switch = None):
  return MoneyField(name, getter, default, switch)

def precision(name, getter = False, default = None, switch = None):
  return PrecisionField(name, getter, default, switch)

def string_list(name, getter = False, default = None, switch = None):
  return StringListField(name, getter, default, switch)

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
  for admin in admins:
    processor(admin, strm)
  strm.delLevel()
  print >>strm, "}"

def process_admin_namespace(strm, processor, admin):
  print >>strm, "namespace AutoTest"
  print >>strm, "{"
  strm.addLevel()
  processor(admin, strm)
  strm.delLevel()
  print >>strm, "}"

def process_header_header(header, name, includes = []):
  print >>header, "/** $" + "Id" + "$"
  print >>header, "* @file %s" % name
  print >>header, "* THIS FILE IS AUTOMATICALY GENERATED, ! DON'T EDIT ! "
  print >>header, "*/"
  print >>header, ""
  guard = name.upper().replace('.','_').replace('/', '_')
  print >>header, "#ifndef __AUTOTESTS_COMMONS_ADMINS_" + guard
  print >>header, "#define __AUTOTESTS_COMMONS_ADMINS_" + guard
  print >>header, ""
  for include in includes:
    inc = include[0] == "<" and include or ("\"%s\"" % include)
    print >>header, "#include %s" % inc
  print >>header, ""

def process_header_footer(header, name, includes = []):
  if includes:
    header.newline()
  for include in includes:
    inc = include[0] == "<" and include or ("\"%s\"" % include)
    print >>header, "#include %s" % inc
  if includes:
    header.newline()
  guard = name.upper().replace('.','_').replace('/', '_')
  print >>header, "#endif  // __AUTOTESTS_COMMONS_ADMINS_" + guard
  print >>header, ""

def process_header(header_):
  header = None
  if type(header_) is StringType:
    header = IndentStream(file(header_, 'w+'))
  else:
    header = header_
  process_header_header(header, header_)
  process_namespace(header, Admin.process_declaration)
  process_namespace(header, Admin.process_implementation)
  process_header_footer(header, header_)
  if header != header_:
    header.close()

def process_header_unit(
  name, process_fns, admin, collector,
  if_not_exists = False, includes = [], tpp_includes = [], path = ""):
  header_name = name + ".hpp"
  header_path = path and (path + "/" + header_name) or header_name
  print >>collector, "#include <tests/AutoTests/Commons/Admins/%s>" % header_path
  if (not (if_not_exists and os.path.exists(header_path))):
    header = IndentStream(file(header_path, 'w+'))
    process_header_header(header, header_name, includes)
    for fn in process_fns:
      process_admin_namespace(header, fn, admin)
    process_header_footer(header, header_name, tpp_includes)
    header.close()

def process_headers():
  collector_name = "GeneratedAdmins.hpp"
  collector = IndentStream(file(collector_name, 'w+'))
  checkers_collector_name = "GeneratedAdminsCheckers.hpp"
  checkers_collector = IndentStream(file(
    checkers_dir_path + "/" + checkers_collector_name, 'w+'))
  process_header_header(collector, collector_name)
  process_header_header(checkers_collector, checkers_collector_name)
  for admin in admins:
    process_header_unit(
      admin.name,
      [Admin.process_declaration, Admin.process_implementation],
      admin, collector,
      includes = ["Admins.hpp"])
    if admin.enumfields and admin.checker_name:
      process_header_unit(
        admin.checker_name,
        [Admin.process_checker_header],
        admin, checkers_collector, True,
        includes = ["<tests/AutoTests/Commons/Admins/%s.hpp>" % admin.name,
                    "<tests/AutoTests/Commons/Checkers/Checker.hpp>",
                    "<tests/AutoTests/Commons/Checkers/AdminsChecker.hpp>"],
        tpp_includes = ["%s.tpp" % admin.checker_name],
        path = checkers_dir_path)      
  print >>collector, ""
  print >>checkers_collector, ""
  process_header_footer(collector, collector_name)
  process_header_footer(checkers_collector, collector_name)
  collector.close()
  checkers_collector.close()
    
def process_source_header(source, name, includes = []):
  print >>source, "/** $" + "Id" + "$"
  print >>source, "* @file %s" % name
  print >>source, "* THIS FILE IS AUTOMATICALY GENERATED, ! DON'T EDIT ! "
  print >>source, "*/"
  print >>source, ""

  for include in includes:
    inc = include[0] == "<" and include or ("\"%s\"" % include)
    print >>source, "#include %s" % inc
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
  process_namespace(source, Admin.process_data)
  if source != source_:
    source.close()

def process_source_unit(
  name, process_fn, admin,
  if_not_exists = False,
  includes = [], ext = "cpp", path = "") :
  header = name + ".hpp"
  source_name = name + ".%s" % ext
  source_path = path and (path + "/" + source_name) or source_name
  source_name_alt = name + ".cpp"
  source_path_alt = path and (path + "/" + source_name_alt) or source_name_alt
  if (not (if_not_exists and (os.path.exists(source_path) \
           or os.path.exists(source_path_alt)))):
    source = IndentStream(file(source_path, 'w+'))
    inc = ext != "tpp" and (includes + [header]) or (includes)
    process_source_header(source, source_name,inc)
    process_admin_namespace(source, process_fn, admin)
    source.close()

def process_sources():
  for admin in admins:
    process_source_unit(admin.name, Admin.process_data, admin)
    if admin.enumfields and admin.checker_name:
      process_source_unit(
        admin.checker_name,#MARK
        Admin.process_checker_data,
        admin, True, ["<tests/AutoTests/Commons/Admins/AdminsContainer.hpp>"],
        ext="tpp", path = checkers_dir_path)

def process_admins(header_ = None, source_ = None):
  for admin in admins:
    admin.postprocess()
  if header_:
    process_header(header_)
  else:
    process_headers()
  if source_:
    process_source(source_)
  else:
    process_sources()



