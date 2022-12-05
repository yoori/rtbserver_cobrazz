#!/usr/bin/env python

import sys
import os.path
import re

import cx_Oracle
import ORMBase

NOT_KEY_FIELDS = ['version']
REJECTED_TABLES_REGEXP = [
    re.compile("REPORTLOG"),
    re.compile("ERRLOG"),
    re.compile(".*\$.*"),
    re.compile("XXPHM_.*"),
    re.compile("MV_.*")]

def mapDbType (type, length, scale):
    if type == 'DATE':
        return ORMBase.ORMDate
    elif type[0:9] == 'TIMESTAMP':
        return ORMBase.ORMTimestamp
    elif type == 'NUMBER':
        if scale != None and int(str(scale)) > 0:
            return ORMBase.ORMFloat
        else:
            return ORMBase.ORMInt
    elif type == 'BLOB':
        return ORMBase.ORMBlob
    elif type == 'CLOB':
        return ORMBase.ORMClob
    return ORMBase.ORMString

def makeIndex (table, sequence, fields, pk):
    if len(pk) == 1:
        pk = pk[0][0]
        index = fields[pk](pk)
        del fields[pk]
        return ORMBase.Index([index], sequence)
    else:
        if sequence:
            #sort indexes 
            indexes = []
            for p in pk:
                indexes.append(p[0])
            def sorter(x, y):
                if x.find(table) +1:
                    return -1
                if y.find(table) +1:
                    return 1
                return cmp(x, y)
            indexes.sort(sorter)
            print 'WARNING!, found '+table+' with sequence '+sequence+' but with '+str(len(pk))+' primary keys {', pk, '} use '+ indexes[0] 
            ret = ORMBase.Index([fields[indexes[0]](indexes[0])], sequence)
            del fields[indexes[0]]
            return ret
        else:
            #sort indexes
            indexes = []
            for p in pk:
                indexes.append(fields[p[0]](p[0]))
                del fields[p[0]]
            def sorter(x, y):
                if x.dbName.find(table) +1:
                    return -1
                if y.dbName.find(table) +1:
                    return 1
                return cmp(x.dbName, y.dbName)
            indexes.sort(sorter)
            if indexes[0].dbName.find(table)+1:
                return ORMBase.Index(indexes, sequence)
            return ORMBase.Index(indexes)

class Schemer:
    def __init__(self, connectionString):
        self.conn = cx_Oracle.connect(connectionString)

    def close (self):
        self.conn.close()
    
    def getRecords (self, sql):
        cursor = self.conn.cursor()
        cursor.execute(sql)
        ret = cursor.fetchall()
        cursor.close()
        return ret

    def getTables (self):
        tblRecords = self.getRecords("select * from USER_TABLES order by TABLE_NAME")
        ret = []
        for tblRecord in tblRecords:
            ret.append(tblRecord[0])
        return ret
    
    def getSequence (self, tableName):
        sqlString = "select SEQUENCE_NAME from USER_SEQUENCES where SEQUENCE_NAME = '%sSEQ' or SEQUENCE_NAME = '%s_SEQ'"
        seqRecords = self.getRecords(sqlString%(tableName, tableName))
        if len(seqRecords) == 1:
            return seqRecords[0][0]
        tbln = tableName[0:len(tableName)-1]
        seqRecords = self.getRecords(sqlString%(tbln, tbln))
        if len(seqRecords) == 1:
            return seqRecords[0][0]
        return None

    def getPrimaryKey(self, tableName):
        sqlString = """
        select USER_CONS_COLUMNS.COLUMN_NAME from USER_CONSTRAINTS
        inner join USER_CONS_COLUMNS on USER_CONS_COLUMNS.CONSTRAINT_NAME = USER_CONSTRAINTS.CONSTRAINT_NAME
        where USER_CONSTRAINTS.CONSTRAINT_TYPE = 'P' and USER_CONSTRAINTS.TABLE_NAME = '%s'
        """
        pkRecords = self.getRecords(sqlString%(tableName))
        if len(pkRecords) == 0:
            def flt( field ):
                fieldName, fieldType = field
                return fieldName.lower() not in NOT_KEY_FIELDS and \
                       fieldType(fieldName).type != 'ORMBlob'
            def fld2tuple( field ):
                fieldName, _ = field
                return ( fieldName, )
            return True, map(fld2tuple, filter(flt, self.getFields(tableName).items()))
        return False, pkRecords
    
    def getReferences (self, tableName):
        sqlString = """select USER_CONS_COLUMNS.COLUMN_NAME, c2.TABLE_NAME from USER_CONS_COLUMNS
        inner join USER_CONSTRAINTS c1
        on USER_CONS_COLUMNS.CONSTRAINT_NAME = c1.CONSTRAINT_NAME
        inner join USER_CONSTRAINTS c2
        on  c2.CONSTRAINT_NAME = c1.R_CONSTRAINT_NAME
        where c1.CONSTRAINT_TYPE = 'R' and USER_CONS_COLUMNS.TABLE_NAME = '%s' """
        linksRecords = self.getRecords(sqlString%(tableName))
        fields = {}
        for field in linksRecords:
            fields[field[0]] = field[1]
        return fields
                                    
    def getFields(self, tableName):
        sqlString = """select COLUMN_NAME, DATA_TYPE, DATA_LENGTH, DATA_SCALE
        from USER_TAB_COLUMNS where TABLE_NAME = '%s' """
        fieldRecords = self.getRecords(sqlString%(tableName))
        fields = {}
        for field in fieldRecords:
            fields[field[0]] =  mapDbType(field[1], field[2], field[3])
        return fields

    def checkTable (self, table):
        for regexp in REJECTED_TABLES_REGEXP:
            if regexp.search(table): return False
        return True

    def getScheme (self):
        tables = self.getTables()
        objects = {}
        count = 0
        for table in tables:
            if self.checkTable(table):
                sequence = self.getSequence(table)
                construct, pk = self.getPrimaryKey(table)
                fields = self.getFields(table)
                object = None
                if construct:
                    object = ORMBase.DualObject(table)
                else:
                    object = ORMBase.Object(table)
                object.id = makeIndex (table, sequence, fields, pk)
                items = fields.items()
                items.sort(lambda x,y: cmp(x[0], y[0]))
                for fieldName, fieldType in items:
                    field = fieldType(fieldName)
                    links  = self.getReferences(table)
                    if links.has_key(field.dbName):
                        field.link = links[field.dbName]
                    object.fields.append(field)
                objects[table] = object
                count = count + 1
                print "Proccess table ", table, "...done"
            else:
                print "! REJECT table ", table
        print "processed", count, 'tables'
        return objects

def dumpSchemeMap (connString, objects, unimportant, toFile):
    try:
        schemer = Schemer(connString)
        #get scheme
        scheme = schemer.getScheme()
        #merge scheme
        for object in objects:
            if scheme.has_key(object.dbName):
                scheme[object.dbName].merge(object)
        for tbl in unimportant:
            if scheme.has_key(tbl.upper()):
              del scheme[tbl.upper()]
        keys = scheme.keys()
        keys.sort(cmp=lambda x,y: cmp(x.lower(), y.lower()), key=str.lower)
        #print new scheme
        out = file(toFile, 'w+')
        print >>out, 'from ORMBase import *\n'
        for key in keys:
            print >>out, scheme[key].getMakeStr()
        print >>out, "objects = ["
        for key in keys:
            print >>out, "    %s," % scheme[key].uniqueName()
        print >>out, "]\n"
        print >>out, "unimportant = ["
        for tbl in unimportant:
            print >>out, "    '%s'," % tbl.upper()
        print >>out, "]"
        out.close()
    except cx_Oracle.DatabaseError,info:
        print "SQL Error:",info

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print 'Usage: ', sys.argv[0], 'bs_unittest/adserver@oracle.ocslab.com/addbdev.ocslab.com'
        sys.exit(0)
    from ORAORMData import objects, unimportant
    dumpSchemeMap(sys.argv[1],
                  objects,
                  unimportant,
                  'ORAORMData.py')

