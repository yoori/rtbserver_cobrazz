add_library(UserBindClient SHARED
  UserBindDistributedGrpcClient.cpp)

target_link_libraries(UserBindClient
  PRIVATE
    Generics
    Commons
    Logger
    UserBindControllerGrpc
    UserBindServerGrpcClient
)

target_include_directories(UserBindClient PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../..
  ${CMAKE_BINARY_DIR}/UserInfoSvcs/UserBindServer
  ${PB_DIRECTORY}/UserInfoSvcs/UserBindServer)
