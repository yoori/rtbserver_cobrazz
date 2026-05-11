add_library(ImprTrackFrontend SHARED
  ImprTrackFrontend.cpp
  RequestInfoFiller.cpp
)

target_link_libraries(ImprTrackFrontend
  Stream
  Geoip
  CORBACommons
  Commons
  ChannelClient
  UserInfoManagerControllerCall
  UserInfoManagerGrpcClient
  UserBindServerIDL
  UserBindClient
  RequestMatchers
  UserIdController
  CommonModule
  CampaignManagerGrpcDistributedClient
  FeConfigXSD
  FCGI
)
