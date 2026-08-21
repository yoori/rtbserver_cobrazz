add_subdirectory(UserBindPerf)
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
    TestCommons
    Generics
    Commons
    ProfileMap
    ProfileMapRocksDB
)

target_include_directories(UserBindContainerTest
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)
