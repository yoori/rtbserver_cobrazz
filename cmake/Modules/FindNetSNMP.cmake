include(FindPackageHandleStandardArgs)

find_path(NETSNMP_INCLUDE_DIR
  NAMES net-snmp/net-snmp-config.h
)

find_library(NETSNMP_LIBRARY
  NAMES netsnmp
)

find_library(NETSNMP_AGENT_LIBRARY
  NAMES netsnmpagent
)

find_package_handle_standard_args(NetSNMP
  REQUIRED_VARS
    NETSNMP_INCLUDE_DIR
    NETSNMP_LIBRARY
    NETSNMP_AGENT_LIBRARY
)

set(NETSNMP_FOUND ${NetSNMP_FOUND})

if(NetSNMP_FOUND)
  set(NETSNMP_LIBRARIES
    ${NETSNMP_AGENT_LIBRARY}
    ${NETSNMP_LIBRARY}
  )

  if(NOT TARGET NetSNMP::NetSNMP)
    add_library(NetSNMP::NetSNMP UNKNOWN IMPORTED)
    set_target_properties(NetSNMP::NetSNMP PROPERTIES
      IMPORTED_LOCATION "${NETSNMP_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${NETSNMP_INCLUDE_DIR}"
    )
  endif()

  if(NOT TARGET NetSNMP::Agent)
    add_library(NetSNMP::Agent UNKNOWN IMPORTED)
    set_target_properties(NetSNMP::Agent PROPERTIES
      IMPORTED_LOCATION "${NETSNMP_AGENT_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${NETSNMP_INCLUDE_DIR}"
      INTERFACE_LINK_LIBRARIES NetSNMP::NetSNMP
    )
  endif()
endif()

mark_as_advanced(
  NETSNMP_INCLUDE_DIR
  NETSNMP_LIBRARY
  NETSNMP_AGENT_LIBRARY
)
