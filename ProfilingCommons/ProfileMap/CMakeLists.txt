add_library(ProfileMap SHARED
  ProfileMapFactory.cpp
)

target_link_libraries(ProfileMap

 Generics
 Commons
)

find_package(RocksDB CONFIG REQUIRED)

if(TARGET RocksDB::rocksdb-shared)
  set(ROCKSDB_TARGET RocksDB::rocksdb-shared)
else()
  set(ROCKSDB_TARGET RocksDB::rocksdb)
endif()

add_library(ProfileMapRocksDB SHARED
  RocksDBBatchingProfileMap.cpp
  RocksDBProfileMap.cpp
)

target_link_libraries(ProfileMapRocksDB
  ProfileMap
  Generics
  Commons
  ${ROCKSDB_TARGET}
)

#install(TARGETS ProfileMap  DESTINATION ${INSTALL_DIR})
