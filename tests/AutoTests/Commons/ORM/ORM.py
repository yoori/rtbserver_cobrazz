#!/usr/bin/env python

import ORAORMData, PQORMData
from ORMBase import IndentStream
import sys
import os.path

sys.stdout = IndentStream(sys.stdout, 2)

def create_objects(filename, objects, namespace):
  ormHeader = "%s.hpp" % filename
  ormSource = "%s.cpp" % filename
  connection_type = namespace == 'PQ' and \
                    'postgres_connection' or 'oracle_connection'
  
  def_suffix = ormHeader.upper().replace('.','_')
  
  header = file(ormHeader, 'w+')
  print '  generate header code'
  
  print >>header, """/** $Id$
 * @file %s
 * ORM C++ objects
 * THIS FILE IS AUTOMATICALY GENERATED, ! DON'T EDIT ! 
 */

#ifndef _AUTOTEST_COMMONS_ORM_%s
#define _AUTOTEST_COMMONS_ORM_%s
 
#include \"ORM.hpp\"
 
namespace AutoTest
{
  namespace ORM
  {
    namespace %s
    {
      namespace DB = AutoTest::DBC;
""" % (ormHeader, def_suffix, def_suffix, namespace)

  stream = IndentStream(header, 3)

  print '  map objects and print classes pre declarations'
  dbMap = {}
  for object in objects:
    object.printSelf(stream)
    dbMap[object.dbName] = object

  stream.newline()

  print '  link fields to objects and print classes declarations'
  for object in objects:
    for field in object.fields:
      if field.link:
        if dbMap.has_key(field.link):
          link = dbMap[field.link]
          if link.id.indexesCount() > 0 and link.id.fields[0].type == field.type:
            field.link = link.name
          else:
            print 'ERROR! ', object.name + '.' + field.name + ' referenced '+ field.link, 'with index type mismatch'
            sys.exit(0)
        else:
          print 'ERROR!', object.name + '.' + field.name+  ' referenced unknown', field.link
          sys.exit(0)
          
    sys.stdout.addLevel()
    object.printDeclaration(stream, connection_type)    
    sys.stdout.delLevel()


  print '  print links'
  for object in objects:
    object.printLinks(stream)

  print >>header, """    }
  }
}

#endif // _AUTOTEST_COMMONS_ORM_%s""" % def_suffix

  source = file(ormSource, 'w+')
  print '  generate source code'
  print >>source, """

#include <sstream>

#include <tests/AutoTests/Commons/Common.hpp>
#include "Utils.hpp"
 
namespace AutoTest
{
  namespace ORM
  {
    namespace %s
    {
      namespace DB = AutoTest::DBC;
""" % (ormSource, namespace)

  stream = IndentStream(source, 3)

  print '  print objects implementations'
  for object in objects:
    sys.stdout.addLevel()
    object.printImplementation(stream, connection_type)
    sys.stdout.delLevel()

  print >>source, """    }
  }
}"""

  header.close()
  source.close()
  print 'done'

print "Postgres"
create_objects("PQORMObjects", PQORMData.objects, 'PQ')
