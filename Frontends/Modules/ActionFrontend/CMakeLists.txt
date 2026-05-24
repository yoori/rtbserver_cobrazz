add_library(ActionFrontend SHARED
  RequestInfoFiller.cpp
  ActionFrontend.cpp
  ActionFrontendStat.cpp
)

target_link_libraries(ActionFrontend
  Generics
  Geoip
  Commons
  CommonModule
  CampaignManagerGrpcDistributedClient
  FeConfigXSD
  UserBindClient
  UserIdController
  CookieManager
  FCGI
  ProcessingTemplates
  RequestMatchers
)
