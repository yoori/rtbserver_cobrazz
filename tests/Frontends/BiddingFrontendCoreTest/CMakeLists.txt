add_executable(BiddingFrontendCoreTest
  BiddingFrontendCoreTest.cpp)

add_executable(BiddingFrontendRequestPackingTest BiddingFrontendRequestPackingTest.cpp)
target_link_libraries(BiddingFrontendRequestPackingTest
  PRIVATE
    BiddingFrontend
    Commons
    CommonModule
    Generics
    Logger
    UserIdController
    CampaignManagerGrpcClient
    ChannelServerGrpcClient
    UserInfoManagerGrpcClient
    UserBindServerGrpcClient
)

target_link_libraries(BiddingFrontendCoreTest
  PRIVATE
    PkgConfig::Jemalloc
    BiddingFrontend
    Commons
    CommonModule
    CampaignManagerGrpcClient
    ChannelServerGrpcClient
    FrontendServerCommons
    Generics
    HTTP
    Logger
    Sync
    UserBindServerGrpcClient
    UserInfoManagerGrpcClient
    UserIdController
)
