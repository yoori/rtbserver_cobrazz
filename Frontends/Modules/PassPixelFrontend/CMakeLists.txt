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
