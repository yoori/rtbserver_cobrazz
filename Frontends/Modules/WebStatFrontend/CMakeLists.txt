add_library(WebStatFrontend SHARED
  RequestInfoFiller.cpp 
  WebStatFrontend.cpp
)


target_link_libraries(WebStatFrontend



 Stream
 Geoip
 CORBACommons

 Polyglot

 Commons
 #ChannelControllerStubs
 ChannelManagerControllerIDL
# UserInfoManagerControllerStubs
UserInfoManagerControllerIDL UserInfoClusterControlIDL  UserInfoManagerControllerL

# CampaignManagerStubs
CampaignManagerIDL BidRateProvider CTRProvider CampaignConfig CampaignConfigSource CampaignIndex CreativeTemplateArgs SecToken  CampaignManagerLogger

 DomainParser
# CampaignServerStubs
StatInfoIDL BillStatInfoIDL CampaignServerIDL

# UserBindServerStubs
UserBindServerIDL UserBindContainer

 RequestMatchers
 FrontendStatistic
 UserIdController
 CommonModule

 FeConfigXSD
FCGI
)
install(TARGETS WebStatFrontend  DESTINATION ${INSTALL_DIR})
