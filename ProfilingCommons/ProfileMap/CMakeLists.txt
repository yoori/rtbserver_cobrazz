add_library(ProfileMap SHARED

  ProfileMapFactory.cpp
  RocksDBProfileMap.cpp
)

find_package(RocksDB CONFIG REQUIRED)

if(TARGET RocksDB::rocksdb-shared)
  set(ROCKSDB_TARGET RocksDB::rocksdb-shared)
else()
  set(ROCKSDB_TARGET RocksDB::rocksdb)
endif()

target_link_libraries(ProfileMap

  Generics
  Commons
  ${ROCKSDB_TARGET}
)

#install(TARGETS ProfileMap  DESTINATION ${INSTALL_DIR})
