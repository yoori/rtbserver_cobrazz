add_library(ActionFrontend SHARED
  RequestInfoFiller.cpp
  ActionFrontend.cpp
  ActionFrontendStat.cpp
)

target_link_libraries(ActionFrontend
  Generics
  Geoip
  Commons
  CORBACommons
  CommonModule
  CampaignManagerGrpcDistributedClient
  FeConfigXSD
  FrontendStatistic
  UserBindClient
  UserIdController
  CookieManager
  FCGI
  ProcessingTemplates
  RequestMatchers
)
