include(FindPackageHandleStandardArgs)

find_path(EVENT_INCLUDE_DIR
  NAMES event.h
)

find_library(EVENT_LIBRARY
  NAMES event
)

find_package_handle_standard_args(Event
  REQUIRED_VARS
    EVENT_INCLUDE_DIR
    EVENT_LIBRARY
)

set(EVENT_FOUND ${Event_FOUND})

if (Event_FOUND)
  set(EVENT_INCLUDE_DIRS ${EVENT_INCLUDE_DIR})
  set(EVENT_LIBRARIES ${EVENT_LIBRARY})

  if (NOT TARGET Event::Event)
    add_library(Event::Event UNKNOWN IMPORTED)
    set_target_properties(Event::Event PROPERTIES
      IMPORTED_LOCATION "${EVENT_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${EVENT_INCLUDE_DIR}"
    )
  endif()
endif()

mark_as_advanced(
  EVENT_INCLUDE_DIR
  EVENT_LIBRARY
)
