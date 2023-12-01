include_directories (
  ${rocksdb_SOURCE_DIR}/include
)

add_library(ProfileMap SHARED
  AsyncRocksDBProfileMap.cpp
  ProfileMapFactory.cpp
  RocksDBProfileMap.cpp
)

target_link_libraries(ProfileMap
  Commons
  Generics
  rocksdb
  uring
  UServerUtils
)