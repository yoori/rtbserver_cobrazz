add_library(WebStatFrontend SHARED
  RequestInfoFiller.cpp
  WebStatRequestState.cpp
  WebStatFrontend.cpp
)

target_link_libraries(WebStatFrontend
  Stream
  Commons
  RequestMatchers
  CommonModule
  CampaignManagerGrpcDistributedClient
  FeConfigXSD
  FCGI
)
