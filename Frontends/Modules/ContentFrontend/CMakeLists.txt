add_library(ContentFrontend SHARED
  ContentFrontend.cpp
)

target_link_libraries(ContentFrontend
  Stream
  Commons
  CampaignManagerGrpcDistributedClient
  FeConfigXSD
  FCGI
)
