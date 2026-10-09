add_subdirectory(UserBindPerf)
add_subdirectory(BindRequestRocksDBTest)
add_subdirectory(UserBindArenaFillTest)
add_subdirectory(UserBindServerCorePerfTest)
add_subdirectory(UserInfoManagerCorePerfTest)
add_subdirectory(RocksDBProfileMapTest)
add_subdirectory(RocksDBBatchingProcessorQueueTest)
add_subdirectory(RocksDBBatchingProcessorQueuePerfTest)
add_subdirectory(RocksDBProfileMapProcessorTest)
add_subdirectory(RocksDBBatchingProfileMapPerfTest)

add_executable(UserBindContainerTest
  ../UnitTests/UserInfoSvcs/UserBindContainerTest/UserBindContainerTest.cpp
)

target_link_libraries(UserBindContainerTest
  PRIVATE
    UserBindContainer
    Generics
    Commons
    Logger
    ProfileMap
    ProfileMapRocksDB
    Sync
)
