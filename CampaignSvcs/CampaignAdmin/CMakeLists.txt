add_executable(CampaignAdmin
  CampaignAdmin.cpp
)

target_link_libraries(CampaignAdmin
  CampaignManagerGrpcClient
  GrpcBatch
  UtilCommons
  String
  Generics
  gRPC::grpc++
)
