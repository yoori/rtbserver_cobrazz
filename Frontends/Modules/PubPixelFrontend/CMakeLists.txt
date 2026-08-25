add_library(PubPixelFrontend SHARED
  RequestInfoFiller.cpp
  PubPixelFrontend.cpp
)

target_link_libraries(PubPixelFrontend
  PRIVATE
    Commons
    CampaignManagerGrpcDistributedClient
    FeConfigXSD
    FCGI
)
