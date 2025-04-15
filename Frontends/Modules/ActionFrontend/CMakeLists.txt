add_library(ActionFrontend SHARED
  ActionFrontend.cpp
  ActionFrontendStat.cpp
  RequestInfoFiller.cpp
)

target_link_libraries(ActionFrontend
  CORBACommons
  CampaignManagerIDL
  CommonModule
  Commons
  CookieManager
  FCGI
  FeConfigXSD
  FrontendStatistic
  Generics
  Geoip
  ProcessingTemplates
  RequestMatchers
  SecToken
  UserIdController
)
