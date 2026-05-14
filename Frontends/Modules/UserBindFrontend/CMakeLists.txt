add_library(UserBindFrontend SHARED
  BindRequestState.cpp
  UserBindMatchRequestState.cpp
  UserBindFrontend.cpp
  RequestInfoFiller.cpp
)

target_link_libraries(UserBindFrontend
  Stream
  Geoip
  Commons
  UserIdController
  CommonModule
  CampaignManagerGrpcDistributedClient
  FeConfigXSD
  UserBindClient
  FCGI
)
