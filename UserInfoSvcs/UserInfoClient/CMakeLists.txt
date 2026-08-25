add_library(UserInfoClient SHARED
  UserInfoGrpcAlgs.cpp
  UserInfoDistributedGrpcClient.cpp
)

target_link_libraries(UserInfoClient
  PRIVATE
  Generics
  Commons
  Logger
  UserInfoControllerGrpc
  UserInfoManagerGrpcClient
)

target_include_directories(UserInfoClient PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../..
  ${PB_DIRECTORY}/UserInfoSvcs/UserInfoController
  ${PB_DIRECTORY}/UserInfoSvcs/UserInfoManager
  ${CMAKE_BINARY_DIR}/UserInfoSvcs/UserInfoManager
)
