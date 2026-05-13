add_library(ProfileMap SHARED

  ProfileMapIOUring.cpp
  ProfileMapFactory.cpp
)

target_link_libraries(ProfileMap

 Generics
 Commons
)

find_package(RocksDB CONFIG REQUIRED)
find_package(ToplingDB CONFIG QUIET)
find_library(JEMALLOC_LIBRARY NAMES jemalloc REQUIRED)

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

if(ToplingDB_FOUND)
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
endif()

#install(TARGETS ProfileMap  DESTINATION ${INSTALL_DIR})
