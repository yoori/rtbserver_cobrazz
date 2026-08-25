add_library(ContentFrontend SHARED
  ContentFrontend.cpp
)

target_link_libraries(ContentFrontend
  PRIVATE
  Commons
  CampaignManagerGrpcDistributedClient
  FeConfigXSD
  FCGI
)
