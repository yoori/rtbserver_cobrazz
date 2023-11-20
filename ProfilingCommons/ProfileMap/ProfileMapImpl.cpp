// STD
#include <thread>

// THIS
#include "ProfileMapImpl.hpp"

namespace AdServer::ProfilingCommons::IoUring
{

namespace Internal
{

ProfileMapImpl::ProfileMapImpl(
  Logger* logger,
  const std::uint64_t memtable_memory_budget_mb,
  const std::uint64_t block_сache_size_mb,
  const std::string& db_path,
  std::optional<std::int32_t> ttl,
  std::optional<std::string> column_family_name)
  : logger_(ReferenceCounting::add_ref(logger)),
    db_path_(db_path)
{
  std::size_t number_threads = std::thread::hardware_concurrency();
  if (number_threads == 0)
  {
    number_threads = 16;
  }

  rocksdb::DBOptions db_options;
  db_options.IncreaseParallelism(number_threads);
  db_options.create_if_missing = true;

  rocksdb::ColumnFamilyOptions column_family_options;
   /*column_family_options.OptimizeLevelStyleCompaction(
   memtable_memory_budget_mb * 1024 * 1024);*/
  column_family_options.OptimizeForPointLookup(
  block_сache_size_mb * 1024 * 1024);
  column_family_options.target_file_size_multiplier = 2;

  if (!column_family_name.has_value())
  {
    column_family_name = rocksdb::kDefaultColumnFamilyName;
  }


  std::optional<std::vector <std::int32_t>> ttls;
  if (ttl.has_value())
  {
    ttls = std::vector<std::int32_t>{*ttl};
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

  ManagerPoolConfig config;
  config.event_queue_max_size = 1000000;
  config.io_uring_flags = IORING_SETUP_ATTACH_WQ;
  config.io_uring_size = 6400;
  config.number_io_urings = 1.5 * number_threads;
  db_manager_pool_ = std::make_unique<DataBaseManagerPool>(
    config,
    logger_.in());

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
  const std::string_view value(
    static_cast<const char*>(profile->membuf().data()),
    profile->membuf().size());
  const auto status = db_manager_pool_->put(
    data_base_,
    *column_family_handle_,
    write_options_,
    key,
    value);
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
  const auto status = db_manager_pool_->erase(
    data_base_,
    *column_family_handle_,
    write_options_,
    key);
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