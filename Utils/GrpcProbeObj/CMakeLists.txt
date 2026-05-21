add_executable(GrpcProbeObj
  Application.cpp
)

target_link_libraries(GrpcProbeObj
  GrpcBatch
  gRPC::grpc++
  protobuf::libprotobuf
)
