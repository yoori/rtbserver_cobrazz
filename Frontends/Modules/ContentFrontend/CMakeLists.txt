add_library(ContentFrontend SHARED
  ContentFrontend.cpp
)

target_link_libraries(ContentFrontend
  Stream
  Geoip
  CORBACommons
  Polyglot
  Commons
  ChannelManagerControllerIDL
  UserInfoManagerControllerIDL
  UserInfoClusterControlIDL
  UserInfoManagerControllerL
  CampaignManagerIDL
  SecToken
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
