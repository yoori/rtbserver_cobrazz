add_library(PassPixelFrontend SHARED
  RequestInfoFiller.cpp
  PassPixelFrontend.cpp
)

target_link_libraries(PassPixelFrontend
  PRIVATE
    Geoip
    Commons
    CommonModule
    CampaignManagerGrpcDistributedClient
    FeConfigXSD
    FCGI
)
