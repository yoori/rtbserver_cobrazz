add_executable(RefPoolTest
  RefPoolTest.cpp
)

add_executable(DistributedPartitionPoolTest
  DistributedPartitionPoolTest.cpp
)

add_executable(BatchPartialResponseTest
  BatchPartialResponseTest.cpp
)

add_executable(BatchCooperativeTest BatchCooperativeTest.cpp)
target_link_libraries(BatchCooperativeTest PRIVATE PkgConfig::Jemalloc Commons Generics GrpcBatch)

add_executable(BatchingTimerTest
  BatchingTimerTest.cpp
)

add_executable(BatchRequestBufferTest BatchRequestBufferTest.cpp)
target_link_libraries(BatchRequestBufferTest PRIVATE GrpcBatch)

add_executable(ServiceMicroStringTest ServiceMicroStringTest.cpp)
target_link_libraries(ServiceMicroStringTest PRIVATE CampaignManagerGrpc ChannelServerGrpc)

add_executable(DeferredClientDeletionTest DeferredClientDeletionTest.cpp)
add_executable(DbAccessTest DbAccessTest.cpp)
target_link_libraries(DbAccessTest PRIVATE GrpcBatch)
target_link_libraries(DeferredClientDeletionTest PRIVATE Commons Generics GrpcBatch Logger)

add_executable(CampaignServerGrpcTest CampaignServerGrpcTest.cpp)
target_link_libraries(CampaignServerGrpcTest PRIVATE CampaignServerGrpc Generics Logger)

add_executable(CampaignConfigGrpcTest CampaignConfigGrpcTest.cpp)
target_link_libraries(CampaignConfigGrpcTest
  PRIVATE CampaignServerGrpc CampaignTypes ExpressionChannel Generics)

add_executable(DrainCallbackLifetimeTest DrainCallbackLifetimeTest.cpp)
target_link_libraries(DrainCallbackLifetimeTest PRIVATE Commons Generics GrpcBatch)

add_executable(InflightLimiterTest InflightLimiterTest.cpp)
target_link_libraries(InflightLimiterTest PRIVATE Commons Generics GrpcBatch)
target_link_options(InflightLimiterTest PRIVATE
  "-Wl,--wrap=_ZNSt18condition_variable4waitERSt11unique_lockISt5mutexE"
  "-Wl,--wrap=pthread_mutex_lock"
)

target_link_libraries(RefPoolTest
  PRIVATE
    Commons
    Generics
)

target_link_libraries(DistributedPartitionPoolTest
  PRIVATE
    Commons
    Generics
    Logger
    Sync
)

target_link_libraries(BatchPartialResponseTest
  PRIVATE
    Commons
    Generics
    GrpcBatch
)

target_link_libraries(BatchingTimerTest PRIVATE Commons Generics GrpcBatch)
