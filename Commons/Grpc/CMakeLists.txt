add_grpc_sources(
  GrpcBatch
  Batch.proto
  ${PB_DIRECTORY}/Commons/Grpc)

add_grpc_sources(
  GrpcBatch
  ProcessControl.proto
  ${PB_DIRECTORY}/Commons/Grpc)

target_link_libraries(GrpcBatch protobuf::libprotobuf)

target_sources(GrpcBatch PRIVATE
  AsyncBatchingClientBase.cpp
  AsyncBatchingClientBase.hpp
  BatchingQueue.cpp
  BatchingQueue.hpp
  BatchingStreamBase.cpp
  BatchingStreamBase.hpp
  GrpcClient.hpp
  GrpcExecutor.cpp
  GrpcExecutor.hpp
  GrpcServer.hpp
  GrpcServiceBase.hpp
)

target_link_libraries(GrpcBatch
  Commons
  Generics
  gRPC::grpc++
)
