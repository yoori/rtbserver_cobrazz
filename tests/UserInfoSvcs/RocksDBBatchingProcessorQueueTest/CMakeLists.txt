add_executable(RocksDBBatchingProcessorQueueTest
  RocksDBBatchingProcessorQueueTest.cpp)

target_link_libraries(RocksDBBatchingProcessorQueueTest
  PRIVATE
    Generics
    Commons
    ProfileMapRocksDB
)

target_include_directories(RocksDBBatchingProcessorQueueTest
  PRIVATE
    ${CMAKE_SOURCE_DIR}
)
