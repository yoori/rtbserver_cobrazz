add_library(PassFrontend SHARED
  RequestInfoFiller.cpp
  PassFrontend.cpp
)

target_link_libraries(PassFrontend
  Stream
  Commons
  RequestMatchers
  CommonModule
  CampaignManagerGrpcDistributedClient
  FeConfigXSD
  FCGI
)
