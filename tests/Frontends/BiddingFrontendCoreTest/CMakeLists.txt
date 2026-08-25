add_executable(BiddingFrontendCoreTest
  BiddingFrontendCoreTest.cpp)

target_link_libraries(BiddingFrontendCoreTest
  PRIVATE
    PkgConfig::Jemalloc
    BiddingFrontend
    CampaignServerIDL
    Commons
    CommonModule
    CORBACommons
    CampaignManagerGrpcClient
    ChannelServerGrpcClient
    FrontendServerCommons
    Generics
    HTTP
    Logger
    ProcessControlIDL
    Sync
    UserBindServerGrpcClient
    UserInfoManagerGrpcClient
    UserIdController
)

target_include_directories(BiddingFrontendCoreTest
  PRIVATE
    ${CMAKE_SOURCE_DIR}
)
