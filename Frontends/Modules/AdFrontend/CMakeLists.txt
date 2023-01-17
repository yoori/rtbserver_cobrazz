add_library(AdFrontend SHARED
    RequestInfoFiller.cpp 
  AdFrontend.cpp 
  AdFrontendStat.cpp 
  DebugSink.cpp
)
target_link_libraries(AdFrontend

 Stream
 HTTP
 Geoip
  CORBACommons

 Polyglot

 Commons
# ChannelControllerStubs
ChannelManagerControllerIDL
# UserInfoManagerControllerStubs
UserInfoManagerControllerIDL UserInfoClusterControlIDL  UserInfoManagerControllerL
# CampaignManagerStubs
 CampaignManagerIDL BidRateProvider CTRProvider CampaignConfig CampaignConfigSource CampaignIndex CreativeTemplateArgs SecToken  CampaignManagerLogger
 CookieManager
 RequestMatchers
 FrontendStatistic

 FeConfigXSD
 CommonModule

CookieManager FCGI ProcessingTemplates  RequestMatchers UserIdController
)


install(TARGETS  AdFrontend DESTINATION ${INSTALL_DIR})