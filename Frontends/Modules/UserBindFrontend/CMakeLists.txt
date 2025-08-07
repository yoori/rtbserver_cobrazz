add_library(UserBindFrontend SHARED
  RequestInfoFiller.cpp
  UserBindFrontend.cpp
  UserChannelsHandler.cpp
)

target_link_libraries(UserBindFrontend
  CORBACommons
  CampaignManagerIDL
  ChannelManagerControllerIDL
  CommonModule
  Commons
  DomainParser
  FCGI
  FeConfigXSD
  FrontendStatistic
  Geoip
  RequestMatchers
  SecToken
  Stream
  UserBindServerIDL
  UserIdController
  UserInfoClusterControlIDL
  UserInfoManagerControllerIDL
  UserInfoManagerControllerL
)
