cmake_minimum_required(VERSION 3.19)

project (unixcommons)

if(UNIX AND NOT APPLE)
SET(LINUX true)
endif()

if(LINUX)
    set(CMAKE_PREFIX_PATH $ENV{HOME}/Qt/5.15.2/gcc_64)
    set(CMAKE_SHARED_LINKER_FLAGS "-Xlinker --export-dynamic -Xlinker -zdefs")
else()
    set(CMAKE_PREFIX_PATH $ENV{HOME}/Qt/5.15.2/clang_64)
endif()

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(CTest)
endif()
 set(CMAKE_MODULE_PATH
    ${CMAKE_MODULE_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules
  )

set(INSTALL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../cobrazz-bin)
set(INSTALL_LIB ${INSTALL_DIR})
set(INSTALL_BIN  ${INSTALL_DIR})


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

set(UNIXCOMMONS ${CMAKE_CURRENT_SOURCE_DIR}/../unixcommons_cobrazz)

include_directories( /usr/local/include src Commons .
    ${UNIXCOMMONS}/src
    ${UNIXCOMMONS}/src/CORBA
    ${UNIXCOMMONS}/Build/IDL_COMMON


)

set(CORBA_INCLUDES ${UNIXCOMMONS}/src/CORBA)
set(TMP_SOURCES ${CMAKE_CURRENT_BINARY_DIR})

#set(INSTALL_DIR ~/rtbserver_cobrazz-bin)

set(PB_DIRECTORY ${TMP_SOURCES}/PB_COMMON)
include_directories(${PB_DIRECTORY})

set(IDL_DIRECTORY ${TMP_SOURCES}/IDL_COMMON)
include_directories(${IDL_DIRECTORY})


set(XSD_DIRECTORY ${TMP_SOURCES}/XSD_COMMON)
include_directories(${XSD_DIRECTORY} ${XSD_DIRECTORY}/xsd)

set(PST_DIRECTORY ${TMP_SOURCES}/PST_COMMON)
include_directories(${PST_DIRECTORY})


#include_directories(Utils/Predictor)
add_subdirectory(${UNIXCOMMONS}  ${CMAKE_CURRENT_BINARY_DIR}/unixcommons)

link_directories(~/unixcommons-cobrazz-bin2/lib)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a -fPIC ")
add_definitions( -D_USE_OCCI -DXSD_CXX11)

# -D_USE_OCCI -DXSD_CXX11
link_directories( /usr/local/lib  )

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


    

