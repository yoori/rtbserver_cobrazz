include(FindPackageHandleStandardArgs)

set(VANGA_ROOT "/opt/foros/vanga" CACHE PATH "Vanga installation root")

find_path(VANGA_INCLUDE_DIR
  NAMES DTree/DTree.hpp
  HINTS
    ${VANGA_ROOT}
    ENV VANGA_ROOT
  PATH_SUFFIXES include
)

find_library(VANGA_DTREE_LIBRARY
  NAMES VangaDTree DTree
  HINTS
    ${VANGA_ROOT}
    ENV VANGA_ROOT
  PATH_SUFFIXES lib lib64
)

find_library(VANGA_GEARS_BASIC_LIBRARY
  NAMES GearsBasic
  HINTS
    ${VANGA_ROOT}
    ENV VANGA_ROOT
  PATH_SUFFIXES lib lib64
)

find_library(VANGA_GEARS_STRING_LIBRARY
  NAMES GearsString
  HINTS
    ${VANGA_ROOT}
    ENV VANGA_ROOT
  PATH_SUFFIXES lib lib64
)

find_library(VANGA_GEARS_THREADING_LIBRARY
  NAMES GearsThreading
  HINTS
    ${VANGA_ROOT}
    ENV VANGA_ROOT
  PATH_SUFFIXES lib lib64
)

find_package_handle_standard_args(Vanga
  REQUIRED_VARS
    VANGA_INCLUDE_DIR
    VANGA_DTREE_LIBRARY
    VANGA_GEARS_BASIC_LIBRARY
    VANGA_GEARS_STRING_LIBRARY
    VANGA_GEARS_THREADING_LIBRARY
)

if(Vanga_FOUND)
  if(NOT TARGET Vanga::Headers)
    add_library(Vanga::Headers INTERFACE IMPORTED)
    set_target_properties(Vanga::Headers PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${VANGA_INCLUDE_DIR}"
    )
  endif()

  function(_vanga_import_library target library)
    if(NOT TARGET Vanga::${target})
      add_library(Vanga::${target} UNKNOWN IMPORTED)
      set_target_properties(Vanga::${target} PROPERTIES
        IMPORTED_LOCATION "${library}"
        INTERFACE_LINK_LIBRARIES Vanga::Headers
      )
    endif()
  endfunction()

  _vanga_import_library(GearsBasic "${VANGA_GEARS_BASIC_LIBRARY}")
  _vanga_import_library(GearsString "${VANGA_GEARS_STRING_LIBRARY}")
  _vanga_import_library(GearsThreading "${VANGA_GEARS_THREADING_LIBRARY}")
  _vanga_import_library(DTree "${VANGA_DTREE_LIBRARY}")

  set_target_properties(Vanga::GearsThreading PROPERTIES
    INTERFACE_LINK_LIBRARIES "Vanga::Headers;Vanga::GearsBasic"
  )
  set_target_properties(Vanga::DTree PROPERTIES
    INTERFACE_LINK_LIBRARIES
      "Vanga::Headers;Vanga::GearsBasic;Vanga::GearsString;Vanga::GearsThreading"
  )
endif()

mark_as_advanced(
  VANGA_INCLUDE_DIR
  VANGA_DTREE_LIBRARY
  VANGA_GEARS_BASIC_LIBRARY
  VANGA_GEARS_STRING_LIBRARY
  VANGA_GEARS_THREADING_LIBRARY
)
