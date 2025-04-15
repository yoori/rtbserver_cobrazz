add_library(PassFrontend SHARED
  RequestInfoFiller.cpp 
  PassFrontend.cpp
)

target_link_libraries(PassFrontend
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
