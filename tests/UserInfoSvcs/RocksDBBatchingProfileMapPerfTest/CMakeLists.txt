add_executable(RocksDBBatchingProfileMapPerfTest
  RocksDBBatchingProfileMapPerfTest.cpp)

target_link_libraries(RocksDBBatchingProfileMapPerfTest
  PRIVATE
    Commons
    Generics
    ProfileMapRocksDB
)
