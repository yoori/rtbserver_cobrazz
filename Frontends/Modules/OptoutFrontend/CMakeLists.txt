add_library(OptoutFrontend SHARED
  OptoutFrontend.cpp
  OptoutFrontendStat.cpp
  RequestInfoFiller.cpp
)

target_link_libraries(OptoutFrontend
  Stream
  CORBACommons
  Commons
  PrivacyFilter
  RequestMatchers
  FrontendStatistic
  CommonModule
  CampaignManagerGrpcDistributedClient
  FeConfigXSD
  FCGI
)
