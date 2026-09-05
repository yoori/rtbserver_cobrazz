add_executable(RocksDBProfileMapProcessorTest
  RocksDBProfileMapProcessorTest.cpp)

target_link_libraries(RocksDBProfileMapProcessorTest
  PRIVATE
    Commons
    Generics
    ProfileMapRocksDB
)

target_include_directories(RocksDBProfileMapProcessorTest
  PRIVATE
    ${CMAKE_SOURCE_DIR}
)
