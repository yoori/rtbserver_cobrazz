add_library(AdInstFrontend SHARED
  RequestInfoFiller.cpp
  AdInstFrontend.cpp
)

target_link_libraries(AdInstFrontend
  PRIVATE
  HTTP
  Geoip
  Commons
  UserInfoClient
  UserInfoManagerGrpcClient
  RequestMatchers
  FeConfigXSD
  CommonModule
  CampaignManagerGrpcDistributedClient
  FCGI
  UserIdController
)
