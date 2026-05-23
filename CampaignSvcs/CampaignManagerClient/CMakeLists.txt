add_library(CampaignManagerGrpcDistributedClient SHARED
  CampaignManagerDistributedGrpcClient.cpp
)

target_link_libraries(CampaignManagerGrpcDistributedClient
  GrpcBatch
  CampaignManagerGrpcClient
  gRPC::grpc++
)
