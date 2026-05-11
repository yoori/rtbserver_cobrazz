add_library(UserInfoClient SHARED
  UserInfoCorbaClient.cpp
  UserInfoDistributedGrpcClient.cpp
)

target_link_libraries(UserInfoClient
  CORBACommons
  Generics
  Commons
  Logger
  UserInfoManagerControllerCall
  UserInfoControllerGrpc
  UserInfoManagerGrpcClient
)

target_include_directories(UserInfoClient PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../..
  ${PB_DIRECTORY}/UserInfoSvcs/UserInfoController2
  ${PB_DIRECTORY}/UserInfoSvcs/UserInfoManager
  ${CMAKE_BINARY_DIR}/UserInfoSvcs/UserInfoManager
)
