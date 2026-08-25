add_grpc_sources(
  GrpcBatch
  Batch.proto
  ${PB_DIRECTORY}/Commons/Grpc)

add_grpc_sources(
  GrpcBatch
  ProcessControl.proto
  ${PB_DIRECTORY}/Commons/Grpc)

target_sources(GrpcBatch PRIVATE
  AsyncBatchingClientBase.cpp
  AsyncBatchingClientBase.hpp
  BatchingQueue.cpp
  BatchingQueue.hpp
  BatchingStreamBase.cpp
  BatchingStreamBase.hpp
  GrpcClient.hpp
  GrpcUnaryAwaiter.hpp
  GrpcExecutor.cpp
  GrpcExecutor.hpp
  GrpcServer.hpp
  GrpcServiceBase.cpp
  GrpcServiceBase.hpp
  GrpcServiceBase.tpp
)

target_link_libraries(GrpcBatch
  PUBLIC
    Commons
    Generics
    protobuf::libprotobuf
    gRPC::grpc++
)
