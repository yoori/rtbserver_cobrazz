add_library(AdInstFrontend SHARED
RequestInfoFiller.cpp 
  AdInstFrontend.cpp
)
target_link_libraries(AdInstFrontend

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

install(TARGETS AdInstFrontend  DESTINATION ${INSTALL_DIR})
