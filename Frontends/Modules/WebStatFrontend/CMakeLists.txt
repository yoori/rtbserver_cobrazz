add_library(WebStatFrontend SHARED
  RequestInfoFiller.cpp
  WebStatFrontend.cpp
)

target_link_libraries(WebStatFrontend
  PRIVATE
  Commons
  RequestMatchers
  CommonModule
  CampaignManagerGrpcDistributedClient
  FeConfigXSD
  FCGI
)
