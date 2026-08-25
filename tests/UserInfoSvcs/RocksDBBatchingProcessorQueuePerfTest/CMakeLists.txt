add_executable(RocksDBBatchingProcessorQueuePerfTest
  RocksDBBatchingProcessorQueuePerfTest.cpp
  SimpleRocksDBBatchingProcessorQueue.cpp)

target_link_libraries(RocksDBBatchingProcessorQueuePerfTest
  PRIVATE
    jemalloc
    Generics
    ProfileMapRocksDB
)

target_include_directories(RocksDBBatchingProcessorQueuePerfTest
  PRIVATE
    ${CMAKE_SOURCE_DIR}
)
