add_library(ImprTrackFrontend SHARED
  ImprTrackMatchRequestState.cpp
  ImprTrackRequestState.cpp
  ImprTrackFrontend.cpp
  RequestInfoFiller.cpp
)

target_link_libraries(ImprTrackFrontend
  Stream
  Geoip
  Commons
  ChannelGrpcClient
  UserInfoManagerGrpcClient
  UserBindClient
  RequestMatchers
  UserIdController
  CommonModule
  CampaignManagerGrpcDistributedClient
  FeConfigXSD
  FCGI
)
