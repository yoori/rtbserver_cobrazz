add_library(ClickFrontend SHARED
  RequestInfoFiller.cpp
  ClickRequestState.cpp
  ClickFrontend.cpp
)

target_link_libraries(ClickFrontend
  PRIVATE
    Geoip
    Commons
    ChannelGrpcClient
    UserInfoClient
    UserInfoManagerGrpcClient
    UserBindClient
    UserIdController
    CommonModule
    CampaignManagerGrpcDistributedClient
    FeConfigXSD
    FCGI
)
