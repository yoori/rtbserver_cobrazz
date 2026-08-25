add_library(ProfileMap SHARED
  ProfileMapFactory.cpp
)

target_link_libraries(ProfileMap
  PRIVATE
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
  RocksDBBatchingProcessorQueue.cpp
  RocksDBBatchingProfileMap.cpp
  RocksDBOptions.cpp
  RocksDBProfileMap.cpp
  RocksDBProfileMapProcessor.cpp
)

target_link_libraries(ProfileMapRocksDB
  PRIVATE
    Generics
    Commons
    ${ROCKSDB_TARGET}
)
