add_executable(FCGIServer
  Acceptor.cpp
  AcceptorBoostAsio.cpp
  BoostAsioContextRunActiveObject.cpp
  FCGIServer.cpp
  WorkerStatsObject.cpp
)

# CampaignIndex
# CampaignConfigSource
# CTRProvider
# BidRateProvider
# CreativeTemplateArgs
# CampaignManagerLogger
# CampaignConfig

target_link_libraries(FCGIServer
  # modules
  ActionFrontend
  AdFrontend
  ClickFrontend
  BiddingFrontend
  WebStatFrontend
  EchoFrontend
  ContentFrontend
  AdInstFrontend
  UserBindFrontend
  ImprTrackFrontend
  OptoutFrontend
  PassFrontend
  PassPixelFrontend
  PubPixelFrontend
  DirectoryModule
  # modules with grpc impl
  AdFrontendGrpc
  BiddingFrontendGrpc
  # dependencies
  CORBACommons
  CampaignManagerIDL
  Commons
  FCGI
  Generics
  GrpcCampaignManagerPool
  ProcessControlVars
  RequestMatchers
  SecToken
  UserInfoClusterControlIDL
  UserInfoManagerControllerIDL
  UserInfoManagerControllerL
  UtilCommons
  boost_system
  XMLUtility
  AdServerCommonsXSD
  FCGIServerConfigXSD
)
