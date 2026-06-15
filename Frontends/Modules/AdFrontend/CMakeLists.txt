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
  Commons
  ChannelGrpcClient
  UserInfoManagerGrpcClient
  CookieManager
  RequestMatchers
  FeConfigXSD
  CommonModule
  CampaignManagerGrpcDistributedClient
  ExpressionChannelIndex
  UserBindClient
  FCGI
  ProcessingTemplates
  UserIdController
)
