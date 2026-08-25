add_executable(GrpcProbeObj
  Application.cpp
)

target_link_libraries(GrpcProbeObj
  PRIVATE
    GrpcBatch
    gRPC::grpc++
    protobuf::libprotobuf
)
