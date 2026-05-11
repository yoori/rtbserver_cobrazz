add_library(PubPixelFrontend SHARED
  RequestInfoFiller.cpp
  PubPixelFrontend.cpp
)

target_link_libraries(PubPixelFrontend
  Stream
  Geoip
  CORBACommons
  Polyglot
  Commons
  ChannelManagerControllerIDL
  UserInfoManagerControllerIDL
  UserInfoClusterControlIDL
  CampaignManagerClient
  DomainParser
  StatInfoIDL
  BillStatInfoIDL
  CampaignServerIDL
  UserBindServerIDL
  UserBindContainer
  RequestMatchers
  FrontendStatistic
  UserIdController
  CommonModule
  FeConfigXSD
  FCGI
)
