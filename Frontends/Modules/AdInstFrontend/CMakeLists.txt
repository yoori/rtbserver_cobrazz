add_library(AdInstFrontend SHARED
  RequestInfoFiller.cpp
  AdInstFrontend.cpp
)

target_link_libraries(AdInstFrontend
  Stream
  HTTP
  Geoip
  CORBACommons
  Polyglot
  Commons
  ChannelManagerControllerIDL
  UserInfoManagerControllerIDL
  UserInfoClusterControlIDL
  CookieManager
  RequestMatchers
  FrontendStatistic
  FeConfigXSD
  CommonModule
  CampaignManagerClient
  FCGI
  ProcessingTemplates
  UserIdController
)
