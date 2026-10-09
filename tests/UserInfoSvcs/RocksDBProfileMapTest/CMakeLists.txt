add_executable(RocksDBProfileMapTest
  RocksDBProfileMapTest.cpp)

target_link_libraries(RocksDBProfileMapTest
  PRIVATE
    Generics
    ProfileMapRocksDB
)
