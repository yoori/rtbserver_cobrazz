#pragma once

#include <rocksdb/options.h>

namespace AdServer::ProfilingCommons
{
  void
  configure_rocksdb_profile_map_options(rocksdb::Options& options);
}
