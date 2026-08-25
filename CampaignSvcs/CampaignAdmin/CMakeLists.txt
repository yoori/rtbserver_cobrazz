add_executable(CampaignAdmin
  CampaignAdmin.cpp
)

target_link_libraries(CampaignAdmin
  PRIVATE
    CampaignManagerGrpc
    CampaignManagerGrpcClient
    Commons
    Generics
    GrpcBatch
    Logger
    UtilCommons
    gRPC::grpc++
)
