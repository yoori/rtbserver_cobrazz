add_executable(HttpServer
  Application.cpp
  Handler.cpp
  HttpResponse.cpp
  Main.cpp
)

target_link_libraries(HttpServer
  # modules
  ActionFrontend
  AdFrontend
  BiddingFrontend
  ClickFrontend
  AdInstFrontend
  WebStatFrontend
  UserBindFrontend
  PubPixelFrontend
  ImprTrackFrontend
  OptoutFrontend
  PassFrontend
  PassPixelFrontend
  EchoFrontend
  ContentFrontend
  DirectoryModule
  # modules with grpc impl
  AdFrontendGrpc
  BiddingFrontendGrpc
  # dependencies
  FrontendsPool
  RequestMatchers
  CORBACommons
  GrpcCampaignManagerPool
  CampaignManagerIDL
  Commons
  Generics
  ProcessControlVars
  SecToken
  UServerUtils
  UserInfoClusterControlIDL
  UserInfoManagerControllerIDL
  UserInfoManagerControllerL
  UtilCommons
  XMLUtility
  HTTPServerConfigXSD
  AdServerCommonsXSD
)
