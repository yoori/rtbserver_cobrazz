add_library(OptoutFrontend SHARED
  OptoutFrontend.cpp 
  OptoutFrontendStat.cpp 
  RequestInfoFiller.cpp
)

target_link_libraries(OptoutFrontend
  Stream
  Geoip
  CORBACommons
  Polyglot
  Commons
  ChannelManagerControllerIDL
  UserInfoManagerControllerIDL UserInfoClusterControlIDL UserInfoManagerControllerL
  CampaignManagerIDL
  SecToken
  DomainParser
  RequestMatchers
  FrontendStatistic
  UserIdController
  CommonModule
  FeConfigXSD
  FCGI
)

#install(TARGETS OptoutFrontend  DESTINATION ${INSTALL_DIR})
