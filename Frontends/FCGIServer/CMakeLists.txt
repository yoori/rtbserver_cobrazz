add_executable(FCGIServer
  FCGIServer.cpp 
  Acceptor.cpp 
  FrontendsPool.cpp 
  WorkerStatsObject.cpp 
  AcceptorBoostAsio.cpp 
  BoostAsioContextRunActiveObject.cpp
)
target_link_libraries(FCGIServer
 Generics
 Commons
 AdServerCommonsXSD
 XMLUtility
 CORBACommons
 ProcessControlVars
 RequestMatchers
 #CampaignManagerStubs
 CampaignManagerIDL BidRateProvider CTRProvider CampaignConfig CampaignConfigSource CampaignIndex CreativeTemplateArgs SecToken  CampaignManagerLogger
# UserInfoManagerControllerStubs
 UserInfoManagerControllerIDL UserInfoClusterControlIDL  UserInfoManagerControllerL
 FCGIServerConfigXSD
 UtilCommons
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
 boost_system

)

#install(TARGETS FCGIServer  DESTINATION ${INSTALL_DIR})