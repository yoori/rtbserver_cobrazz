#pragma once

#include <rocksdb/options.h>

namespace AdServer::ProfilingCommons
{
  void
  configure_rocksdb_profile_map_options(
    rocksdb::Options& options,
    int compaction_threads = 32,
    int per_compaction_threads = 4,
    int flush_threads = 32,
    int max_mem_tables = 6);
}
