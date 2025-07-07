add_library(WebStatFrontend SHARED
  RequestInfoFiller.cpp 
  WebStatFrontend.cpp
)

target_link_libraries(WebStatFrontend
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
  FeConfigXSD
  FCGI
)
