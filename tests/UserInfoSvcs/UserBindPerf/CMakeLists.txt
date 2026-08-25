add_executable(UserBindGrpcPerf
  UserBindGrpcPerf.cpp)

target_link_libraries(UserBindGrpcPerf
  PRIVATE
    Generics
    Commons
    GrpcBatch
    UserBindServerGrpc
    UserBindServerGrpcClient
    UserBindClient
    gRPC::grpc++
)

target_include_directories(UserBindGrpcPerf
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../../..
)
