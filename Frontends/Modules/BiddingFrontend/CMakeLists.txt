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
  BidRateProvider
  BillStatInfoIDL
  CORBACommons
  CTRProvider
  CampaignConfig
  CampaignConfigSource
  CampaignIndex
  CampaignManagerIDL
  CampaignManagerLogger
  CampaignServerIDL
  ChannelManagerControllerIDL
  CommonModule
  Commons
  CreativeTemplateArgs
  DomainParser
  FCGI
  FeConfigXSD
  FrontendStatistic
  Gason
  Geoip
  Polyglot
  RequestMatchers
  SecToken
  StatInfoIDL
  Stream
  UServerUtils
  UserBindContainer
  UserBindServerIDL
  UserIdController
  UserInfoClusterControlIDL
  UserInfoManagerControllerIDL
  UserInfoManagerControllerL
  baidu-biddingPB
  google-biddingPB
  protobuf
  protoc
  tanx-biddingPB
)
