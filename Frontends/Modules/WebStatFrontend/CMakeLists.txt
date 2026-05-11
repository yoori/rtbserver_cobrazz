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
