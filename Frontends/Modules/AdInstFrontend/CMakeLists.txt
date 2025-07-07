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
  UserInfoManagerControllerIDL UserInfoClusterControlIDL  UserInfoManagerControllerL
  CampaignManagerIDL
  SecToken
  CookieManager
  RequestMatchers
  FrontendStatistic

  FeConfigXSD
  CommonModule

  CookieManager
  FCGI
  ProcessingTemplates
  RequestMatchers
  UserIdController
)
