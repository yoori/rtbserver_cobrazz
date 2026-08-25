add_library(BillingServerGrpcClient STATIC)

add_adserver_grpc_client_sources(
  BillingServerGrpcClient
  ${CMAKE_SOURCE_DIR}/CampaignSvcs/BillingServer/BillingServerGrpc.proto
  ${CMAKE_CURRENT_BINARY_DIR}
  AdServer::CampaignSvcs
  --proto_path=${CMAKE_SOURCE_DIR}/Commons/Grpc)

target_link_libraries(BillingServerGrpcClient
  PUBLIC
  Commons
  GrpcBatch
  BillingServerGrpc
  gRPC::grpc++
  PRIVATE
  Sync)
