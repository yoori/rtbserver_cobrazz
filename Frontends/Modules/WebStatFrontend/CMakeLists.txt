add_library(WebStatFrontend SHARED
  RequestInfoFiller.cpp
  WebStatFrontend.cpp
)

target_link_libraries(WebStatFrontend
  Stream
  Commons
  Gason
  RequestMatchers
  CommonModule
  CampaignManagerGrpcDistributedClient
  FeConfigXSD
  FCGI
)
