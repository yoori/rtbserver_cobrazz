find_package(RocksDB CONFIG REQUIRED)

if (TARGET RocksDB::rocksdb-shared)
  set(ROCKSDB_TARGET RocksDB::rocksdb-shared)
else()
  set(ROCKSDB_TARGET RocksDB::rocksdb)
endif()

add_executable(RocksDbMergeUtil
  Application.cpp
)

target_link_libraries(RocksDbMergeUtil
  PRIVATE
    Boost::headers
    ${ROCKSDB_TARGET}
)
