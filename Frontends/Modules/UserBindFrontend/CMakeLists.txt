add_library(UserBindFrontend SHARED
  UserBindFrontend.cpp 
  RequestInfoFiller.cpp
)

target_link_libraries(UserBindFrontend
  ChannelManagerControllerIDL
  UserInfoManagerControllerIDL
  UserInfoClusterControlIDL
  UserInfoManagerControllerL
  CampaignManagerIDL
  UserBindServerIDL

  Stream
  CORBACommons
  Commons
  CommonModule
  SecToken
  DomainParser
  RequestMatchers
  FrontendStatistic
  UserIdController
  Geoip
  FCGI

  FeConfigXSD
)
