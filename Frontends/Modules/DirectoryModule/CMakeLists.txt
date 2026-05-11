add_library(DirectoryModule SHARED
  DirectoryModule.cpp
)

target_link_libraries(DirectoryModule
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
  FeConfigXSD
  FCGI
)
