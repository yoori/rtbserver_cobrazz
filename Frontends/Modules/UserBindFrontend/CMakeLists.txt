add_library(UserBindFrontend SHARED
  BindRequestState.cpp
  UserBindMatchRequestState.cpp
  UserBindFrontend.cpp
  RequestInfoFiller.cpp
)

target_link_libraries(UserBindFrontend
  PRIVATE
    Geoip
    Commons
    ChannelGrpcClient
    UserInfoClient
    UserIdController
    CommonModule
    CampaignManagerGrpcDistributedClient
    FeConfigXSD
    UserBindClient
    FCGI
)
