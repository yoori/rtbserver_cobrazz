add_pb(google-biddingPB google-bidding.proto ${PB_DIRECTORY}/Frontends/Modules/BiddingFrontend)

add_library(BiddingFrontend SHARED
  RequestInfoFiller.cpp
  DebugSink.cpp
  GrpcClientMetricsProvider.cpp
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
  BiddingFrontendCore.cpp
  KeywordFormatter.cpp
  Utils.cpp
)

target_link_libraries(BiddingFrontend
  Stream
  Geoip
  Commons
  ChannelGrpcClient
  UserInfoManagerGrpcClient
  UserBindClient
  RequestMatchers
  UserIdController
  CommonModule
  CampaignManagerGrpcDistributedClient
  FeConfigXSD
  google-biddingPB
  ${PROTOBUF_LDFLAGS}
  protoc
  FCGI
)
