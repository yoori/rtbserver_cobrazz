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
  ChannelManagerControllerIDL
  UserInfoManagerControllerIDL
  UserInfoClusterControlIDL
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
  CampaignManagerClient
  FeConfigXSD
  FCGI
)
