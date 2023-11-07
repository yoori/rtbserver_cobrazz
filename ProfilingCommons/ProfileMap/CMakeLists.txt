add_library(ProfileMap SHARED
  ProfileMapFactory.cpp
  RocksDBProfileMap.cpp
)

target_link_libraries(ProfileMap
  Commons
  Generics
  rocksdb
  uring
)