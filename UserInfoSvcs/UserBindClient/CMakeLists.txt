add_library(UserBindClient SHARED
  UserBindDistributedGrpcClient.cpp)

target_link_libraries(UserBindClient
  Generics
  Commons
  Logger
  UserBindControllerGrpc
  UserBindServerGrpcClient
)

target_include_directories(UserBindClient PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../..)
