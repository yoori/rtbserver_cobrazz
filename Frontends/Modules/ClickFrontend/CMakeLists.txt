add_library(ClickFrontend SHARED
  RequestInfoFiller.cpp
  ClickFrontend.cpp
)

target_link_libraries(ClickFrontend
  Stream
  Geoip
  CORBACommons
  Commons
  ChannelClient
  UserInfoManagerGrpcClient
  UserBindClient
  UserIdController
  CommonModule
  CampaignManagerClient
  FeConfigXSD
  FCGI
)
