#ifndef ROCKSDBWHITHLEVELALTERNATIVEPROFILEMAP_HPP
#define ROCKSDBWHITHLEVELALTERNATIVEPROFILEMAP_HPP

// UNIXCOMMONS
#include <Generics/CompositeActiveObject.hpp>
#include <UServerUtils/Grpc/RocksDB/DataBaseManagerPool.hpp>

// THIS
#include <ProfilingCommons/ProfileMap/AsyncRocksDBProfileMap.hpp>
#include <ProfilingCommons/PlainStorage3/LevelProfileMap.hpp>

namespace AdServer::ProfilingCommons
{

template<class Key, class KeyAdapter, class KeySerializer>
class RocksDBWhithLevelAlternativeProfileMap final
  : public ProfileMap<Key>,
    public Generics::CompositeActiveObject,
    public ReferenceCounting::AtomicImpl
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using DataBaseManagerPool = UServerUtils::Grpc::RocksDB::DataBaseManagerPool;
  using DataBaseManagerPoolPtr = std::shared_ptr<DataBaseManagerPool>;
  using ConstSmartMemBuf_var = Generics::ConstSmartMemBuf_var;
  using ProfileMapT = ProfileMap<Key>;
  using ProfileMap_var = ReferenceCounting::SmartPtr<ProfileMapT>;
  using RocksDBParams = ProfilingCommons::RocksDB::RocksDBParams;

public:
  RocksDBWhithLevelAlternativeProfileMap(
    Logger* logger,
    Generics::ActiveObjectCallback* callback,
    const DataBaseManagerPoolPtr& data_base_manager_pool,
    const bool is_rocksdb_enable,
    const std::string& rocksdb_path,
    const RocksDBParams& rocksdb_params,
    std::optional<std::string> rocksdb_column_family_name,
    const KeyAdapter& key_adapter,
    const bool is_level_enable,
    const char* level_directory,
    const char* level_file_prefix,
    const LevelMapTraits& level_traits,
    LoadingProgressCallbackBase* progress_checker_parent = nullptr);

  void wait_preconditions(const Key&, OperationPriority) const override;

  bool check_profile(const Key& key) const override;

  Generics::ConstSmartMemBuf_var get_profile(
    const Key& key,
    Generics::Time* last_access_time = 0) override;

  void save_profile(
    const Key& key,
    const Generics::ConstSmartMemBuf* mem_buf,
    const Generics::Time& now = Generics::Time::get_time_of_day(),
    OperationPriority op_priority = OP_RUNTIME) override;

  bool remove_profile(
    const Key& key,
    OperationPriority op_priority = OP_RUNTIME) override;

  unsigned long size() const noexcept override;

  unsigned long area_size() const noexcept override;

private:
  ProfileMap_var rocksdb_profile_map_;

  ProfileMap_var level_profile_map_;
};

template<class Key, class KeyAdapter, class KeySerializer>
RocksDBWhithLevelAlternativeProfileMap<Key, KeyAdapter, KeySerializer>::RocksDBWhithLevelAlternativeProfileMap(
  Logger* logger,
  Generics::ActiveObjectCallback* callback,
  const DataBaseManagerPoolPtr& data_base_manager_pool,
  const bool is_rocksdb_enable,
  const std::string& rocksdb_path,
  const RocksDBParams& rocksdb_params,
  std::optional<std::string> rocksdb_column_family_name,
  const KeyAdapter& key_adapter,
  const bool is_level_enable,
  const char* level_directory,
  const char* level_file_prefix,
  const LevelMapTraits& level_traits,
  LoadingProgressCallbackBase* progress_checker_parent)
{
  if (is_rocksdb_enable)
  {
    ReferenceCounting::SmartPtr<
      ProfilingCommons::RocksDB::RocksDBProfileMap<Key, KeyAdapter>> profile_map =
        new ProfilingCommons::RocksDB::RocksDBProfileMap<Key, KeyAdapter>(
          logger,
          data_base_manager_pool,
          rocksdb_path,
          rocksdb_params,
          rocksdb_column_family_name,
          key_adapter);
    rocksdb_profile_map_ = profile_map;
  }

  if (is_level_enable)
  {
    ReferenceCounting::SmartPtr<LevelProfileMap<Key, KeySerializer>> profile_map =
      new LevelProfileMap<Key, KeySerializer>(
        callback,
        level_directory,
        level_file_prefix,
        level_traits,
        progress_checker_parent);
    add_child_object(profile_map.in());
    level_profile_map_ = profile_map;
  }
}

template<class Key, class KeyAdapter, class KeySerializer>
void
RocksDBWhithLevelAlternativeProfileMap<Key, KeyAdapter, KeySerializer>::wait_preconditions(
  const Key& key,
  OperationPriority op_priority) const
{
  if (rocksdb_profile_map_)
  {
    rocksdb_profile_map_->wait_preconditions(key, op_priority);
  }

  if (level_profile_map_)
  {
    level_profile_map_->wait_preconditions(key, op_priority);
  }
}

template<class Key, class KeyAdapter, class KeySerializer>
bool RocksDBWhithLevelAlternativeProfileMap<Key, KeyAdapter, KeySerializer>::check_profile(
  const Key& key) const
{
  bool result = false;
  if (rocksdb_profile_map_)
  {
    result = rocksdb_profile_map_->check_profile(key);
    if (result)
      return result;
  }

  if (level_profile_map_)
  {
    result = level_profile_map_->check_profile(key);
  }

  return result;
}

template<class Key, class KeyAdapter, class KeySerializer>
Generics::ConstSmartMemBuf_var
RocksDBWhithLevelAlternativeProfileMap<Key, KeyAdapter, KeySerializer>::get_profile(
  const Key& key,
  Generics::Time* last_access_time)
{
  Generics::ConstSmartMemBuf_var result;
  if (rocksdb_profile_map_)
  {
    result = rocksdb_profile_map_->get_profile(key, last_access_time);
    if (result)
      return result;
  }

  if (level_profile_map_)
  {
    result = level_profile_map_->get_profile(key, last_access_time);
  }

  return result;
}

template<class Key, class KeyAdapter, class KeySerializer>
void RocksDBWhithLevelAlternativeProfileMap<Key, KeyAdapter, KeySerializer>::save_profile(
  const Key& key,
  const Generics::ConstSmartMemBuf* mem_buf,
  const Generics::Time& now,
  OperationPriority op_priority)
{
  if (rocksdb_profile_map_)
  {
    rocksdb_profile_map_->save_profile(key, mem_buf, now, op_priority);
    return;
  }

  if (level_profile_map_)
  {
    level_profile_map_->save_profile(key, mem_buf, now, op_priority);
  }
}

template<class Key, class KeyAdapter, class KeySerializer>
bool RocksDBWhithLevelAlternativeProfileMap<Key, KeyAdapter, KeySerializer>::remove_profile(
  const Key& key,
  OperationPriority op_priority)
{
  bool res = false;
  if (rocksdb_profile_map_)
  {
    res = rocksdb_profile_map_->remove_profile(key, op_priority);
  }

  if (level_profile_map_)
  {
    res |= level_profile_map_->remove_profile(key, op_priority);
  }
  return res;
}

template<class Key, class KeyAdapter, class KeySerializer>
unsigned long RocksDBWhithLevelAlternativeProfileMap<Key, KeyAdapter, KeySerializer>::size() const noexcept
{
  unsigned long result = 0;
  if (rocksdb_profile_map_)
  {
    result += rocksdb_profile_map_->size();
  }

  if (level_profile_map_)
  {
    result += level_profile_map_->size();
  }

  return result;
}

template<class Key, class KeyAdapter, class KeySerializer>
unsigned long RocksDBWhithLevelAlternativeProfileMap<Key, KeyAdapter, KeySerializer>::area_size() const noexcept
{
  unsigned long result = 0;
  if (rocksdb_profile_map_)
  {
    result += rocksdb_profile_map_->area_size();
  }

  if (level_profile_map_)
  {
    result += level_profile_map_->area_size();
  }

  return result;
}

} // namespace AdServer::ProfilingCommons

#endif // ROCKSDBWHITHLEVELALTERNATIVEPROFILEMAP_HPP
