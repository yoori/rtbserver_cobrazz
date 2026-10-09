add_executable(RefPoolTest
  RefPoolTest.cpp
)

add_executable(DistributedPartitionPoolTest
  DistributedPartitionPoolTest.cpp
)

add_executable(BatchPartialResponseTest
  BatchPartialResponseTest.cpp
)

add_executable(BatchingTimerTest
  BatchingTimerTest.cpp
)

add_executable(DeferredClientDeletionTest DeferredClientDeletionTest.cpp)
target_link_libraries(DeferredClientDeletionTest PRIVATE Commons Generics GrpcBatch Logger)

add_executable(DrainCallbackLifetimeTest DrainCallbackLifetimeTest.cpp)
target_link_libraries(DrainCallbackLifetimeTest PRIVATE Commons Generics GrpcBatch)

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
