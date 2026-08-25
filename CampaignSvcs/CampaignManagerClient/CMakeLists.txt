add_library(CampaignManagerGrpcDistributedClient SHARED
  CampaignManagerDistributedGrpcClient.cpp
)

target_link_libraries(CampaignManagerGrpcDistributedClient
  PRIVATE
    GrpcBatch
    CampaignManagerGrpcClient
    gRPC::grpc++
)

target_include_directories(CampaignManagerGrpcDistributedClient PUBLIC
  ${CMAKE_BINARY_DIR}/CampaignSvcs/CampaignManager
  ${PB_DIRECTORY}/CampaignSvcs/CampaignManager)
