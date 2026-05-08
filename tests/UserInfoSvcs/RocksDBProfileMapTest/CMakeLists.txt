add_executable(RocksDBProfileMapTest
  RocksDBProfileMapTest.cpp)

target_link_libraries(RocksDBProfileMapTest
  PRIVATE
    Generics
    Commons
    ProfileMap
    ProfileMapRocksDB
)

target_include_directories(RocksDBProfileMapTest
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..
)
