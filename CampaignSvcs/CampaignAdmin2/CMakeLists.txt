add_executable(CampaignAdmin2
  CampaignAdmin2.cpp
)

target_link_libraries(CampaignAdmin2
  CampaignManagerGrpcClient
  GrpcBatch
  UtilCommons
  String
  Generics
  gRPC::grpc++
)
