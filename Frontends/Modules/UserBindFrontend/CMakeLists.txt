add_library(UserBindFrontend SHARED
  BindRequestState.cpp
  UserBindMatchRequestState.cpp
  UserBindFrontend.cpp
  RequestInfoFiller.cpp
)

target_link_libraries(UserBindFrontend
  Stream
  Geoip
  CORBACommons
  Commons
  UserIdController
  CommonModule
  CampaignManagerGrpcDistributedClient
  FeConfigXSD
  UserBindClient
  FCGI
)
