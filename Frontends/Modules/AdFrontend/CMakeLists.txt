add_library(AdFrontend SHARED
  RequestInfoFiller.cpp
  AdFrontend.cpp
  AdFrontendStat.cpp
  DebugSink.cpp
)

target_link_libraries(AdFrontend
  Stream
  HTTP
  Geoip
  CORBACommons
  Commons
  ChannelClient
  UserInfoManagerControllerCall
  UserInfoManagerGrpcClient
  CookieManager
  RequestMatchers
  FrontendStatistic
  FeConfigXSD
  CommonModule
  CampaignManagerClient
  ExpressionChannelIndex
  UserBindClient
  FCGI
  ProcessingTemplates
  UserIdController
)
