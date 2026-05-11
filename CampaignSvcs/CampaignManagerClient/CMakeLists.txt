add_library(CampaignManagerClient SHARED
  CampaignManagerCorbaClient.cpp
)

target_link_libraries(CampaignManagerClient
  CORBACommons
  CampaignManagerIDL
  CampaignManagerGrpcClient
)
