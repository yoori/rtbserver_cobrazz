add_executable(RocksDBBatchingProfileMapPerfTest
  RocksDBBatchingProfileMapPerfTest.cpp)

target_link_libraries(RocksDBBatchingProfileMapPerfTest
  PRIVATE
    Commons
    Generics
    ProfileMapRocksDB
)

target_include_directories(RocksDBBatchingProfileMapPerfTest
  PRIVATE
    ${CMAKE_SOURCE_DIR}
)
