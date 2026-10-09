add_executable(RocksDBBatchingProcessorQueueTest
  RocksDBBatchingProcessorQueueTest.cpp)

target_link_libraries(RocksDBBatchingProcessorQueueTest
  PRIVATE
    Generics
    ProfileMapRocksDB
)
