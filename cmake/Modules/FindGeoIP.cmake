include(FindPackageHandleStandardArgs)

set(GEOIP_ROOT "" CACHE PATH "GeoIP installation root")

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
  pkg_search_module(PC_GEOIP QUIET geoip)
endif()

find_path(GEOIP_INCLUDE_DIR
  NAMES GeoIP.h
  HINTS
    ${GEOIP_ROOT}
    ENV GEOIP_ROOT
    ${PC_GEOIP_INCLUDEDIR}
  PATH_SUFFIXES include
)

find_library(GEOIP_LIBRARY
  NAMES GeoIP libGeoIP-1
  HINTS
    ${GEOIP_ROOT}
    ENV GEOIP_ROOT
    ${PC_GEOIP_LIBDIR}
  PATH_SUFFIXES lib lib64
)

find_package_handle_standard_args(GeoIP
  REQUIRED_VARS
    GEOIP_LIBRARY
    GEOIP_INCLUDE_DIR
)

set(GEOIP_FOUND ${GeoIP_FOUND})

if(GeoIP_FOUND)
  set(GEOIP_INCLUDE_DIRS ${GEOIP_INCLUDE_DIR})
  set(GEOIP_LIBRARIES ${GEOIP_LIBRARY})

  if(NOT TARGET GeoIP::GeoIP)
    add_library(GeoIP::GeoIP UNKNOWN IMPORTED)
    set_target_properties(GeoIP::GeoIP PROPERTIES
      IMPORTED_LOCATION "${GEOIP_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${GEOIP_INCLUDE_DIR}"
    )
  endif()
endif()

mark_as_advanced(
  GEOIP_INCLUDE_DIR
  GEOIP_LIBRARY
)
