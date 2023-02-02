add_pb(baidu-biddingPB baidu-bidding.proto ${PB_DIRECTORY}/Frontends/Modules/BiddingFrontend)
add_pb(google-biddingPB google-bidding.proto ${PB_DIRECTORY}/Frontends/Modules/BiddingFrontend)
add_pb(tanx-biddingPB tanx-bidding.proto ${PB_DIRECTORY}/Frontends/Modules/BiddingFrontend)


add_library(BiddingFrontend SHARED
  RequestInfoFiller.cpp 
  AdXmlRequestInfoFiller.cpp 
  BiddingFrontend.cpp 
  BiddingFrontendStat.cpp 
  PlannerPool.cpp 
  BidRequestTask.cpp 
  OpenRtbBidRequestTask.cpp 
  GoogleBidRequestTask.cpp 
  AppNexusBidRequestTask.cpp 
  AdXmlBidRequestTask.cpp 
  AdJsonBidRequestTask.cpp 
  ClickStarBidRequestTask.cpp 
  DAOBidRequestTask.cpp 
  KeywordFormatter.cpp

)
target_link_libraries(BiddingFrontend 

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
 baidu-biddingPB
 google-biddingPB
 tanx-biddingPB
 Gason
protobuf
protoc
FCGI
z
MetricsHTTPProvider
)

install(TARGETS BiddingFrontend  DESTINATION ${INSTALL_DIR})