add_library(ClickFrontend SHARED
  RequestInfoFiller.cpp
  ClickRequestState.cpp
  ClickFrontend.cpp
)

target_link_libraries(ClickFrontend
  Stream
  Geoip
  Commons
  ChannelClient
  UserInfoManagerGrpcClient
  UserBindClient
  UserIdController
  CommonModule
  CampaignManagerGrpcDistributedClient
  FeConfigXSD
  FCGI
)
