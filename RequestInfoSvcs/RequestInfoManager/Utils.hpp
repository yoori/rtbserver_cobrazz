#ifndef USERINFOMANAGER_UTILS_HPP
#define USERINFOMANAGER_UTILS_HPP

// THIS
#include <ProfilingCommons/ProfileMap/AsyncRocksDBProfileMap.hpp>
#include <xsd/AdServerCommons/AdServerCommons.hpp>

namespace AdServer::UserInfoSvcs
{

AdServer::ProfilingCommons::RocksDB::RocksDBParams
fill_rocksdb_map_params(
const xsd::AdServer::Configuration::ChunksRocksDBConfigType& chunks_config) noexcept
{
  using RocksDBCompactionStyleType = ::xsd::AdServer::Configuration::RocksDBCompactionStyleType;
  const auto compaction_style_config = chunks_config.compaction_style();
  rocksdb::CompactionStyle compaction_style = rocksdb::kCompactionStyleLevel;
  if (compaction_style_config == RocksDBCompactionStyleType::value::kCompactionStyleLevel)
  {
    compaction_style = rocksdb::kCompactionStyleLevel;
  }
  else if (compaction_style_config == RocksDBCompactionStyleType::value::kCompactionStyleFIFO)
  {
    compaction_style = rocksdb::kCompactionStyleFIFO;
  }

  return AdServer::ProfilingCommons::RocksDB::RocksDBParams(
    chunks_config.block_cache_size_mb(),
    chunks_config.expire_time(),
    compaction_style);
}

} // namespace AdServer::UserInfoSvcs

#endif //USERINFOMANAGER_UTILS_HPP
