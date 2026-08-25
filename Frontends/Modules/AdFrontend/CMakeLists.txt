add_library(AdFrontend SHARED
  RequestInfoFiller.cpp
  AdFrontend.cpp
  AdFrontendStat.cpp
  DebugSink.cpp
)

target_link_libraries(AdFrontend
  PRIVATE
  HTTP
  Geoip
  Commons
  ChannelGrpcClient
  UserInfoClient
  UserInfoManagerGrpcClient
  RequestMatchers
  FeConfigXSD
  CommonModule
  CampaignManagerGrpcDistributedClient
  UserBindClient
  FCGI
  UserIdController
)
