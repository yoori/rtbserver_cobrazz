include(FindPackageHandleStandardArgs)

find_library(USERVER_CORE_LIBRARY
  NAMES userver-core
)

find_package_handle_standard_args(USERVER
  REQUIRED_VARS USERVER_CORE_LIBRARY
)

if (USERVER_FOUND AND NOT TARGET USERVER::Core)
  add_library(USERVER::Core UNKNOWN IMPORTED)
  set_target_properties(USERVER::Core PROPERTIES
    IMPORTED_LOCATION "${USERVER_CORE_LIBRARY}"
  )
endif()

mark_as_advanced(USERVER_CORE_LIBRARY)
