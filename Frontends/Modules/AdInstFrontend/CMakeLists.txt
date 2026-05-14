add_library(AdInstFrontend SHARED
  RequestInfoFiller.cpp
  AdInstFrontend.cpp
)

target_link_libraries(AdInstFrontend
  Stream
  HTTP
  Geoip
  Commons
  UserInfoManagerGrpcClient
  CookieManager
  RequestMatchers
  FeConfigXSD
  CommonModule
  CampaignManagerGrpcDistributedClient
  FCGI
  ProcessingTemplates
  UserIdController
)
