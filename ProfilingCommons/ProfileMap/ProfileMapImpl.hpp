#ifndef PROFILEMAPIMPL_HPP_
#define PROFILEMAPIMPL_HPP_

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

namespace AdServer::ProfilingCommons::IoUring
{

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
  using DataBaseManagerPoolPtr = std::unique_ptr<DataBaseManagerPool>;
  using ManagerPoolConfig = UServerUtils::Grpc::RocksDB::Config;
  using ColumnFamilyHandle = DataBaseManagerPool::ColumnFamilyHandle;
  using ReadOptions = rocksdb::ReadOptions;
  using WriteOptions = rocksdb::WriteOptions;
  using ConstSmartMemBuf_var = Generics::ConstSmartMemBuf_var;
  using Time = Generics::Time;

public:
  explicit ProfileMapImpl(
    Logger* logger,
    const std::uint64_t memtable_memory_budget_mb,
    const std::uint64_t block_сache_size_mb,
    const std::string& db_path,
    std::optional<std::int32_t> ttl = {},
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

  DataBasePtr data_base_;

  DataBaseManagerPoolPtr db_manager_pool_;

  ColumnFamilyHandle* column_family_handle_ = nullptr;

  ReadOptions read_options_;

  WriteOptions write_options_;
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
class ProfileMapImpl final:
  public ProfileMap<Key>,
  public ReferenceCounting::AtomicImpl
{
public:
  using Logger = Logging::Logger;
  using Logger_var = Logging::Logger_var;
  using ConstSmartMemBuf_var = Generics::ConstSmartMemBuf_var;

public:
  explicit ProfileMapImpl(
    Logger* logger,
    const std::uint64_t memtable_memory_budget_mb,
    const std::uint64_t block_сache_size_mb,
    const std::string& db_path,
    std::optional<std::int32_t> ttl = {},
    std::optional<std::string> column_family_name = {})
    : impl_(
        logger,
        memtable_memory_budget_mb,
        block_сache_size_mb,
        db_path,
        ttl,
        column_family_name)
  {
  }

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
  KeyAdapter key_adapter_;

  Internal::ProfileMapImpl impl_;
};

} // namespace AdServer::ProfilingCommons::IoUring

#endif //PROFILEMAPIMPL_HPP_