#include "RocksDBOptions.hpp"

#include <rocksdb/cache.h>
#include <rocksdb/filter_policy.h>
#include <rocksdb/table.h>

#include <cstddef>
#include <cstdint>
#include <memory>

namespace AdServer::ProfilingCommons
{
  namespace
  {
    constexpr std::size_t BLOCK_CACHE_SIZE = 1024ULL * 1024 * 1024;
    constexpr std::uint64_t BLOCK_SIZE = 8 * 1024;
    constexpr std::size_t WRITE_BUFFER_SIZE = 256ULL * 1024 * 1024;
    constexpr std::uint64_t TARGET_FILE_SIZE_BASE = 64ULL * 1024 * 1024;
    constexpr std::uint64_t SYNC_EVERY_BYTES = 1024ULL * 1024;
    constexpr double BLOOM_BITS_PER_KEY = 10.0;

    std::shared_ptr<rocksdb::Cache> block_cache()
    {
      static std::shared_ptr<rocksdb::Cache> cache = rocksdb::NewLRUCache(
        BLOCK_CACHE_SIZE,
        -1,
        false,
        0.25);

      return cache;
    }
  }

  void
  configure_rocksdb_profile_map_options(rocksdb::Options& options)
  {
    options.IncreaseParallelism();
    options.OptimizeLevelStyleCompaction();
    options.create_if_missing = true;
    options.compression = rocksdb::kNoCompression;
    options.target_file_size_multiplier = 2;

    options.write_buffer_size = WRITE_BUFFER_SIZE;
    options.max_write_buffer_number = 4;
    options.target_file_size_base = TARGET_FILE_SIZE_BASE;
    options.max_background_jobs = 4;
    options.bytes_per_sync = SYNC_EVERY_BYTES;
    options.wal_bytes_per_sync = SYNC_EVERY_BYTES;

    rocksdb::BlockBasedTableOptions table_options;
    table_options.block_cache = block_cache();
    table_options.block_size = BLOCK_SIZE;
    table_options.cache_index_and_filter_blocks = true;
    table_options.cache_index_and_filter_blocks_with_high_priority = true;
    table_options.pin_l0_filter_and_index_blocks_in_cache = true;
    table_options.index_type = rocksdb::BlockBasedTableOptions::kTwoLevelIndexSearch;
    table_options.partition_filters = true;
    table_options.filter_policy.reset(rocksdb::NewBloomFilterPolicy(BLOOM_BITS_PER_KEY));
    table_options.whole_key_filtering = true;
    table_options.checksum = rocksdb::kNoChecksum;

    options.table_factory.reset(rocksdb::NewBlockBasedTableFactory(table_options));
  }
}
