add_library(ImprTrackFrontend SHARED
  ImprTrackMatchRequestState.cpp
  ImprTrackRequestState.cpp
  ImprTrackFrontend.cpp
  RequestInfoFiller.cpp
)

target_link_libraries(ImprTrackFrontend
  PRIVATE
  Geoip
  Commons
  ChannelGrpcClient
  UserInfoClient
  UserInfoManagerGrpcClient
  UserBindClient
  RequestMatchers
  UserIdController
  CommonModule
  CampaignManagerGrpcDistributedClient
  FeConfigXSD
  FCGI
)
