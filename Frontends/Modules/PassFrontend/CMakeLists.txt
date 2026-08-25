add_library(PassFrontend SHARED
  RequestInfoFiller.cpp
  PassFrontend.cpp
)

target_link_libraries(PassFrontend
  PRIVATE
  Commons
  UserInfoClient
  CommonModule
  CampaignManagerGrpcDistributedClient
  FeConfigXSD
  FCGI
)
