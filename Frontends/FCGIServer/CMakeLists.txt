add_library(FrontendServerCommons
  FrontendsPool.cpp
  Acceptor.cpp
  FCGIAcceptor.cpp
  Http2Acceptor.cpp
  WorkerStatsObject.cpp
  BoostAsioContextRunActiveObject.cpp
)

target_link_libraries(FrontendServerCommons
  FCGI
)

add_executable(FCGIServer
  FCGIServer.cpp
)

target_link_libraries(FCGIServer
  ProcessControlVars
  RequestMatchers
  CreativeTemplateArgs

  UserInfoManagerControllerCall
  CampaignManagerIDL
  UserInfoClusterControlIDL
  UserInfoManagerControllerIDL
  CampaignServerIDL

  AdServerCommonsXSD
  FCGIServerConfigXSD

  BiddingFrontend
  DirectoryModule
  PubPixelFrontend
  ContentFrontend
  WebStatFrontend
  ActionFrontend
  UserBindFrontend
  PassFrontend
  PassPixelFrontend
  OptoutFrontend
  AdInstFrontend
  ClickFrontend
  ImprTrackFrontend
  AdFrontend

  FCGI
  UtilCommons
  FrontendServerCommons
  Commons

  Generics
  CORBACommons
  SecToken

  boost_system
  nghttp2
)
