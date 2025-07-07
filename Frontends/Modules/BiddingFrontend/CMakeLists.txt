add_pb(baidu-biddingPB baidu-bidding.proto ${PB_DIRECTORY}/Frontends/Modules/BiddingFrontend)
add_pb(google-biddingPB google-bidding.proto ${PB_DIRECTORY}/Frontends/Modules/BiddingFrontend)
add_pb(tanx-biddingPB tanx-bidding.proto ${PB_DIRECTORY}/Frontends/Modules/BiddingFrontend)

add_library(BiddingFrontend SHARED
  AdJsonBidRequestTask.cpp
  AdXmlBidRequestTask.cpp
  AdXmlRequestInfoFiller.cpp
  AppNexusBidRequestTask.cpp
  BidRequestTask.cpp
  BiddingFrontend.cpp
  BiddingFrontendStat.cpp
  ClickStarBidRequestTask.cpp
  DAOBidRequestTask.cpp
  GoogleBidRequestTask.cpp
  KeywordFormatter.cpp
  OpenRtbBidRequestTask.cpp
  PlannerPool.cpp
  RequestInfoFiller.cpp
  Utils.cpp
)

target_link_libraries(BiddingFrontend
  BillStatInfoIDL
  CORBACommons
  CTRProvider
  CampaignManagerIDL
  CampaignServerIDL
  ChannelManagerControllerIDL
  CommonModule
  Commons
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
