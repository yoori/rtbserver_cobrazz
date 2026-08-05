add_executable(RocksDBProfileMapProcessorTest
  RocksDBProfileMapProcessorTest.cpp)

target_link_libraries(RocksDBProfileMapProcessorTest
  PRIVATE
    Generics
    Commons
    ProfileMap
    ProfileMapRocksDB
)

target_include_directories(RocksDBProfileMapProcessorTest
  PRIVATE
    ${CMAKE_SOURCE_DIR}
)
