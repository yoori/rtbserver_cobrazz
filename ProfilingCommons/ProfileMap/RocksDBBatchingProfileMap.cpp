#include <rocksdb/db.h>
#include <rocksdb/iterator.h>
#include <rocksdb/slice.h>
#include <rocksdb/utilities/db_ttl.h>
#include <rocksdb/write_batch.h>

#include <algorithm>
#include <cstdint>
#include <future>
#include <string_view>
#include <vector>

#include <Stream/MemoryStream.hpp>

#include "RocksDBBatchingProfileMap.hpp"
#include "RocksDBOptions.hpp"
#include "RocksDBProfileMapProcessor.hpp"

namespace AdServer::ProfilingCommons
{
  RocksDBBatchingProfileMapImpl::ProcessorQueue::ProcessorQueue(
    RocksDBBatchingProfileMapImpl& map_impl_val,
    unsigned long batch_size_val,
    const Generics::Time& max_delay_val)
    : map_impl(map_impl_val),
      batch_size(std::max(1UL, batch_size_val)),
      max_delay(max_delay_val)
  {}

  struct RocksDBBatchingProfileMapImpl::BatchScratch final
  {
    std::vector<std::string_view> unique_keys;
    std::vector<std::pair<std::string_view, std::size_t>> key_indexes;
    std::vector<rocksdb::Slice> keys;
    std::vector<rocksdb::PinnableSlice> values;
    std::vector<rocksdb::Status> statuses;
    std::vector<unsigned char> value_expired;
    std::vector<unsigned char> touch_allowed;
    std::vector<Operation*> latest_operations;
    rocksdb::WriteBatch write_batch;
  };

  std::shared_ptr<RocksDBBatchingProfileMapImpl::BatchScratch>
  RocksDBBatchingProfileMapImpl::create_batch_scratch_()
  {
    return std::make_shared<BatchScratch>();
  }

  namespace
  {
    constexpr std::size_t TTL_TIMESTAMP_SIZE = sizeof(std::uint32_t);

    auto
    find_key_index(
      const std::vector<std::pair<std::string_view, std::size_t>>& indexes,
      const std::string_view key) noexcept
    {
      return std::find_if(
        indexes.begin(),
        indexes.end(),
        [key](const auto& index)
        {
          return index.first == key;
        });
    }

    std::uint32_t
    decode_fixed32(const char* data) noexcept
    {
      const auto* bytes = reinterpret_cast<const unsigned char*>(data);
      return static_cast<std::uint32_t>(bytes[0]) |
        (static_cast<std::uint32_t>(bytes[1]) << 8) |
        (static_cast<std::uint32_t>(bytes[2]) << 16) |
        (static_cast<std::uint32_t>(bytes[3]) << 24);
    }

    std::string_view
    ttl_user_value(const rocksdb::PinnableSlice& value) noexcept
    {
      if(value.size() < TTL_TIMESTAMP_SIZE)
      {
        return std::string_view(value.data(), value.size());
      }

      return std::string_view(value.data(), value.size() - TTL_TIMESTAMP_SIZE);
    }

    std::optional<Generics::Time>
    ttl_write_time(const rocksdb::PinnableSlice& value) noexcept
    {
      if(value.size() < TTL_TIMESTAMP_SIZE)
      {
        return std::nullopt;
      }

      const std::uint32_t timestamp = decode_fixed32(
        value.data() + value.size() - TTL_TIMESTAMP_SIZE);
      return Generics::Time(timestamp);
    }

    bool
    ttl_expired(
      const rocksdb::PinnableSlice& value,
      const Generics::Time& now,
      const Generics::Time& expire_time) noexcept
    {
      if(expire_time <= Generics::Time::ZERO)
      {
        return false;
      }

      const auto write_time = ttl_write_time(value);
      if(!write_time || *write_time > now)
      {
        return false;
      }

      return *write_time + expire_time <= now;
    }

    bool
    should_touch_ttl(
      const rocksdb::PinnableSlice& value,
      const Generics::Time& now,
      const Generics::Time& touch_period) noexcept
    {
      if(touch_period <= Generics::Time::ZERO)
      {
        return false;
      }

      const auto write_time = ttl_write_time(value);
      if(!write_time || *write_time > now)
      {
        return false;
      }

      return write_time->tv_sec / touch_period.tv_sec <
        now.tv_sec / touch_period.tv_sec;
    }

  }

  RocksDBBatchingProfileMapImpl::RocksDBBatchingProfileMapImpl(
    const String::SubString& path,
    const Generics::Time& expire_time,
    unsigned long workers_count,
    unsigned long batch_size,
    const Generics::Time& max_delay,
    bool disable_wal)
    : RocksDBBatchingProfileMapImpl(
        std::make_shared<RocksDBProfileMapProcessor>(workers_count),
        path,
        expire_time,
        batch_size,
        max_delay,
        disable_wal)
  {
    owns_processor_ = true;
  }

  RocksDBBatchingProfileMapImpl::RocksDBBatchingProfileMapImpl(
    std::shared_ptr<RocksDBProfileMapProcessor> processor,
    const String::SubString& path,
    const Generics::Time& expire_time,
    unsigned long batch_size,
    const Generics::Time& max_delay,
    bool disable_wal)
    : path_(path.str()),
      expire_time_(expire_time),
      batch_size_(std::max(1UL, batch_size)),
      max_delay_(max_delay),
      disable_wal_(disable_wal),
      processor_(std::move(processor)),
      processor_queue_(*this, batch_size_, max_delay_),
      owns_processor_(false)
  {
    static const char* FUN = "RocksDBBatchingProfileMapImpl::RocksDBBatchingProfileMapImpl()";

    if(!processor_)
    {
      throw ProfileMap<std::string>::Exception(
        "RocksDBBatchingProfileMapImpl: null RocksDBProfileMapProcessor");
    }

    rocksdb::Options options;
    configure_rocksdb_profile_map_options(options);

    rocksdb::DBWithTTL* db = nullptr;
    const auto status = rocksdb::DBWithTTL::Open(
      options,
      path_.c_str(),
      &db,
      expire_time.tv_sec);
    if(!status.ok())
    {
      Stream::Error ostr;
      ostr << FUN << ": can't open DB: " << path_;
      throw ProfileMap<std::string>::Exception(ostr.str());
    }

    db_.reset(db);
  }

  RocksDBBatchingProfileMapImpl::~RocksDBBatchingProfileMapImpl() noexcept
  {
    if(db_)
    {
      db_->Close();
    }
  }

  void
  RocksDBBatchingProfileMapImpl::activate_object_()
  {
    if(owns_processor_)
    {
      processor_->activate_object();
    }

    try
    {
      processor_->register_map_(*this);
    }
    catch(...)
    {
      if(owns_processor_)
      {
        processor_->deactivate_object();
        processor_->wait_object();
      }
      throw;
    }
  }

  void
  RocksDBBatchingProfileMapImpl::deactivate_object_()
  {
    processor_->unregister_map_(*this);
    if(owns_processor_)
    {
      processor_->deactivate_object();
    }
  }

  void
  RocksDBBatchingProfileMapImpl::wait_object_()
  {
    processor_->wait_unregister_map_(*this);
    if(owns_processor_)
    {
      processor_->wait_object();
    }
    check_background_error_();
  }

  bool
  RocksDBBatchingProfileMapImpl::check_profile(const std::string& key) const
  {
    check_background_error_();

    using CheckResult = std::pair<bool, std::optional<std::string> >;
    std::promise<CheckResult> promise;
    std::future<CheckResult> future = promise.get_future();

    check_profile_async(
      key,
      [&promise](bool result, std::optional<std::string> error)
      {
        promise.set_value(std::make_pair(result, error));
      });

    const auto result = future.get();
    if(result.second)
    {
      throw ProfileMap<std::string>::Exception(*result.second);
    }

    return result.first;
  }

  void
  RocksDBBatchingProfileMapImpl::check_profile_async(
    const std::string& key,
    CheckCallback callback) const
  {
    check_background_error_();

    Operation operation;
    operation.type = OT_CHECK;
    operation.key = key;
    operation.check_callback = std::move(callback);

    if(!processor_->enqueue_operation_(*this, std::move(operation)))
    {
      throw ProfileMap<std::string>::Exception(
        "RocksDBBatchingProfileMapImpl::check_profile_async(): "
        "object isn't active");
    }
  }

  Generics::ConstSmartMemBuf_var
  RocksDBBatchingProfileMapImpl::get_profile(
    const std::string& key,
    Generics::Time* last_access_time)
  {
    static_cast<void>(last_access_time);

    check_background_error_();

    using GetResult = std::pair<Generics::ConstSmartMemBuf_var, std::optional<std::string> >;
    std::promise<GetResult> promise;
    std::future<GetResult> future = promise.get_future();

    get_profile_async(
      key,
      [&promise](
        Generics::ConstSmartMemBuf_var profile,
        std::optional<std::string> error)
      {
        promise.set_value(std::make_pair(std::move(profile), std::move(error)));
      },
      std::nullopt);

    const auto result = future.get();
    if(result.second)
    {
      throw ProfileMap<std::string>::Exception(*result.second);
    }

    return result.first;
  }

  Generics::SmartMemBuf_var
  RocksDBBatchingProfileMapImpl::get_own_profile(
    const std::string& key,
    Generics::Time* last_access_time)
  {
    static_cast<void>(last_access_time);

    check_background_error_();

    using GetResult = std::pair<
      Generics::SmartMemBuf_var,
      std::optional<std::string> >;
    std::promise<GetResult> promise;
    std::future<GetResult> future = promise.get_future();

    get_own_profile_async(
      key,
      [&promise](
        Generics::SmartMemBuf_var profile,
        std::optional<std::string> error)
      {
        promise.set_value(std::make_pair(
          std::move(profile),
          std::move(error)));
      },
      std::nullopt);

    auto result = future.get();
    if(result.second)
    {
      throw ProfileMap<std::string>::Exception(*result.second);
    }

    return std::move(result.first);
  }

  Generics::ConstSmartMemBuf_var
  RocksDBBatchingProfileMapImpl::get_profile_async(
    const std::string& key,
    GetCallback callback,
    std::optional<Generics::Time> last_access_time)
  {
    static_cast<void>(last_access_time);

    check_background_error_();

    Operation operation;
    operation.type = OT_GET;
    operation.key = key;
    operation.get_callback = std::move(callback);

    if(!processor_->enqueue_operation_(*this, std::move(operation)))
    {
      throw ProfileMap<std::string>::Exception(
        "RocksDBBatchingProfileMapImpl::get_profile_async(): "
        "object isn't active");
    }

    return Generics::ConstSmartMemBuf_var();
  }

  Generics::SmartMemBuf_var
  RocksDBBatchingProfileMapImpl::get_own_profile_async(
    const std::string& key,
    GetOwnCallback callback,
    std::optional<Generics::Time> last_access_time)
  {
    static_cast<void>(last_access_time);

    check_background_error_();

    Operation operation;
    operation.type = OT_GET;
    operation.key = key;
    operation.get_own_callback = std::move(callback);

    if(!processor_->enqueue_operation_(*this, std::move(operation)))
    {
      throw ProfileMap<std::string>::Exception(
        "RocksDBBatchingProfileMapImpl::get_own_profile_async(): "
        "object isn't active");
    }

    return Generics::SmartMemBuf_var();
  }

  void
  RocksDBBatchingProfileMapImpl::save_profile(
    const std::string& key,
    const Generics::ConstSmartMemBuf* profile,
    const Generics::Time& now,
    OperationPriority)
  {
    std::promise<std::optional<std::string>> promise;
    std::future<std::optional<std::string>> future = promise.get_future();

    save_profile_async(
      key,
      profile,
      now,
      [&promise](std::optional<std::string> error)
      {
        promise.set_value(std::move(error));
      });

    const auto error = future.get();
    if(error)
    {
      throw ProfileMap<std::string>::Exception(*error);
    }
  }

  void
  RocksDBBatchingProfileMapImpl::save_profile_async(
    const std::string& key,
    const Generics::ConstSmartMemBuf* profile,
    const Generics::Time&,
    SaveCallback callback)
  {
    static const char* FUN = "RocksDBBatchingProfileMapImpl::save_profile_async()";

    check_background_error_();

    if(!profile)
    {
      Stream::Error ostr;
      ostr << FUN << ": null profile for key='" << key << "'";
      throw ProfileMap<std::string>::Exception(ostr.str());
    }

    Operation operation;
    operation.type = OT_SAVE;
    operation.key = key;
    operation.profile = ReferenceCounting::add_ref(profile);
    if(callback)
    {
      operation.save_callback = std::move(callback);
    }

    if(!processor_->enqueue_operation_(*this, std::move(operation)))
    {
      throw ProfileMap<std::string>::Exception(
        "RocksDBBatchingProfileMapImpl::save_profile_async(): "
        "object isn't active");
    }
  }

  bool
  RocksDBBatchingProfileMapImpl::remove_profile(
    const std::string& key,
    OperationPriority)
  {
    check_background_error_();

    using RemoveResult = std::pair<bool, std::optional<std::string>>;
    std::promise<RemoveResult> promise;
    std::future<RemoveResult> future = promise.get_future();

    remove_profile_async(
      key,
      OP_RUNTIME,
      [&promise](bool result, std::optional<std::string> error)
      {
        promise.set_value(std::make_pair(result, std::move(error)));
      });

    const auto result = future.get();
    if(result.second)
    {
      throw ProfileMap<std::string>::Exception(*result.second);
    }

    return result.first;
  }

  void
  RocksDBBatchingProfileMapImpl::remove_profile_async(
    const std::string& key,
    OperationPriority,
    RemoveCallback callback)
  {
    check_background_error_();

    Operation operation;
    operation.type = OT_REMOVE;
    operation.key = key;
    if(callback)
    {
      operation.remove_callback = std::move(callback);
    }

    if(!processor_->enqueue_operation_(*this, std::move(operation)))
    {
      throw ProfileMap<std::string>::Exception(
        "RocksDBBatchingProfileMapImpl::remove_profile_async(): "
        "object isn't active");
    }
  }

  void
  RocksDBBatchingProfileMapImpl::clear_expired_async(
    const Generics::Time&,
    CompleteCallback complete)
  {
    if(complete)
    {
      complete();
    }
  }

  void
  RocksDBBatchingProfileMapImpl::process_keys(
    std::function<void(const std::string&)> process_key,
    std::function<void(void)> process_complete)
    /*throw(Exception)*/
  {
    static const char* FUN = "RocksDBBatchingProfileMapImpl::process_keys()";

    check_background_error_();
    processor_->wait_pending_operations_(*this);

    std::unique_ptr<rocksdb::Iterator> it(
      db_->NewIterator(rocksdb::ReadOptions()));
    for(it->SeekToFirst(); it->Valid(); it->Next())
    {
      process_key(it->key().ToString());
    }

    const auto status = it->status();
    if(!status.ok())
    {
      Stream::Error ostr;
      ostr << FUN << ": can't iterate DB '" << path_ << "': " <<
        status.ToString();
      throw ProfileMap<std::string>::Exception(ostr.str());
    }

    if(process_complete)
    {
      process_complete();
    }
  }

  void
  RocksDBBatchingProfileMapImpl::process_batch_(
    Operations& batch,
    BatchScratch& scratch)
  {
    if(batch.empty())
    {
      return;
    }

    if(is_write_operation_(batch.front().type))
    {
      process_write_batch_(batch, scratch);
    }
    else
    {
      process_read_batch_(batch, scratch);
    }
  }

  void
  RocksDBBatchingProfileMapImpl::process_read_batch_(
    Operations& batch,
    BatchScratch& scratch)
  {
    logical_read_operations_.fetch_add(batch.size(), std::memory_order_relaxed);
    physical_read_operations_.fetch_add(1, std::memory_order_relaxed);

    auto& unique_keys = scratch.unique_keys;
    auto& key_indexes = scratch.key_indexes;
    auto& keys = scratch.keys;
    auto& values = scratch.values;
    auto& statuses = scratch.statuses;
    auto& value_expired = scratch.value_expired;
    auto& touch_allowed = scratch.touch_allowed;

    unique_keys.clear();
    key_indexes.clear();
    keys.clear();
    values.clear();
    statuses.clear();
    value_expired.clear();
    touch_allowed.clear();

    unique_keys.reserve(batch.size());
    key_indexes.reserve(batch.size());

    for(const auto& operation : batch)
    {
      const std::string_view key(operation.key);
      const auto it = find_key_index(key_indexes, key);
      if(it == key_indexes.end())
      {
        key_indexes.emplace_back(key, unique_keys.size());
        unique_keys.emplace_back(key);
      }
    }

    keys.reserve(unique_keys.size());
    for(const auto& key : unique_keys)
    {
      keys.emplace_back(key.data(), key.size());
    }

    rocksdb::ReadOptions read_options;
    read_options.async_io = true;
    read_options.optimize_multiget_for_io = true;

    values.resize(keys.size());
    statuses.resize(keys.size());
    rocksdb::DB* const base_db = db_->GetBaseDB();
    base_db->MultiGet(
      read_options,
      base_db->DefaultColumnFamily(),
      keys.size(),
      keys.data(),
      values.data(),
      statuses.data());

    value_expired.assign(keys.size(), 0);
    touch_allowed.assign(keys.size(), 0);

    const Generics::Time now = Generics::Time::get_time_of_day();
    for(std::size_t key_index = 0; key_index < keys.size(); ++key_index)
    {
      if(statuses[key_index].ok() &&
        ttl_expired(values[key_index], now, expire_time_))
      {
        value_expired[key_index] = 1;
      }
    }

    for(const auto& operation : batch)
    {
      if(operation.get_callback || operation.get_own_callback)
      {
        const auto key_index_it =
          find_key_index(key_indexes, std::string_view(operation.key));
        touch_allowed[key_index_it->second] = 1;
      }
    }

    const Generics::Time touch_period(expire_time_.tv_sec / 4);

    if(touch_period > Generics::Time::ZERO)
    {
      for(std::size_t key_index = 0; key_index < keys.size(); ++key_index)
      {
        if(touch_allowed[key_index] &&
          statuses[key_index].ok() &&
          !value_expired[key_index] &&
          should_touch_ttl(values[key_index], now, touch_period))
        {
          const std::string_view value = ttl_user_value(values[key_index]);
          Operation touch_operation;
          touch_operation.type = OT_TOUCH;
          touch_operation.key.assign(keys[key_index].data(), keys[key_index].size());
          touch_operation.profile = Generics::ConstSmartMemBuf_var(
            new Generics::ConstSmartMemBuf(value.data(), value.size()));
          processor_->enqueue_operation_(*this, std::move(touch_operation));
        }
      }
    }

    for(auto& operation : batch)
    {
      try
      {
        const auto key_index_it =
          find_key_index(key_indexes, std::string_view(operation.key));
        const std::size_t key_index = key_index_it->second;
        const auto& status = statuses[key_index];
        const auto& value = values[key_index];
        const bool not_found = status.IsNotFound() || value_expired[key_index];
        const std::string_view user_value = ttl_user_value(value);

        if(operation.check_callback)
        {
          if(not_found)
          {
            (*operation.check_callback)(false, std::nullopt);
          }
          else if(!status.ok())
          {
            (*operation.check_callback)(false, status.ToString());
          }
          else
          {
            (*operation.check_callback)(true, std::nullopt);
          }
        }

        if(operation.get_callback)
        {
          if(not_found)
          {
            (*operation.get_callback)(Generics::ConstSmartMemBuf_var(), std::nullopt);
          }
          else if(!status.ok())
          {
            (*operation.get_callback)(
              Generics::ConstSmartMemBuf_var(),
              status.ToString());
          }
          else
          {
            (*operation.get_callback)(
              Generics::ConstSmartMemBuf_var(
                new Generics::ConstSmartMemBuf(
                  user_value.data(),
                  user_value.size())),
              std::nullopt);
          }
        }

        if(operation.get_own_callback)
        {
          if(not_found)
          {
            (*operation.get_own_callback)(
              Generics::SmartMemBuf_var(),
              std::nullopt);
          }
          else if(!status.ok())
          {
            (*operation.get_own_callback)(
              Generics::SmartMemBuf_var(),
              status.ToString());
          }
          else
          {
            (*operation.get_own_callback)(
              Generics::SmartMemBuf_var(
                new Generics::SmartMemBuf(
                  user_value.data(),
                  user_value.size())),
              std::nullopt);
          }
        }
      }
      catch(...)
      {}
    }
  }

  void
  RocksDBBatchingProfileMapImpl::process_write_batch_(
    Operations& batch,
    BatchScratch& scratch)
  {
    logical_write_operations_.fetch_add(batch.size(), std::memory_order_relaxed);
    physical_write_operations_.fetch_add(1, std::memory_order_relaxed);

    auto& write_batch = scratch.write_batch;
    auto& latest_operations = scratch.latest_operations;
    auto& key_indexes = scratch.key_indexes;

    write_batch.Clear();
    latest_operations.clear();
    key_indexes.clear();

    latest_operations.reserve(batch.size());
    key_indexes.reserve(batch.size());

    for(auto& operation : batch)
    {
      const std::string_view key(operation.key);
      const auto it = find_key_index(key_indexes, key);
      if(it == key_indexes.end())
      {
        key_indexes.emplace_back(key, latest_operations.size());
        latest_operations.emplace_back(&operation);
      }
      else
      {
        Operation*& current_operation = latest_operations[it->second];
        if(operation.type != OT_TOUCH || current_operation->type == OT_TOUCH)
        {
          current_operation = &operation;
        }
      }
    }

    for(const auto* operation : latest_operations)
    {
      if(operation->type == OT_SAVE || operation->type == OT_TOUCH)
      {
        write_batch.Put(
          operation->key,
          rocksdb::Slice(
            static_cast<const char*>(operation->profile->membuf().data()),
            operation->profile->membuf().size()));
      }
      else if(operation->type == OT_REMOVE)
      {
        write_batch.Delete(operation->key);
      }
    }

    rocksdb::WriteOptions write_options;
    write_options.disableWAL = disable_wal_;

    const auto status = db_->Write(write_options, &write_batch);
    if(!status.ok())
    {
      throw ProfileMap<std::string>::Exception(
        "RocksDBBatchingProfileMapImpl::process_write_batch_(): " +
        status.ToString());
    }

    for(auto& operation : batch)
    {
      if(operation.save_callback)
      {
        try
        {
          (*operation.save_callback)(std::nullopt);
        }
        catch(...)
        {}
      }

      if(operation.remove_callback)
      {
        try
        {
          (*operation.remove_callback)(true, std::nullopt);
        }
        catch(...)
        {}
      }
    }
  }

  void
  RocksDBBatchingProfileMapImpl::notify_failed_operations_(
    Operations& operations,
    const std::string& error) noexcept
  {
    for(auto& operation : operations)
    {
      try
      {
        if(operation.check_callback)
        {
          (*operation.check_callback)(false, error);
        }

        if(operation.get_callback)
        {
          (*operation.get_callback)(Generics::ConstSmartMemBuf_var(), error);
        }

        if(operation.get_own_callback)
        {
          (*operation.get_own_callback)(Generics::SmartMemBuf_var(), error);
        }

        if(operation.save_callback)
        {
          (*operation.save_callback)(error);
        }

        if(operation.remove_callback)
        {
          (*operation.remove_callback)(false, error);
        }
      }
      catch(...)
      {}
    }
  }

  bool
  RocksDBBatchingProfileMapImpl::direct_check_profile_(const std::string& key) const
  {
    std::string value;
    const auto status = db_->Get(rocksdb::ReadOptions(), key, &value);

    if(status.IsNotFound())
    {
      return false;
    }

    if(!status.ok())
    {
      throw ProfileMap<std::string>::Exception(
        "RocksDBBatchingProfileMapImpl::direct_check_profile_(): " +
        status.ToString());
    }

    return true;
  }

  void
  RocksDBBatchingProfileMapImpl::check_background_error_() const
  {
    Sync::PosixGuard guard(error_lock_);
    if(!background_error_.empty())
    {
      throw ProfileMap<std::string>::Exception(
        "RocksDBBatchingProfileMapImpl background error: " +
        background_error_);
    }
  }

  unsigned long
  RocksDBBatchingProfileMapImpl::size() const noexcept
  {
    return 1;
  }

  unsigned long
  RocksDBBatchingProfileMapImpl::area_size() const noexcept
  {
    return static_cast<unsigned long>(expire_time_.tv_sec >= 0 ? 1 : 1);
  }

  ProfileMap<std::string>::Stats
  RocksDBBatchingProfileMapImpl::stats() const noexcept
  {
    return {
      logical_read_operations_.load(std::memory_order_relaxed),
      logical_write_operations_.load(std::memory_order_relaxed),
      physical_read_operations_.load(std::memory_order_relaxed),
      physical_write_operations_.load(std::memory_order_relaxed)
    };
  }
}
