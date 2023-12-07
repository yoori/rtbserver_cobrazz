#ifndef ASYNCROCKSDBPROFILEMAP_HPP_
#define ASYNCROCKSDBPROFILEMAP_HPP_

// STD
#include <optional>

// UNIXCOMMONS
#include <Generics/Uncopyable.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <UServerUtils/Grpc/RocksDB/DataBase.hpp>
#include <UServerUtils/Grpc/RocksDB/DataBaseManagerPool.hpp>

// THIS
#include "ProfileMap.hpp"

namespace AdServer::ProfilingCommons::RocksDB
{

struct RocksDBParams final
{
  explicit RocksDBParams(
    const std::uint64_t block_сache_size_mb,
    const std::optional<std::int32_t> ttl,
    const rocksdb::CompactionStyle compaction_style = rocksdb::kCompactionStyleLevel,
    const std::optional<std::size_t> number_background_threads = {})
    : block_сache_size_mb(block_сache_size_mb),
      ttl(ttl),
      compaction_style(compaction_style),
      number_background_threads(number_background_threads)
  {
  }

  ~RocksDBParams() = default;

  const std::uint64_t block_сache_size_mb;
  const std::optional<std::int32_t> ttl;
  const rocksdb::CompactionStyle compaction_style;
  const std::optional<std::size_t> number_background_threads;
};

namespace Internal
{

class ProfileMapImpl final: Generics::Uncopyable
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;

  DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

private:
  using DataBase = UServerUtils::Grpc::RocksDB::DataBase;
  using DataBasePtr = std::shared_ptr<DataBase>;
  using DataBaseManagerPool = UServerUtils::Grpc::RocksDB::DataBaseManagerPool;
  using DataBaseManagerPoolPtr = std::shared_ptr<DataBaseManagerPool>;
  using ColumnFamilyHandle = DataBaseManagerPool::ColumnFamilyHandle;
  using ReadOptions = rocksdb::ReadOptions;
  using WriteOptions = rocksdb::WriteOptions;
  using ConstSmartMemBuf_var = Generics::ConstSmartMemBuf_var;
  using Time = Generics::Time;

public:
  explicit ProfileMapImpl(
    Logger* logger,
    const DataBaseManagerPoolPtr& db_manager_pool,
    const std::string& db_path,
    const RocksDBParams& rocksdb_params,
    std::optional<std::string> column_family_name = {});

  ~ProfileMapImpl() = default;

  bool check_profile(const std::string_view key) const;

  ConstSmartMemBuf_var get_profile(
    const std::string_view key);

  void save_profile(
    const std::string_view key,
    const Generics::ConstSmartMemBuf* profile);

  bool remove_profile(
    const std::string_view key);

private:
  Logger_var logger_;

  const std::string db_path_;

  ColumnFamilyHandle* column_family_handle_ = nullptr;

  ReadOptions read_options_;

  WriteOptions write_options_;

  DataBasePtr data_base_;

  DataBaseManagerPoolPtr db_manager_pool_;
};

} // namespace Internal

struct DefaultKeyAdapter
{
  template<typename Key>
  std::string operator()(const Key& key)
  {
    return key;
  }
};

template<class Key, class KeyAdapter = DefaultKeyAdapter>
class RocksDBProfileMap final:
  public ProfileMap<Key>,
  public ReferenceCounting::AtomicImpl
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using DataBaseManagerPool = UServerUtils::Grpc::RocksDB::DataBaseManagerPool;
  using DataBaseManagerPoolPtr = std::shared_ptr<DataBaseManagerPool>;
  using ConstSmartMemBuf_var = Generics::ConstSmartMemBuf_var;

public:
  explicit RocksDBProfileMap(
    Logger* logger,
    const DataBaseManagerPoolPtr& db_manager_pool,
    const std::string& db_path,
    const RocksDBParams& rocksdb_params,
    std::optional<std::string> column_family_name = {},
    const KeyAdapter& key_adapter = KeyAdapter())
    : key_adapter_(key_adapter),
      impl_(
        logger,
        db_manager_pool,
        db_path,
        rocksdb_params,
        column_family_name)
  {
  }

  ~RocksDBProfileMap() override = default;

  bool check_profile(const Key& key) const override
  {
    return impl_.check_profile(key_adapter_(key));
  }

  ConstSmartMemBuf_var get_profile(
    const Key& key,
    Generics::Time* /*last_access_time*/) override
  {
    return impl_.get_profile(key_adapter_(key));
  }

  void save_profile(
    const Key& key,
    const Generics::ConstSmartMemBuf* profile,
    const Generics::Time& /*now*/,
    OperationPriority /*op_priority*/) override
  {
    impl_.save_profile(key_adapter_(key), profile);
  }

  bool remove_profile(
    const Key& key,
    OperationPriority /*op_priority*/) override
  {
    return impl_.remove_profile(key_adapter_(key));
  }

  unsigned long size() const noexcept override
  {
    return 1;
  }

  unsigned long area_size() const noexcept override
  {
    return 1;
  }

private:
  mutable KeyAdapter key_adapter_;

  Internal::ProfileMapImpl impl_;
};

} // namespace AdServer::ProfilingCommons::RocksDB

#endif // ASYNCROCKSDBPROFILEMAP_HPP_