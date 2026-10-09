add_executable(RocksDBBatchingProcessorQueuePerfTest
  RocksDBBatchingProcessorQueuePerfTest.cpp
  SimpleRocksDBBatchingProcessorQueue.cpp)

target_link_libraries(RocksDBBatchingProcessorQueuePerfTest
  PRIVATE
    PkgConfig::Jemalloc
    Generics
    ProfileMapRocksDB
)
