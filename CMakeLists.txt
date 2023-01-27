cmake_minimum_required(VERSION 3.19)

project(rtbserver)

include(cmake/SetupEnvironment.cmake)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/)

set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules
)

find_package(ACE REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(XercesC REQUIRED)
find_package(Threads REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(YY REQUIRED)
find_package(LL REQUIRED)

find_package(IDL REQUIRED)
find_package(XSD REQUIRED)
find_package(PST REQUIRED)
find_package(PB REQUIRED)

#find_package(OpenDDS REQUIRED)

if(NOT DEFINED UNIXCOMMONS)
  SET(UNIXCOMMONS "${CMAKE_CURRENT_SOURCE_DIR}/../unixcommons_cobrazz")
endif()

message(STATUS "Use unixcommons: ${UNIXCOMMONS}")
set(UNIXCOMMONS_BUILD_ROOT ${CMAKE_CURRENT_BINARY_DIR}/unixcommons)

include_directories( /usr/local/include src Commons .
  ${UNIXCOMMONS}/src
  ${UNIXCOMMONS}/src/CORBA
  ${UNIXCOMMONS_BUILD_ROOT}/IDL_COMMON
)

set(CORBA_INCLUDES ${UNIXCOMMONS}/src/CORBA)
set(TMP_SOURCES ${CMAKE_CURRENT_BINARY_DIR})

set(PB_DIRECTORY ${TMP_SOURCES}/PB_COMMON)
include_directories(${PB_DIRECTORY})

set(IDL_DIRECTORY ${TMP_SOURCES}/IDL_COMMON)
include_directories(${IDL_DIRECTORY})

set(XSD_DIRECTORY ${TMP_SOURCES}/XSD_COMMON)
include_directories(${XSD_DIRECTORY} ${XSD_DIRECTORY}/xsd)

set(PST_DIRECTORY ${TMP_SOURCES}/PST_COMMON)
include_directories(${PST_DIRECTORY})

add_subdirectory(${UNIXCOMMONS} ${UNIXCOMMONS_BUILD_ROOT})

link_directories(~/unixcommons-cobrazz-bin2/lib)

add_definitions( -D_USE_OCCI -DXSD_CXX11)

# -D_USE_OCCI -DXSD_CXX11
#link_directories( /usr/local/lib  )

ADD_SUBDIRECTORY(PredictorSvcs)
ADD_SUBDIRECTORY(Predictor)

ADD_SUBDIRECTORY(RequestInfoSvcs)
ADD_SUBDIRECTORY(ProfilingCommons)
ADD_SUBDIRECTORY(UserInfoSvcs)
ADD_SUBDIRECTORY(Frontends)
ADD_SUBDIRECTORY(Controlling)
ADD_SUBDIRECTORY(CampaignSvcs)
ADD_SUBDIRECTORY(Commons)
ADD_SUBDIRECTORY(xsd)
ADD_SUBDIRECTORY(LogProcessing)
ADD_SUBDIRECTORY(LogCommons)
ADD_SUBDIRECTORY(UtilCommons)
ADD_SUBDIRECTORY(Utils)
ADD_SUBDIRECTORY(Plain)
ADD_SUBDIRECTORY(ChannelSearchSvcs)
ADD_SUBDIRECTORY(ChannelSvcs)
