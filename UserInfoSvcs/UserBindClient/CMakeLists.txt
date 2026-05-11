add_library(UserBindClient SHARED
  UserBindCorbaClient.cpp
  UserBindDistributedGrpcClient.cpp)

target_link_libraries(UserBindClient
  Generics
  Commons
  CORBACommons
  Logger
  UserBindControllerIDL
  UserBindControllerStubs
  UserBindControllerGrpc
  UserBindServerIDL
  UserBindServerGrpcClient
)

target_include_directories(UserBindClient PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../..)
