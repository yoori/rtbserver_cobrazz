#ifndef ROCKSDBWHITHALTERNATIVEPROFILEMAP_HPP
#define ROCKSDBWHITHALTERNATIVEPROFILEMAP_HPP

// UNIXCOMMONS
#include <eh/Exception.hpp>
#include <UServerUtils/RocksDB/DataBaseManagerPool.hpp>

// THIS
#include <ProfilingCommons/ProfileMap/AsyncRocksDBProfileMap.hpp>

namespace AdServer::ProfilingCommons
{

template<class Key, class KeyAdapter>
class RocksDBWhithAlternativeProfileMap final
  : public ProfileMap<Key>,
    public ReferenceCounting::AtomicImpl
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using DataBaseManagerPool = UServerUtils::Grpc::RocksDB::DataBaseManagerPool;
  using DataBaseManagerPoolPtr = std::shared_ptr<DataBaseManagerPool>;
  using ConstSmartMemBuf_var = Generics::ConstSmartMemBuf_var;
  using ProfileMap_var = ReferenceCounting::SmartPtr<ProfileMap<Key>>;
  using RocksDBParams = ProfilingCommons::RocksDB::RocksDBParams;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

public:
  explicit RocksDBWhithAlternativeProfileMap(
    Logger* logger,
    const DataBaseManagerPoolPtr& data_base_manager_pool,
    const bool is_rocksdb_enable,
    const std::string& rocksdb_path,
    const RocksDBParams& rocksdb_params,
    const std::optional<std::string> rocksdb_column_family_name,
    const KeyAdapter& key_adapter,
    ProfileMap<Key>* other_profile_map);

  ~RocksDBWhithAlternativeProfileMap() override = default;

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

  ProfileMap_var other_profile_map_;
};

template<class Key, class KeyAdapter>
RocksDBWhithAlternativeProfileMap<Key, KeyAdapter>::RocksDBWhithAlternativeProfileMap(
  Logger* logger,
  const DataBaseManagerPoolPtr& data_base_manager_pool,
  const bool is_rocksdb_enable,
  const std::string& rocksdb_path,
  const RocksDBParams& rocksdb_params,
  const std::optional<std::string> rocksdb_column_family_name,
  const KeyAdapter& key_adapter,
  ProfileMap<Key>* other_profile_map)
  : other_profile_map_(ReferenceCounting::add_ref(other_profile_map))
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

  if (rocksdb_profile_map_.in() == nullptr && other_profile_map_.in() == nullptr)
  {
    Stream::Error stream;
    stream << FNS
           << "All profile maps is null";
    throw Exception(stream);
  }
}

template<class Key, class KeyAdapter>
void
RocksDBWhithAlternativeProfileMap<Key, KeyAdapter>::wait_preconditions(
  const Key& key,
  OperationPriority op_priority) const
{
  if (rocksdb_profile_map_)
  {
    rocksdb_profile_map_->wait_preconditions(key, op_priority);
  }

  if (other_profile_map_)
  {
    other_profile_map_->wait_preconditions(key, op_priority);
  }
}

template<class Key, class KeyAdapter>
bool RocksDBWhithAlternativeProfileMap<Key, KeyAdapter>::check_profile(
  const Key& key) const
{
  bool result = false;
  if (rocksdb_profile_map_)
  {
    result = rocksdb_profile_map_->check_profile(key);
    if (result)
      return result;
  }

  if (other_profile_map_)
  {
    result = other_profile_map_->check_profile(key);
  }

  return result;
}

template<class Key, class KeyAdapter>
Generics::ConstSmartMemBuf_var
RocksDBWhithAlternativeProfileMap<Key, KeyAdapter>::get_profile(
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

  if (other_profile_map_)
  {
    result = other_profile_map_->get_profile(key, last_access_time);
  }

  return result;
}

template<class Key, class KeyAdapter>
void RocksDBWhithAlternativeProfileMap<Key, KeyAdapter>::save_profile(
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

  if (other_profile_map_)
  {
    other_profile_map_->save_profile(key, mem_buf, now, op_priority);
  }
}

template<class Key, class KeyAdapter>
bool RocksDBWhithAlternativeProfileMap<Key, KeyAdapter>::remove_profile(
  const Key& key,
  OperationPriority op_priority)
{
  if (rocksdb_profile_map_)
  {
    rocksdb_profile_map_->remove_profile(key, op_priority);
  }

  if (other_profile_map_)
  {
    other_profile_map_->remove_profile(key, op_priority);
  }
}

template<class Key, class KeyAdapter>
unsigned long RocksDBWhithAlternativeProfileMap<Key, KeyAdapter>::size() const noexcept
{
  unsigned long result = 0;
  if (rocksdb_profile_map_)
  {
    result += rocksdb_profile_map_->size();
  }

  if (other_profile_map_)
  {
    result += other_profile_map_->size();
  }

  return result;
}

template<class Key, class KeyAdapter>
unsigned long RocksDBWhithAlternativeProfileMap<Key, KeyAdapter>::area_size() const noexcept
{
  unsigned long result = 0;
  if (rocksdb_profile_map_)
  {
    result += rocksdb_profile_map_->area_size();
  }

  if (other_profile_map_)
  {
    result += other_profile_map_->area_size();
  }

  return result;
}

} // namespace AdServer::ProfilingCommons

#endif // ROCKSDBWHITHALTERNATIVEPROFILEMAP_HPP
