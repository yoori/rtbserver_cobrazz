################################################################################
#
# CMake script for finding ACE.
# If the optional ACE_ROOT_DIR environment variable exists, header files and
# libraries will be searched in the ACE_ROOT_DIR/include and ACE_ROOT_DIR/libs
# directories, respectively. Otherwise the default CMake search process will be
# used.
#
# This script creates the following variables:
#  ACE_FOUND: Boolean that indicates if the package was found
#  ACE_INCLUDE_DIRS: Paths to the necessary header files
#  ACE_LIBRARIES: Package libraries
#  ACE_LIBRARY_DIRS: Path to package libraries
#
################################################################################

include(FindPackageHandleStandardArgs)

# See if ACE_ROOT is not already set in CMake
IF (NOT ACE_ROOT)
  # See if ACE_ROOT is set in process environment
  IF ( NOT $ENV{ACE_ROOT} STREQUAL "" )
    SET (ACE_ROOT "$ENV{ACE_ROOT}")
    MESSAGE(STATUS "Detected ACE_ROOT set to '${ACE_ROOT}'")
  ENDIF ()
ENDIF ()

# If ACE_ROOT is available, set up our hints
IF (ACE_ROOT)
  SET (ACE_INCLUDE_HINTS HINTS "${ACE_ROOT}/include" "${ACE_ROOT}")
  SET (ACE_LIBRARY_HINTS HINTS "${ACE_ROOT}/lib")
ENDIF ()

# Find headers and libraries
find_path(ACE_INCLUDE_DIR NAMES ace/ACE.h ${ACE_INCLUDE_HINTS})
find_library(ACE_LIBRARY_RELEASE NAMES ACE ${ACE_LIBRARY_HINTS})
find_library(ACE_LIBRARY_DEBUG NAMES ACE${CMAKE_DEBUG_POSTFIX} ${ACE_LIBRARY_HINTS})
find_library(ACE_SSL_LIBRARY NAMES ACE_SSL ${ACE_LIBRARY_HINTS})

set(ACE_LIBRARY "")
if(ACE_LIBRARY_DEBUG AND NOT ACE_LIBRARY_DEBUG STREQUAL ACE_LIBRARY_RELEASE)
  list(APPEND ACE_LIBRARY debug ${ACE_LIBRARY_DEBUG})
endif()
if(ACE_LIBRARY_RELEASE)
  list(APPEND ACE_LIBRARY optimized ${ACE_LIBRARY_RELEASE})
endif()

# Set ACE_FOUND honoring the QUIET and REQUIRED arguments
find_package_handle_standard_args(ACE DEFAULT_MSG ACE_LIBRARY_RELEASE ACE_INCLUDE_DIR)

# Output variables
if(ACE_FOUND)
  # Include dirs
  set(ACE_INCLUDE_DIRS ${ACE_INCLUDE_DIR})

  # Libraries
  set(ACE_LIBRARIES ${ACE_LIBRARY})

  # Link dirs
  #get_filename_component(ACE_LIBRARY_DIRS ${ACE_LIBRARY} PATH)

  if(NOT TARGET ACE::ACE)
    add_library(ACE::ACE UNKNOWN IMPORTED)
    set_target_properties(ACE::ACE PROPERTIES
      IMPORTED_LOCATION "${ACE_LIBRARY_RELEASE}"
      INTERFACE_INCLUDE_DIRECTORIES "${ACE_INCLUDE_DIR}"
    )
  endif()

  if(ACE_SSL_LIBRARY AND NOT TARGET ACE::SSL)
    add_library(ACE::SSL UNKNOWN IMPORTED)
    set_target_properties(ACE::SSL PROPERTIES
      IMPORTED_LOCATION "${ACE_SSL_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${ACE_INCLUDE_DIR}"
      INTERFACE_LINK_LIBRARIES ACE::ACE
    )
  endif()
endif()

# Advanced options for not cluttering the cmake UIs
mark_as_advanced(ACE_INCLUDE_DIR ACE_LIBRARY_RELEASE ACE_LIBRARY_DEBUG ACE_SSL_LIBRARY)
