add_library(OptoutFrontend SHARED
  OptoutFrontend.cpp
  OptoutFrontendStat.cpp
  RequestInfoFiller.cpp
)

target_link_libraries(OptoutFrontend
  PRIVATE
  Commons
  PrivacyFilter
  RequestMatchers
  CommonModule
  CampaignManagerGrpcDistributedClient
  FeConfigXSD
  FCGI
)
