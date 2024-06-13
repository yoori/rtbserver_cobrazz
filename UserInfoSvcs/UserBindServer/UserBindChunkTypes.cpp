// STD
#include <cassert>

// THIS
#include <LogCommons/LogCommons.ipp>
#include <UserInfoSvcs/UserBindServer/UserBindChunkTypes.hpp>
#include <UserInfoSvcs/UserBindServer/Utils.hpp>

namespace AdServer::UserInfoSvcs
{

std::uint16_t get_current_day_from_2000()
{
  static std::tm begin_tm {
    .tm_sec = 0,
    .tm_min = 0,
    .tm_hour = 0,
    .tm_mday = 1,
    .tm_mon = 0,
    .tm_year = 2000 - 1900,
    .tm_wday = 0,
    .tm_yday = 0,
    .tm_isdst = -1,
    .tm_gmtoff = 0,
    .tm_zone = ""
  };
  static const auto begin_time =
    std::chrono::system_clock::from_time_t(
      std::mktime(&begin_tm));

  const auto now = std::chrono::system_clock::now();
  if (now < begin_time)
  {
    std::ostringstream stream;
    stream << FNS
           << "now time before 01.01.2000";
    throw std::runtime_error(stream.str());
  }

  return static_cast<uint16_t>(std::chrono::duration_cast<std::chrono::days>(
    now - begin_time).count());
}

std::vector<RocksdbColumnFamilyPtr> create_rocksdb(
  Logging::Logger* logger,
  const std::string& db_path,
  const std::vector<std::string>& column_family_names,
  const std::size_t number_threads,
  const rocksdb::CompactionStyle compaction_style,
  const std::uint32_t block_сache_size_mb,
  const std::uint32_t ttl)
{
  rocksdb::DBOptions db_options;
  db_options.IncreaseParallelism(number_threads);
  db_options.create_if_missing = true;

  rocksdb::ColumnFamilyOptions column_family_options;
  column_family_options.OptimizeForPointLookup(block_сache_size_mb);
  column_family_options.compaction_style = compaction_style;
  column_family_options.ttl = ttl;

  std::optional<std::vector<int>> ttls(std::vector<int>{});
  std::vector<rocksdb::ColumnFamilyDescriptor> descriptors;
  for (const auto& name : column_family_names)
  {
    ttls->emplace_back(static_cast<int>(ttl));
    descriptors.emplace_back(name, column_family_options);
  }

  auto db = std::make_shared<UServerUtils::Grpc::RocksDB::DataBase>(
    logger,
    db_path,
    db_options,
    descriptors,
    true,
    ttls);

  std::vector<RocksdbColumnFamilyPtr> data_base_column_families;
  for (const auto& name : column_family_names)
  {
    data_base_column_families.emplace_back(
      std::make_shared<RocksdbColumnFamily>(db, name));
  }

  return data_base_column_families;
}

const SeenUserInfoHolder::TimeOffset SeenUserInfoHolder::TIME_OFFSET_NOT_INIT_ =
  std::numeric_limits<SeenUserInfoHolder::TimeOffset>::min();

const Generics::Time SeenUserInfoHolder::TIME_OFFSET_MIN_ =
  Generics::Time(TIME_OFFSET_NOT_INIT_ + 1);

const Generics::Time SeenUserInfoHolder::TIME_OFFSET_MAX_ =
  Generics::Time(std::numeric_limits<SeenUserInfoHolder::TimeOffset>::max());

SeenUserInfoHolder::SeenUserInfoHolder() noexcept
  : first_seen_time_offset_(TIME_OFFSET_NOT_INIT_),
    initial_day_(get_current_day_from_2000())
{
}

Generics::Time SeenUserInfoHolder::get_time(
  const Generics::Time& base_time) const noexcept
{
  assert(first_seen_time_offset_ != TIME_OFFSET_NOT_INIT_);
  return Generics::Time(first_seen_time_offset_) + base_time;
}

void SeenUserInfoHolder::set_time(
  const Generics::Time& time,
  const Generics::Time& base_time) noexcept
{
  first_seen_time_offset_ = std::min(
    std::max(TIME_OFFSET_MIN_, time - base_time),
    TIME_OFFSET_MAX_).tv_sec;
}

void SeenUserInfoHolder::adjust_time(
  const Generics::Time& old_base_time,
  const Generics::Time& new_base_time) noexcept
{
  set_time(get_time(old_base_time), new_base_time);
}

bool SeenUserInfoHolder::is_init() const noexcept
{
  return first_seen_time_offset_ != TIME_OFFSET_NOT_INIT_;
}

BoundUserInfoHolder::BoundUserInfoHolder() noexcept
  : flags(0),
    bad_event_count(0),
    last_bad_event_day(0),
    initial_day(get_current_day_from_2000())
{
}

Portion::Portion(
  const std::size_t actual_fetch_size,
  const std::size_t max_fetch_size,
  const FilterDate& filter,
  const RocksdbParamsPtr& seen_rocksdb_params,
  const RocksdbParamsPtr& bound_rocksdb_params)
  : SeenContainersT(actual_fetch_size, max_fetch_size, filter, seen_rocksdb_params),
    BoundContainersT(actual_fetch_size, max_fetch_size, filter, bound_rocksdb_params)
{
}

Portions::Portions(
  Logging::Logger* logger,
  const std::size_t portions_number,
  const std::string& directory,
  const std::string& seen_name,
  const std::string& bound_name,
  const std::string& rocksdb_path,
  const std::size_t rocksdb_number_threads,
  const rocksdb::CompactionStyle rocksdb_compaction_style,
  const std::uint32_t rocksdb_block_сache_size_mb,
  const std::uint32_t rocksdb_ttl,
  const std::size_t actual_fetch_size,
  const std::size_t max_fetch_size,
  const FilterDate& filter,
  const LoadFilter& load_filter)
  : directory_(directory),
    seen_name_(seen_name),
    bound_name_(bound_name),
    load_filter_(load_filter)
{
  std::vector<std::string> column_family_names = {
    seen_name + "_rocksdb",
    bound_name + "_rocksdb"
  };

  auto column_families = create_rocksdb(
    logger,
    rocksdb_path,
    column_family_names,
    rocksdb_number_threads,
    rocksdb_compaction_style,
    rocksdb_block_сache_size_mb,
    rocksdb_ttl);

  const auto read_options = std::make_shared<rocksdb::ReadOptions>();
  const auto write_options = std::make_shared<rocksdb::WriteOptions>();
  write_options->disableWAL = true;
  write_options->sync = false;

  auto seen_rocksdb_params = std::make_shared<RocksdbParams>(
    logger,
    column_families[0],
    read_options,
    write_options);
  auto bound_rocksdb_params = std::make_shared<RocksdbParams>(
    logger,
    column_families[1],
    read_options,
    write_options);

  array_.reserve(portions_number);
  for (std::size_t i = 0; i < portions_number; ++i)
  {
    array_.emplace_back(std::make_shared<Portion>(
      actual_fetch_size,
      max_fetch_size,
      filter,
      seen_rocksdb_params,
      bound_rocksdb_params));
  }

  load();
}

std::size_t Portions::hash(const String::SubString& id) const noexcept
{
  return Utils::hash(id);
}

PortionPtr Portions::portion(const std::size_t id_hash) const noexcept
{
  const auto index = portion_index(id_hash);
  return array_[index];
}

std::size_t Portions::portion_index(const std::size_t id_hash) const noexcept
{
  return ((id_hash & 0xFFFF) ^ ((id_hash >> 16) & 0xFFFF)) % array_.size();
}

void Portions::save()
{
  const std::string seen_file_path = directory_ + "/" + seen_name_;
  const std::string temp_seen_file_path = directory_ + "/~" + seen_name_;
  const std::string bound_file_path = directory_ + "/" + bound_name_;
  const std::string temp_bound_file_path = directory_ + "/~" + bound_name_;

  std::ofstream file_seen_stream(temp_seen_file_path, std::ios::out | std::ios::trunc);
  if (!file_seen_stream.is_open())
  {
    Stream::Error stream;
    stream << FNS
           << "can't open file="
           << temp_seen_file_path;
  }
  bool need_white_space = false;
  for (const auto& portion : array_)
  {
    if (need_white_space)
    {
      file_seen_stream << ' ';
    }
    const auto start_position = file_seen_stream.tellp();
    portion->SeenContainersT::save(file_seen_stream);
    const auto end_position = file_seen_stream.tellp();
    if (start_position == end_position)
    {
      need_white_space = false;
    }
    else
    {
      need_white_space = true;
    }
  }
  if (file_seen_stream.bad() || file_seen_stream.fail())
  {
    Stream::Error stream;
    stream << FNS
           << "error writing to file="
           << temp_seen_file_path;
  }
  file_seen_stream.close();
  std::filesystem::rename(temp_seen_file_path, seen_file_path);

  std::ofstream file_bound_stream(temp_bound_file_path, std::ios::out | std::ios::trunc);
  if (!file_bound_stream.is_open())
  {
    Stream::Error stream;
    stream << FNS
           << "can't open file="
           << temp_bound_file_path;
  }
  bool need_new_line = false;
  for (const auto& portion : array_)
  {
    if (need_new_line)
    {
      file_bound_stream << '\n';
    }
    const auto start_position = file_bound_stream.tellp();
    portion->BoundContainersT::save(file_bound_stream);
    const auto end_position = file_bound_stream.tellp();
    if (start_position != end_position)
    {
      need_new_line = true;
    }
    else
    {
      need_new_line = false;
    }
  }
  if (file_bound_stream.bad() || file_bound_stream.fail())
  {
    Stream::Error stream;
    stream << FNS
           << "error writing to file="
           << temp_bound_file_path;
  }

  file_bound_stream.close();
  std::filesystem::rename(temp_bound_file_path, bound_file_path);
}

void Portions::load()
{
  const std::string seen_file_path = directory_ + "/" + seen_name_;
  const std::string bound_file_path = directory_ + "/" + bound_name_;

  Loader<HashHashAdapter, SeenUserInfoHolder> seen_loader(seen_file_path, *this);
  seen_loader.load();
  Loader<
    std::pair<
      StringDefHashAdapter,
      ExternalIdHashAdapter>,
   BoundUserInfoHolder> bound_loader(bound_file_path, *this);
  bound_loader.load();
}

void Portions::on_load(
  HashHashAdapter&& key,
  SeenUserInfoHolder&& value)
{
  const auto id_hash = key.hash();
  auto id_portion = portion(id_hash);

  if (load_filter_(id_hash))
  {
    id_portion->set_seen(
      std::move(key),
      std::move(value));
  }
}

void Portions::on_load(
  std::pair<StringDefHashAdapter, ExternalIdHashAdapter>&& key,
  BoundUserInfoHolder&& value)
{
  std::string id(key.first.str());
  if (!id.empty())
  {
    id += '/';
  }
  id += key.second.str();
  const auto id_hash = hash(id);
  auto id_portion = portion(id_hash);

  if (load_filter_(id_hash))
  {
    id_portion->set_bound(
      std::move(key.first),
      std::move(key.second),
      std::move(value));
  }
}

} // namespace AdServer::UserInfoSvcs