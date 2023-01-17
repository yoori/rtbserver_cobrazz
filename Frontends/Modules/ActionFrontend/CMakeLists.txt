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
# CampaignManagerStubs
 CampaignManagerIDL BidRateProvider CTRProvider CampaignConfig CampaignConfigSource CampaignIndex CreativeTemplateArgs SecToken  CampaignManagerLogger
 FeConfigXSD
 FrontendStatistic
 UserIdController
 CookieManager FCGI ProcessingTemplates RequestMatchers UserIdController

)

install(TARGETS ActionFrontend  DESTINATION ${INSTALL_DIR})