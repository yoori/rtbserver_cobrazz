add_library(PubPixelFrontend SHARED
  RequestInfoFiller.cpp
  PubPixelFrontend.cpp
)

target_link_libraries(PubPixelFrontend
  Stream
  Commons
  CampaignManagerClient
  RequestMatchers
  FeConfigXSD
  FCGI
)
