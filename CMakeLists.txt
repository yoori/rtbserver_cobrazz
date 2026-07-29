cmake_minimum_required(VERSION 3.19)

set(CMAKE_CXX_COMPILER  /opt/rh/gcc-toolset-10/root/usr/bin/g++)
set(CMAKE_C_COMPILER /opt/rh/gcc-toolset-10/root/usr/bin/gcc)

project(rtbserver)

add_library(project_cxx_std INTERFACE)
target_compile_features(project_cxx_std INTERFACE cxx_std_20)
link_libraries(project_cxx_std)

include(cmake/SetupEnvironment.cmake)
include(cmake/SetupGRPC.cmake)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/)

set(CMAKE_MODULE_PATH
  ${CMAKE_MODULE_PATH}
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules
)

set(FOROS_BOOST_ROOT "/opt/foros/boost185" CACHE PATH "Boost installation root")
if(EXISTS "${FOROS_BOOST_ROOT}/include/boost/version.hpp")
  set(BOOST_ROOT "${FOROS_BOOST_ROOT}" CACHE PATH "Boost root" FORCE)
  set(Boost_ROOT "${FOROS_BOOST_ROOT}" CACHE PATH "Boost root" FORCE)
  set(Boost_DIR
    "${FOROS_BOOST_ROOT}/lib64/cmake/Boost-1.85.0"
    CACHE PATH "Boost CMake config directory"
    FORCE)
  set(BOOST_INCLUDEDIR
    "${FOROS_BOOST_ROOT}/include"
    CACHE PATH "Boost include directory"
    FORCE)
  set(Boost_INCLUDE_DIR
    "${FOROS_BOOST_ROOT}/include"
    CACHE PATH "Boost include directory"
    FORCE)
  set(BOOST_LIBRARYDIR
    "${FOROS_BOOST_ROOT}/lib64"
    CACHE PATH "Boost library directory"
    FORCE)
  set(Boost_LIBRARY_DIR_DEBUG
    "${FOROS_BOOST_ROOT}/lib64"
    CACHE PATH "Boost debug library directory"
    FORCE)
  set(Boost_LIBRARY_DIR_RELEASE
    "${FOROS_BOOST_ROOT}/lib64"
    CACHE PATH "Boost release library directory"
    FORCE)
  set(Boost_THREAD_LIBRARY_DEBUG
    "${FOROS_BOOST_ROOT}/lib64/libboost_thread.so"
    CACHE FILEPATH "Boost thread debug library"
    FORCE)
  set(Boost_THREAD_LIBRARY_RELEASE
    "${FOROS_BOOST_ROOT}/lib64/libboost_thread.so"
    CACHE FILEPATH "Boost thread release library"
    FORCE)
  set(Boost_NO_SYSTEM_PATHS ON CACHE BOOL "Use only configured Boost root" FORCE)
  include_directories(SYSTEM "${FOROS_BOOST_ROOT}/include")
  link_directories("${FOROS_BOOST_ROOT}/lib64")
endif()

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
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
find_package(absl REQUIRED)
find_package(catboost REQUIRED)

#find_package(OpenDDS REQUIRED)

if(NOT DEFINED UNIXCOMMONS)
  message(STATUS "CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
  SET(UNIXCOMMONS "${CMAKE_CURRENT_SOURCE_DIR}/../unixcommons_cobrazz")
endif()

message(STATUS "Use unixcommons: ${UNIXCOMMONS}")
set(UNIXCOMMONS_BUILD_ROOT ${CMAKE_CURRENT_BINARY_DIR}/unixcommons)

include_directories(
  .
  Commons
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

#add_definitions( -D_USE_OCCI -DXSD_CXX11)
add_definitions(-DXSD_CXX11)

# -D_USE_OCCI -DXSD_CXX11
#link_directories( /usr/local/lib  )

ADD_SUBDIRECTORY(PredictorSvcs)
ADD_SUBDIRECTORY(Predictor)

ADD_SUBDIRECTORY(RequestInfoSvcs)
ADD_SUBDIRECTORY(ProfilingCommons)
ADD_SUBDIRECTORY(Commons/Grpc)
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
ADD_SUBDIRECTORY(tests)

install(DIRECTORY bin/ DESTINATION bin USE_SOURCE_PERMISSIONS)
install(DIRECTORY lib/ DESTINATION lib USE_SOURCE_PERMISSIONS)
install(DIRECTORY ConfigSys/ DESTINATION bin USE_SOURCE_PERMISSIONS)

install(DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} DESTINATION bin USE_SOURCE_PERMISSIONS)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/Predictor/Scripts/" DESTINATION bin
  FILES_MATCHING PATTERN "*.pl" PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ)

install(DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} DESTINATION lib
  FILES_MATCHING PATTERN "*.so" EXCLUDE)
install(DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} DESTINATION lib
  FILES_MATCHING PATTERN "*.so" PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ)

install(DIRECTORY "${CMAKE_SOURCE_DIR}/DACS" DESTINATION . USE_SOURCE_PERMISSIONS)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/CMS/Plugin/data/mibs" DESTINATION . USE_SOURCE_PERMISSIONS)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/xsd" DESTINATION . USE_SOURCE_PERMISSIONS)
