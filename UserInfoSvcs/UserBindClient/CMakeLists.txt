add_library(UserBindClient SHARED
  UserBindCorbaClient.cpp
  UserBindDistributedGrpcClient.cpp)

target_link_libraries(UserBindClient
  Generics
  Commons
  CORBACommons
  Logger
  FeConfigXSD
  UserBindControllerIDL
  UserBindControllerStubs
  UserBindControllerGrpc
  UserBindServerIDL
  UserBindServerGrpc
  UserBindServerGrpcClient
  GrpcBatch
  gRPC::grpc++)

target_include_directories(UserBindClient PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../..)
