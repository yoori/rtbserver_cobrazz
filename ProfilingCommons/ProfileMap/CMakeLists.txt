add_library(ProfileMap SHARED

  ProfileMapIOUring.cpp
  ProfileMapFactory.cpp
)

target_link_libraries(ProfileMap

 Generics
 Commons
)

find_package(RocksDB CONFIG REQUIRED)
find_package(ToplingDB CONFIG REQUIRED)
find_library(JEMALLOC_LIBRARY NAMES jemalloc REQUIRED)

add_library(ProfileMapRocksDB SHARED
  RocksDBBatchingProfileMap.cpp
  RocksDBProfileMap.cpp
)

target_link_libraries(ProfileMapRocksDB
  ProfileMap
  Generics
  Commons
  RocksDB::rocksdb-shared
)

add_library(ProfileMapToplingDB SHARED
  ToplingDBProfileMap.cpp
)

target_link_libraries(ProfileMapToplingDB
  ProfileMap
  Generics
  Commons
  ToplingDB::rocksdb
  ${JEMALLOC_LIBRARY}
)

#install(TARGETS ProfileMap  DESTINATION ${INSTALL_DIR})
