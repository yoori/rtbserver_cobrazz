add_library(FrontendServerCommons
  FCGIAcceptor.cpp
  Http2Acceptor.cpp
  WorkerStatsObject.cpp
  Http2Acceptor.cpp
)

target_link_libraries(FrontendServerCommons
  FCGI
)

add_executable(FCGIServer
  FCGIServer.cpp
  FrontendsPool.cpp
)

target_link_libraries(FCGIServer
  jemalloc

  RequestMatchers
  CreativeTemplateArgs

  CampaignManagerIDL
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
  XMLUtility
  UtilCommons
  FrontendServerCommons
  Commons
  HttpServer

  Generics
  SecToken

  boost_system
  nghttp2
)
