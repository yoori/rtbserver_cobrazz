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
