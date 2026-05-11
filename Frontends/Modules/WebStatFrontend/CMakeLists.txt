add_library(WebStatFrontend SHARED
  RequestInfoFiller.cpp
  WebStatFrontend.cpp
)

target_link_libraries(WebStatFrontend
  Stream
  Commons
  RequestMatchers
  CommonModule
  CampaignManagerClient
  FeConfigXSD
  FCGI
)
