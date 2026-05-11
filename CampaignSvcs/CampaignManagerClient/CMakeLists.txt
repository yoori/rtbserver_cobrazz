add_library(CampaignManagerClient SHARED
  CampaignManagerCorbaClient.cpp
)

target_link_libraries(CampaignManagerClient
  CORBACommons
  CampaignManagerGrpcClient
  CampaignManagerIDL
)

add_library(CampaignManagerGrpcDistributedClient SHARED
  CampaignManagerDistributedGrpcClient.cpp
)

target_link_libraries(CampaignManagerGrpcDistributedClient
  GrpcBatch
  CampaignManagerGrpcClient
  gRPC::grpc++
)
