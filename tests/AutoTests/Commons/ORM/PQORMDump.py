#!/usr/bin/env python

import re, psycopg2, ORMBase, os, shutil
from optparse import OptionParser

DEFAULT_HOST="stat-dev1"
DEFAULT_USER="test_ads"
DEFAULT_PSWD="adserver"
DEFAULT_PORT=5432

SCHEMAS_SQL=['public', 'stat']
DATA_FILE='PQORMData.py'

def check_defined(parser, options):
  for opt, value in options.__dict__.items():
    if not value:
      parser.error("Required argument '%s' is undefined" % opt)

class PQTable:

  def __init__(self, name, fields = [], pkey = [], sequence = None, refs = []):
    self.name = name
    self.fields = fields
    self.pkey = pkey
    self.sequence = sequence
    self.refs = refs

  def __index(self):
    return ORMBase.PQIndex(map(lambda x: x.orm(), self.pkey), self.sequence)

  def table(self):
    if len(self.pkey):
      object = ORMBase.Object(self.name)
    else:
      object = ORMBase.DualObject(self.name)
    object.id = self.__index()
    self.fields.sort(lambda x,y: cmp(x.name, y.name))
    for fld in self.fields:
      field = fld.orm()
      # Process reference
      object.fields.append(field)
    return object
  
class PQField:

  def __init__(self, table, name, type, link = None):
    self.table = table
    self.name = name
    self.type = type
    self.link = link

  def orm(self):
    if self.type == 'date':
      return ORMBase.ORMDate(self.name, link = self.link)
    elif self.type[0:9] == 'timestamp':
      return ORMBase.ORMTimestamp(self.name, link = self.link)
    elif self.type in ('integer', 'bigint', 'smallint'):
      return ORMBase.ORMInt(self.name, link = self.link)
    elif self.type in ('boolean'):
      return ORMBase.ORMBool(self.name, link = self.link)
    elif self.type[0:7] == 'numeric':
      return ORMBase.ORMFloat(self.name, link = self.link)
    elif self.type[0:9] == 'character':
      return ORMBase.ORMString(self.name, link = self.link)
    elif self.type in ('bit', 'text', 'oid', 'name', 'inet'):
      return ORMBase.ORMString(self.name, link = self.link)
    raise Exception('"%s.%s", unsupported type "%s"' % \
                    (self.table, self.name, self.type))

class PQSchema:

  def __init__(self, options, unimportant):
    self.conn = psycopg2.connect(
      host=options.host,
      database=options.database,
      user=options.user,
      password=options.password,
      port=options.port)
    self.unimportant = unimportant
    self.tables = []
    self.__init_tables()

  def __del__(self):
    self.conn.close()

  def __select(self, sql, vars = None):
    cursor = self.conn.cursor()
    cursor.execute(sql, vars)
    result = cursor.fetchall()
    cursor.close()
    return result

  def __get_reference(self, table):
    return map(lambda x: PQField(x[0], x[1][0], x[2][0]),
               filter(lambda x: len(x) == 3 and len(x[1]) == 1 and len(x[2]) == 1,
                      self.__select("SELECT confrelid::regclass AS fk_table_name, "
                                    "ARRAY(SELECT attname "
                                    "FROM pg_attribute "
                                    "WHERE attrelid = confrelid AND "
                                    "attnum IN ( SELECT unnest(confkey) ) ) AS fk_cols, "
                                    "ARRAY(SELECT format_type(atttypid, atttypmod) "
                                    "FROM pg_attribute "
                                    "WHERE attrelid = confrelid AND "
                                    "attnum IN ( SELECT unnest(confkey) ) ) AS fk_types "
                                    "FROM pg_constraint cnst "
                                    "WHERE cnst.conrelid = %s::regclass and condeferrable = true",
                                    (table, ))))
      
  def __get_columns(self, table, pkey, refs):
    keys = map(lambda x: x.name, pkey)
    def ref_check(r):
      (_, table) = r
      return len(filter(lambda x: x.upper() == table.upper(),
                        self.unimportant)) == 0
    references = dict(
      filter(ref_check, map(lambda x: (x.name, x.table), refs)))
    
    return map(lambda x: PQField(table, x[0], x[1], references.get(x[0])),
               filter(lambda x: x[2] in SCHEMAS_SQL and x[0] not in keys,
                      self.__select(
                        "select column_name, data_type, table_schema "
                        "from information_schema.columns "
                        "where table_name = %s", (table,))))

  def __get_pk_key(self, table):
    return map(lambda x: PQField(table, x[0], x[1]),
               self.__select(
                 "select pg_attribute.attname, "
                 "format_type(pg_attribute.atttypid, pg_attribute.atttypmod) "
                 "from pg_index, pg_class, pg_attribute "
                 "where pg_class.oid = %s::regclass AND "
                 "indrelid = pg_class.oid AND "
                 "pg_attribute.attrelid = pg_class.oid AND "
                 "pg_attribute.attnum = any(pg_index.indkey) AND indisprimary",
                 (table, )))

  def __get_sequence(self, table):
    seq = self.__select(
      "SELECT s.relname as sequence_name "
      "FROM pg_class s "
      "JOIN pg_depend d ON d.objid = s.oid "
      "JOIN pg_class t ON d.objid = s.oid AND d.refobjid = t.oid "
      "JOIN pg_attribute a ON (d.refobjid, d.refobjsubid) = (a.attrelid, a.attnum) "
      "JOIN pg_namespace n ON n.oid = s.relnamespace "
      "WHERE s.relkind     = 'S' and t.relname = %s"   ,
      (table, ))
    return seq and seq[0]

  def __init_tables(self):
    def create_table(t):
      (table, _) = t
      pkey = self.__get_pk_key(table)
      sequence = self.__get_sequence(table)
      refs = self.__get_reference(table)
      return PQTable(table,
                     fields = self.__get_columns(table, pkey, refs),
                     pkey = pkey,
                     sequence = sequence,
                     refs = refs)
    def check(t):
      (table, schema) = t
      return re.compile(".*_sdate_.*").match(table) is None and \
              re.compile(".*_[0-9]+_[0-9]+_[0-9]+$").match(table) is None and \
             schema in SCHEMAS_SQL and \
             len(filter(lambda x: x.upper() == table.upper(),
                        self.unimportant)) == 0
    self.tables = \
      map(create_table,
          filter(check,
                 self.__select(
                   "select table_name, table_schema from "
                   "information_schema.tables where "
                   "table_type = 'BASE TABLE'")))

def process_schema( schema, objects, unimportant, toFile ):
  out = file(toFile, 'w+')
  print >>out, 'from ORMBase import *\n'

  sorted_tables = sorted(schema.tables, key=lambda x: x.name.upper())
  skipped_tables = []

  try:
    for t in sorted_tables:
      try:
        table = t.table()
      except Exception, x:
        print "Skip table %s: %s" %(t.name, x)
        skipped_tables.append(t)
        continue
      objs = filter(lambda x: x.dbName.upper() == t.name.upper(), objects)
      if objs:
        table.merge(objs[0])
      else:
        table.used = False
      print >>out, table.getMakeStr()
    sorted_tables = filter(lambda x: x not in skipped_tables, sorted_tables)
    print >>out, "objects = ["
    for t in sorted_tables:
      print >>out, "    %s," % t.name
    print >>out, "]\n"
  except Exception, exc:
    print "  Error:", exc
    shutil.move(DATA_FILE + ".tmp", DATA_FILE)
    exit(1)

  print >>out, "unimportant = ["
  for table in unimportant:
    print >>out, "    '%s'," % table.upper()
  print >>out, "]"
  
  out.close()

if __name__ == '__main__':
  parser = OptionParser(add_help_option=False)
  parser.add_option(
    "--help", action="help", help="show this help message and exit")
  parser.add_option(
    "-h", "--host", dest="host", help="Postgres host", default=DEFAULT_HOST)
  parser.add_option(
    "-d", "--database", dest="database", help="Postgres database")
  parser.add_option(
    "-u", "--user", dest="user", help="Postgres user", default=DEFAULT_USER)
  parser.add_option(
    "-p", "--password", dest="password", help="Postgres password", default=DEFAULT_PSWD)
  parser.add_option(
    "--port", dest="port", help="Postgres port", type="int", default=DEFAULT_PORT)
  
  (options, args) = parser.parse_args()

  check_defined(parser, options)

  from PQORMData import objects, unimportant

  schema = PQSchema(options, unimportant)

  shutil.copyfile(DATA_FILE, DATA_FILE + ".tmp")
  process_schema(schema, objects, unimportant, DATA_FILE)
  os.remove(DATA_FILE  + ".tmp")
  
