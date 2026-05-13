add_pb(google-biddingPB google-bidding.proto ${PB_DIRECTORY}/Frontends/Modules/BiddingFrontend)

add_library(BiddingFrontend SHARED
  RequestInfoFiller.cpp
  DebugSink.cpp
  AdXmlRequestInfoFiller.cpp
  BiddingFrontend.cpp
  BiddingFrontendStat.cpp
  BidRequestState.cpp
  OpenRtbBidRequestState.cpp
  GoogleBidRequestState.cpp
  AdXmlBidRequestState.cpp
  AdJsonBidRequestState.cpp
  ClickStarBidRequestState.cpp
  DAOBidRequestState.cpp
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
  CampaignManagerGrpcDistributedClient
  FeConfigXSD
  google-biddingPB
  Gason
  ${PROTOBUF_LDFLAGS}
  protoc
  FCGI
)
