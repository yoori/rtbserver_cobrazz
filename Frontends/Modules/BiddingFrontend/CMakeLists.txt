add_pb(google-biddingPB google-bidding.proto ${PB_DIRECTORY}/Frontends/Modules/BiddingFrontend)

add_library(BiddingFrontend SHARED
  RequestInfoFiller.cpp
  DebugSink.cpp
  AdXmlRequestInfoFiller.cpp
  BiddingFrontend.cpp
  BiddingFrontendStat.cpp
  PlannerPool.cpp
  BidRequestTask.cpp
  OpenRtbBidRequestTask.cpp
  GoogleBidRequestTask.cpp
  AdXmlBidRequestTask.cpp
  AdJsonBidRequestTask.cpp
  ClickStarBidRequestTask.cpp
  DAOBidRequestTask.cpp
  KeywordFormatter.cpp
  RequestMetricsProvider.cpp
  Utils.cpp
)

target_link_libraries(BiddingFrontend
  Stream
  Geoip
  CORBACommons
  Commons
  ChannelClient
  UserInfoManagerControllerCall
  UserInfoManagerGrpcClient
  UserBindClient
  RequestMatchers
  UserIdController
  CommonModule
  CampaignManagerClient
  FeConfigXSD
  google-biddingPB
  Gason
  ${PROTOBUF_LDFLAGS}
  protoc
  FCGI
)
