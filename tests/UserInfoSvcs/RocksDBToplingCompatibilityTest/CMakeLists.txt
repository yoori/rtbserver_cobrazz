add_executable(RocksDBToplingCompatibilityTest
  RocksDBToplingCompatibilityTest.cpp)

target_link_libraries(RocksDBToplingCompatibilityTest
  PRIVATE
    Generics
    Commons
    ProfileMap
    ProfileMapRocksDB
    ProfileMapToplingDB
)

target_include_directories(RocksDBToplingCompatibilityTest
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..
)
