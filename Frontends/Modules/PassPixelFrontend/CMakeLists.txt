add_library(PassPixelFrontend SHARED
  RequestInfoFiller.cpp 
  PassPixelFrontend.cpp
)

target_link_libraries(PassPixelFrontend
  Stream
  Geoip
  CORBACommons
  Polyglot
  Commons
  CampaignManagerIDL
  SecToken
  DomainParser
  RequestMatchers
  FrontendStatistic
  UserIdController
  CommonModule
  FCGI
  FeConfigXSD
)
