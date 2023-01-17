add_library(PubPixelFrontend SHARED
 RequestInfoFiller.cpp 
  PubPixelFrontend.cpp
)


target_link_libraries(PubPixelFrontend



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

install(TARGETS PubPixelFrontend DESTINATION ${INSTALL_DIR})