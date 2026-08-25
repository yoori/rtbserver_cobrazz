cmake_minimum_required(VERSION 3.19)

project(rtbserver)

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
if(NOT EXISTS "${FOROS_BOOST_ROOT}/include/boost/version.hpp")
  message(FATAL_ERROR "Boost headers not found in ${FOROS_BOOST_ROOT}")
endif()

set(Boost_DIR
  "${FOROS_BOOST_ROOT}/lib64/cmake/Boost-1.85.0"
  CACHE PATH "Boost CMake config directory"
  FORCE)

find_package(Boost 1.85.0 CONFIG REQUIRED COMPONENTS system thread)

if(NOT TARGET Boost::headers)
  message(FATAL_ERROR "Boost::headers target not found")
endif()

get_target_property(
  BOOST_HEADERS_COMPILE_OPTIONS
  Boost::headers
  INTERFACE_COMPILE_OPTIONS)
if(NOT BOOST_HEADERS_COMPILE_OPTIONS)
  set(BOOST_HEADERS_COMPILE_OPTIONS)
endif()
if(NOT "-msse2" IN_LIST BOOST_HEADERS_COMPILE_OPTIONS)
  set_property(TARGET Boost::headers APPEND PROPERTY INTERFACE_COMPILE_OPTIONS -msse2)
endif()
unset(BOOST_HEADERS_COMPILE_OPTIONS)

find_package(ACE REQUIRED)
find_package(TAO REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(XercesC REQUIRED)
find_package(Threads REQUIRED)
find_package(LibXml2 REQUIRED)
find_package(LibXslt REQUIRED)
find_package(Nghttp2 REQUIRED)
find_package(PostgreSQL REQUIRED)
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
find_package(xgboost CONFIG REQUIRED)
find_package(Vanga REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(Jemalloc REQUIRED IMPORTED_TARGET jemalloc)
pkg_check_modules(RdKafka REQUIRED IMPORTED_TARGET rdkafka++)
pkg_check_modules(ZeroMQ REQUIRED IMPORTED_TARGET libzmq)

if(NOT DEFINED UNIXCOMMONS)
  message(STATUS "CMAKE_CURRENT_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
  set(UNIXCOMMONS "${CMAKE_CURRENT_SOURCE_DIR}/../unixcommons_cobrazz")
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

add_compile_definitions(XSD_CXX11)

add_subdirectory(Predictor)
add_subdirectory(RequestInfoSvcs)
add_subdirectory(ProfilingCommons)
add_subdirectory(Commons/Grpc)
add_subdirectory(UserInfoSvcs)
add_subdirectory(Frontends)
add_subdirectory(Controlling)
add_subdirectory(CampaignSvcs)
add_subdirectory(Commons)
add_subdirectory(xsd)
add_subdirectory(LogProcessing)
add_subdirectory(LogCommons)
add_subdirectory(UtilCommons)
add_subdirectory(Utils)
add_subdirectory(Plain)
add_subdirectory(ChannelSearchSvcs)
add_subdirectory(ChannelSvcs)
add_subdirectory(tests)

install(DIRECTORY bin/ DESTINATION bin USE_SOURCE_PERMISSIONS)
install(DIRECTORY lib/ DESTINATION lib USE_SOURCE_PERMISSIONS)
install(DIRECTORY modules/Predictor/ DESTINATION lib/Predictor USE_SOURCE_PERMISSIONS)
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
