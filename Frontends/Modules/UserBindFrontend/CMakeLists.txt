add_library(UserBindFrontend SHARED
  UserBindFrontend.cpp
  RequestInfoFiller.cpp
)

target_link_libraries(UserBindFrontend
  Stream
  Geoip
  CORBACommons
  Commons
  UserIdController
  CommonModule
  CampaignManagerClient
  FeConfigXSD
  UserBindClient
  FCGI
)
