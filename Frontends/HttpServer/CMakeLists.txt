add_executable(HttpServer
  Application.cpp
  Handler.cpp
  HttpResponse.cpp
  Main.cpp
)

target_link_libraries(HttpServer
  ActionFrontend
  AdFrontend
  AdInstFrontend
  AdServerCommonsXSD
  BidRateProvider
  BiddingFrontend
  CORBACommons
  CTRProvider
  CampaignConfig
  CampaignConfigSource
  CampaignIndex
  CampaignManagerIDL
  CampaignManagerLogger
  ClickFrontend
  Commons
  ContentFrontend
  CreativeTemplateArgs
  DirectoryModule
  EchoFrontend
  FrontendsPool
  Generics
  GrpcCampaignManagerPool
  HTTPServerConfigXSD
  ImprTrackFrontend
  OptoutFrontend
  PassFrontend
  PassPixelFrontend
  ProcessControlVars
  PubPixelFrontend
  RequestMatchers
  SecToken
  UServerUtils
  UserBindFrontend
  UserInfoClusterControlIDL
  UserInfoManagerControllerIDL
  UserInfoManagerControllerL
  UtilCommons
  WebStatFrontend
  XMLUtility
)