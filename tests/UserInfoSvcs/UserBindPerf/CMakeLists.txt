add_executable(UserBindGrpcPerf
  UserBindGrpcPerf.cpp
  UserBindAsyncClient.cpp
  UserBindSyncClient.cpp)

add_adserver_grpc_client_sources(
  UserBindGrpcPerf
  ${CMAKE_SOURCE_DIR}/UserInfoSvcs/UserBindServer/UserBindServerGrpc.proto
  ${CMAKE_CURRENT_BINARY_DIR}
  AdServer::UserInfoSvcs
  --proto_path=${CMAKE_SOURCE_DIR}/Commons/Grpc)

target_link_libraries(UserBindGrpcPerf
  PRIVATE
    Generics
    Commons
    GrpcBatch
    UserBindServerGrpc
    gRPC::grpc++
)

target_include_directories(UserBindGrpcPerf
  PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..
)

add_executable(UserBindCorbaPerf
  UserBindCorbaPerf.cpp)

target_link_libraries(UserBindCorbaPerf
  PRIVATE
    Generics
    CORBACommons
    Commons
    UserBindServerIDL
)

target_include_directories(UserBindCorbaPerf
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..
)
