# - Find Net-SNMP
#
# -*- cmake -*-
#
# Find the Net-SNMP module
#
#  NETSNMP_INCLUDE_DIR - where to find Net-SNMP.h, etc.
#  NETSNMP_LIBRARIES   - List of libraries when using Net-SNMP.
#  NETSNMP_FOUND       - True if Net-SNMP found.

#IF (NETSNMP_INCLUDE_DIR)
  # Already in cache, be silent
#  SET(NETSNMP_FIND_QUIETLY TRUE)
#ENDIF (NETSNMP_INCLUDE_DIR)

#FIND_PATH(NETSNMP_INCLUDE_DIR snmp.h
#  /usr/include/net-snmp/library
#)

#SET(NETSNMP_NAMES netsnmp)
FIND_LIBRARY(USERVER_CORE
  NAMES userver-core
  PATHS /usr/lib /usr/local/lib /usr/lib64
)


MARK_AS_ADVANCED(
    USERVER_CORE
  )
