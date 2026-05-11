add_library(AdInstFrontend SHARED
  RequestInfoFiller.cpp
  AdInstFrontend.cpp
)

target_link_libraries(AdInstFrontend
  Stream
  HTTP
  Geoip
  CORBACommons
  Commons
  UserInfoManagerControllerCall
  UserInfoManagerGrpcClient
  CookieManager
  RequestMatchers
  FeConfigXSD
  CommonModule
  CampaignManagerClient
  FCGI
  ProcessingTemplates
  UserIdController
)
