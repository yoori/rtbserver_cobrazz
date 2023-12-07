// STD
#include <thread>

// THIS
#include "AsyncRocksDBProfileMap.hpp"

namespace AdServer::ProfilingCommons::RocksDB
{

namespace Internal
{

ProfileMapImpl::ProfileMapImpl(
  Logger* logger,
  const DataBaseManagerPoolPtr& db_manager_pool,
  const std::string& db_path,
  const RocksDBParams& rocksdb_params,
  std::optional<std::string> column_family_name)
  : logger_(ReferenceCounting::add_ref(logger)),
    db_manager_pool_(db_manager_pool),
    db_path_(db_path)
{
  std::size_t number_threads = 0;
  if (rocksdb_params.number_background_threads)
  {
    number_threads = *rocksdb_params.number_background_threads;
  }
  else
  {
    number_threads = std::thread::hardware_concurrency();
  }

  if (number_threads == 0)
  {
    number_threads = 5;
  }

  rocksdb::DBOptions db_options;
  db_options.IncreaseParallelism(number_threads);
  db_options.create_if_missing = true;

  rocksdb::ColumnFamilyOptions column_family_options;
  column_family_options.OptimizeForPointLookup(rocksdb_params.block_сache_size_mb);
  column_family_options.compaction_style = rocksdb_params.compaction_style;
  column_family_options.target_file_size_multiplier = 2;

  std::optional<std::vector<std::int32_t>> ttls;
  if (rocksdb_params.ttl.has_value())
  {
    const auto ttl = *rocksdb_params.ttl;
    column_family_options.ttl = ttl;
    ttls = std::vector<std::int32_t>{ttl};
  }

  if (!column_family_name.has_value())
  {
    column_family_name = rocksdb::kDefaultColumnFamilyName;
  }

  std::vector<rocksdb::ColumnFamilyDescriptor> descriptors{
    {*column_family_name, column_family_options}};

  data_base_ = std::make_shared<DataBase>(
    logger_.in(),
    db_path,
    db_options,
    descriptors,
    true,
    ttls);

  column_family_handle_ = &data_base_->column_family(
    *column_family_name);

  write_options_.disableWAL = true;
  write_options_.sync = false;
}

ProfileMapImpl::ConstSmartMemBuf_var
ProfileMapImpl::ProfileMapImpl::get_profile(
  const std::string_view key)
{
  std::string value;
  const auto status = db_manager_pool_->get(
    data_base_,
    *column_family_handle_,
    read_options_,
    key,
    value);

  if(status.IsNotFound())
  {
    return {};
  }

  if (!status.ok())
  {
    Stream::Error stream;
    stream << FNS
           << ": can't read record from DB: "
           << db_path_
           << " with key="
           << key;
    throw Exception(stream);
  }

  return ConstSmartMemBuf_var(
    new Generics::ConstSmartMemBuf(
      value.data(),
      value.size()));
}

bool ProfileMapImpl::check_profile(const std::string_view key) const
{
  std::string value;
  const auto status = db_manager_pool_->get(
    data_base_,
    *column_family_handle_,
    read_options_,
    key,
    value);

  if (!status.IsNotFound())
  {
    return false;
  }

  if (!status.ok())
  {
    Stream::Error stream;
    stream << FNS
           << ": can't read record from DB: "
           << db_path_
           << " with key="
           << key;
    throw Exception(stream);
  }

  return true;
}

void ProfileMapImpl::save_profile(
  const std::string_view key,
  const Generics::ConstSmartMemBuf* profile)
{
  /**
   * The write operation with parameters disableWAL = true,
   * sync = false; is itself asynchronous, so let’s replace
   * it with the original one.
   **/

  /*const auto& membuf = profile->membuf();
  const std::string_view value(
    static_cast<const char*>(membuf.data()),
    membuf.size());
  const auto status = db_manager_pool_->put(
    data_base_,
    *column_family_handle_,
    write_options_,
    key,
    value);*/

  rocksdb::Slice key_slice(
    key.data(),
    key.size());
  const auto& membuf = profile->membuf();
  rocksdb::Slice value_slice(
    static_cast<const char*>(membuf.data()),
    membuf.size());
  const auto status = data_base_->get().Put(
    write_options_,
    column_family_handle_,
    key_slice,
    value_slice);
  if (!status.ok())
  {
    Stream::Error stream;
    stream << FNS
           << ": can't save record to DB '"
           << db_path_
           << "': "
           << status.ToString();
    throw Exception(stream);
  }
}

bool ProfileMapImpl::remove_profile(
  const std::string_view key)
{
  /**
   * Since delete is implemented via write,
   * replace the asynchronous version with a synchronous one.
   **/

  /*const auto status = db_manager_pool_->erase(
    data_base_,
    *column_family_handle_,
    write_options_,
    key);*/

  rocksdb::Slice key_slice(
    key.data(),
    key.size());
  const auto status = data_base_->get().Delete(
    write_options_,
    column_family_handle_,
    key_slice);
 if (status.ok() || status.IsNotFound())
  {
    return true;
  }
  else
  {
    return false;
  }
}

} // namespace Internal

} // namespace AdServer::ProfilingCommons