include(FindPackageHandleStandardArgs)

find_path(Nghttp2_INCLUDE_DIR
  NAMES nghttp2/nghttp2.h
)

find_library(Nghttp2_LIBRARY
  NAMES nghttp2
)

find_package_handle_standard_args(Nghttp2
  REQUIRED_VARS
    Nghttp2_LIBRARY
    Nghttp2_INCLUDE_DIR
)

if (Nghttp2_FOUND AND NOT TARGET Nghttp2::Nghttp2)
  add_library(Nghttp2::Nghttp2 UNKNOWN IMPORTED)
  set_target_properties(Nghttp2::Nghttp2 PROPERTIES
    IMPORTED_LOCATION "${Nghttp2_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${Nghttp2_INCLUDE_DIR}"
  )
endif()

mark_as_advanced(
  Nghttp2_INCLUDE_DIR
  Nghttp2_LIBRARY
)
