add_library(UserBindFrontend SHARED
UserBindFrontend.cpp 
  RequestInfoFiller.cpp
)


target_link_libraries(UserBindFrontend



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

install(TARGETS UserBindFrontend  DESTINATION ${INSTALL_DIR})