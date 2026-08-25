add_library(ActionFrontend SHARED
  RequestInfoFiller.cpp
  ActionFrontend.cpp
  ActionFrontendStat.cpp
)

target_link_libraries(ActionFrontend
  PRIVATE
  Generics
  Geoip
  Commons
  CommonModule
  ChannelGrpcClient
  CampaignManagerGrpcDistributedClient
  FeConfigXSD
  UserInfoClient
  UserBindClient
  UserIdController
  FCGI
  RequestMatchers
)
