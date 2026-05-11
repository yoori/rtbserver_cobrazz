add_library(PassPixelFrontend SHARED
  RequestInfoFiller.cpp
  PassPixelFrontend.cpp
)

target_link_libraries(PassPixelFrontend
  Stream
  Geoip
  Commons
  RequestMatchers
  CommonModule
  CampaignManagerClient
  FeConfigXSD
  FCGI
)
