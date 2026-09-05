#include <rocksdb/db.h>
#include <rocksdb/iterator.h>
#include <rocksdb/slice.h>
#include <rocksdb/utilities/db_ttl.h>
#include <rocksdb/write_batch.h>

#include <algorithm>
#include <boost/unordered/unordered_flat_map.hpp>
#include <cstdint>
#include <functional>
#include <future>
#include <string_view>
#include <vector>

#include <Generics/GnuHashTable.hpp>
#include <Stream/MemoryStream.hpp>

#include "RocksDBBatchingProfileMap.hpp"
#include "RocksDBOptions.hpp"
#include "RocksDBProfileMapProcessor.hpp"

namespace AdServer::ProfilingCommons
{
  struct RocksDBBatchingProfileMapImpl::BatchScratch final
  {
    using KeyIndexMap = boost::unordered_flat_map<
      Generics::StringViewHashAdapter,
      std::size_t,
      Generics::HashFunForHashAdapter<Generics::StringViewHashAdapter>>;

    KeyIndexMap key_indexes;
    std::vector<std::size_t> operation_key_indexes;
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
    const Generics::Time BACKGROUND_ERROR_RETRY_PERIOD = Generics::Time::ONE_MINUTE;

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
      if (value.size() < TTL_TIMESTAMP_SIZE)
      {
        return std::string_view(value.data(), value.size());
      }

      return std::string_view(value.data(), value.size() - TTL_TIMESTAMP_SIZE);
    }

    std::optional<Generics::Time>
    ttl_write_time(const rocksdb::PinnableSlice& value) noexcept
    {
      if (value.size() < TTL_TIMESTAMP_SIZE)
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
      if (expire_time <= Generics::Time::ZERO)
      {
        return false;
      }

      const auto write_time = ttl_write_time(value);
      if (!write_time || *write_time > now)
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
      if (touch_period <= Generics::Time::ZERO)
      {
        return false;
      }

      const auto write_time = ttl_write_time(value);
      if (!write_time || *write_time > now)
      {
        return false;
      }

      return write_time->tv_sec / touch_period.tv_sec < now.tv_sec / touch_period.tv_sec;
    }
  }

  RocksDBBatchingProfileMapImpl::RocksDBBatchingProfileMapImpl(
    const String::SubString& path,
    const Generics::Time& expire_time,
    unsigned long workers_count,
    unsigned long batch_size,
    const Generics::Time& max_delay,
    bool disable_wal,
    unsigned long enqueue_buckets_count)
    : RocksDBBatchingProfileMapImpl(
        std::make_shared<RocksDBProfileMapProcessor>(workers_count, enqueue_buckets_count),
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
      processor_queue_(
        batch_size_,
        max_delay_,
        processor_ ? processor_->enqueue_buckets_count_ : 1),
      owns_processor_(false)
  {
    static const char* FUN = "RocksDBBatchingProfileMapImpl::RocksDBBatchingProfileMapImpl()";

    if (!processor_)
    {
      throw ProfileMap<std::string>::Exception(
        "RocksDBBatchingProfileMapImpl: null RocksDBProfileMapProcessor");
    }

    rocksdb::Options options;
    configure_rocksdb_profile_map_options(options);

    rocksdb::DBWithTTL* db = nullptr;
    const auto status = rocksdb::DBWithTTL::Open(options, path_.c_str(), &db, expire_time.tv_sec);
    if (!status.ok())
    {
      Stream::Error ostr;
      ostr << FUN << ": can't open DB: " << path_;
      throw ProfileMap<std::string>::Exception(ostr.str());
    }

    db_.reset(db);
  }

  RocksDBBatchingProfileMapImpl::~RocksDBBatchingProfileMapImpl() noexcept
  {
    if (db_)
    {
      db_->Close();
    }
  }

  void
  RocksDBBatchingProfileMapImpl::activate_object_()
  {
    if (owns_processor_)
    {
      processor_->activate_object();
    }

    try
    {
      processor_->register_map_(*this);
      {
        Sync::PosixGuard guard(error_lock_);
        stopping_ = false;
        submission_gate_.activate_object();
      }
    }
    catch(...)
    {
      if (owns_processor_)
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
    {
      Sync::PosixGuard guard(error_lock_);
      stopping_ = true;
      submission_gate_.deactivate_object();
    }

    if (owns_processor_)
    {
      processor_->deactivate_object();
    }
  }

  void
  RocksDBBatchingProfileMapImpl::wait_object_()
  {
    submission_gate_.wait_object();
    processor_->wait_unregister_map_(*this);
    if (owns_processor_)
    {
      processor_->wait_object();
    }
    check_background_error_();
  }

  bool
  RocksDBBatchingProfileMapImpl::check_profile(const std::string& key) const
  {
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
    if (result.second)
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
    Operation operation;
    operation.type = OT_CHECK;
    operation.key = key;
    if (callback)
    {
      operation.check_callback = std::move(callback);
    }
    enqueue_async_operation_(
      std::move(operation),
      "RocksDBBatchingProfileMapImpl::check_profile_async()");
  }

  Generics::ConstSmartMemBuf_var
  RocksDBBatchingProfileMapImpl::get_profile(
    const std::string& key,
    Generics::Time* last_access_time)
  {
    static_cast<void>(last_access_time);

    using GetResult = std::pair<Generics::ConstSmartMemBuf_var, std::optional<std::string> >;
    std::promise<GetResult> promise;
    std::future<GetResult> future = promise.get_future();

    get_profile_async(
      key,
      [&promise](Generics::ConstSmartMemBuf_var profile, std::optional<std::string> error)
      {
        promise.set_value(std::make_pair(std::move(profile), std::move(error)));
      },
      std::nullopt);

    const auto result = future.get();
    if (result.second)
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

    using GetResult = std::pair<Generics::SmartMemBuf_var, std::optional<std::string> >;
    std::promise<GetResult> promise;
    std::future<GetResult> future = promise.get_future();

    get_own_profile_async(
      key,
      [&promise](Generics::SmartMemBuf_var profile, std::optional<std::string> error)
      {
        promise.set_value(std::make_pair(std::move(profile), std::move(error)));
      },
      std::nullopt);

    auto result = future.get();
    if (result.second)
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

    Operation operation;
    operation.type = OT_GET;
    operation.key = key;
    if (callback)
    {
      operation.get_callback = std::move(callback);
    }
    enqueue_async_operation_(
      std::move(operation),
      "RocksDBBatchingProfileMapImpl::get_profile_async()");

    return Generics::ConstSmartMemBuf_var();
  }

  Generics::SmartMemBuf_var
  RocksDBBatchingProfileMapImpl::get_own_profile_async(
    const std::string& key,
    GetOwnCallback callback,
    std::optional<Generics::Time> last_access_time)
  {
    static_cast<void>(last_access_time);

    Operation operation;
    operation.type = OT_GET;
    operation.key = key;
    if (callback)
    {
      operation.get_own_callback = std::move(callback);
    }
    enqueue_async_operation_(
      std::move(operation),
      "RocksDBBatchingProfileMapImpl::get_own_profile_async()");

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
    if (error)
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

    Operation operation;
    operation.type = OT_SAVE;
    operation.key = key;
    if (callback)
    {
      operation.save_callback = std::move(callback);
    }

    if (!profile)
    {
      Stream::Error ostr;
      ostr << FUN << ": null profile for key='" << key << "'";
      const std::string error = ostr.str().str();
      if (notify_failed_operation_(operation, error))
      {
        return;
      }

      throw ProfileMap<std::string>::Exception(error);
    }

    operation.profile = ReferenceCounting::add_ref(profile);
    enqueue_async_operation_(
      std::move(operation),
      "RocksDBBatchingProfileMapImpl::save_profile_async()");
  }

  bool
  RocksDBBatchingProfileMapImpl::remove_profile(const std::string& key, OperationPriority)
  {
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
    if (result.second)
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
    Operation operation;
    operation.type = OT_REMOVE;
    operation.key = key;
    if (callback)
    {
      operation.remove_callback = std::move(callback);
    }
    enqueue_async_operation_(
      std::move(operation),
      "RocksDBBatchingProfileMapImpl::remove_profile_async()");
  }

  void
  RocksDBBatchingProfileMapImpl::clear_expired_async(
    const Generics::Time&,
    CompleteCallback complete)
  {
    if (complete)
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

    std::unique_ptr<rocksdb::Iterator> it(db_->NewIterator(rocksdb::ReadOptions()));
    for (it->SeekToFirst(); it->Valid(); it->Next())
    {
      process_key(it->key().ToString());
    }

    const auto status = it->status();
    if (!status.ok())
    {
      Stream::Error ostr;
      ostr << FUN << ": can't iterate DB '" << path_ << "': " << status.ToString();
      throw ProfileMap<std::string>::Exception(ostr.str());
    }

    if (process_complete)
    {
      process_complete();
    }
  }

  void
  RocksDBBatchingProfileMapImpl::process_batch_(Operations& batch, BatchScratch& scratch)
  {
    if (batch.empty())
    {
      return;
    }

    if (is_write_operation_(batch.front().type))
    {
      process_write_batch_(batch, scratch);
    }
    else
    {
      process_read_batch_(batch, scratch);
    }
  }

  void
  RocksDBBatchingProfileMapImpl::process_read_batch_(Operations& batch, BatchScratch& scratch)
  {
    logical_read_operations_.fetch_add(batch.size(), std::memory_order_relaxed);
    physical_read_operations_.fetch_add(1, std::memory_order_relaxed);

    auto& key_indexes = scratch.key_indexes;
    // map batch input index to MultiGet request index
    auto& operation_key_indexes = scratch.operation_key_indexes;
    auto& keys = scratch.keys;
    auto& values = scratch.values;
    auto& statuses = scratch.statuses;
    auto& value_expired = scratch.value_expired;
    auto& touch_allowed = scratch.touch_allowed;

    key_indexes.clear();
    operation_key_indexes.clear();
    keys.clear();
    values.clear();
    statuses.clear();
    value_expired.clear();
    touch_allowed.clear();

    key_indexes.reserve(batch.size());
    operation_key_indexes.reserve(batch.size());
    keys.reserve(batch.size());

    for (auto& operation : batch)
    {
      const Generics::StringViewHashAdapter key(operation.key);
      const std::size_t next_key_index = keys.size();
      const auto [it, inserted] = key_indexes.emplace(key, next_key_index);
      if (inserted)
      {
        operation_key_indexes.emplace_back(next_key_index);
        keys.emplace_back(key.text().data(), key.text().size());
      }
      else
      {
        operation_key_indexes.emplace_back(it->second);
      }
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
    for (std::size_t key_index = 0; key_index < keys.size(); ++key_index)
    {
      if (statuses[key_index].ok() && ttl_expired(values[key_index], now, expire_time_))
      {
        value_expired[key_index] = 1;
      }
    }

    auto operation_key_index_it = operation_key_indexes.begin();
    for (const auto& operation : batch)
    {
      if (operation.get_callback || operation.get_own_callback)
      {
        touch_allowed[*operation_key_index_it] = 1;
      }
      ++operation_key_index_it;
    }

    const Generics::Time touch_period(expire_time_.tv_sec / 4);

    if (touch_period > Generics::Time::ZERO)
    {
      Operations touch_operations;

      for (std::size_t key_index = 0; key_index < keys.size(); ++key_index)
      {
        if (touch_allowed[key_index] &&
          statuses[key_index].ok() &&
          !value_expired[key_index] &&
          should_touch_ttl(values[key_index], now, touch_period))
        {
          const std::string_view value = ttl_user_value(values[key_index]);
          Operation touch_operation;
          touch_operation.type = OT_TOUCH;
          touch_operation.key.assign(
            std::string_view(keys[key_index].data(), keys[key_index].size()));
          touch_operation.profile = Generics::ConstSmartMemBuf_var(
            new Generics::ConstSmartMemBuf(value.data(), value.size()));
          touch_operations.emplace_back(std::move(touch_operation));
        }
      }

      processor_->enqueue_operations_(*this, std::move(touch_operations));
    }

    operation_key_index_it = operation_key_indexes.begin();
    for (auto& operation : batch)
    {
      try
      {
        const std::size_t key_index = *operation_key_index_it;
        const auto& status = statuses[key_index];
        const auto& value = values[key_index];
        const bool not_found = status.IsNotFound() || value_expired[key_index];
        const std::string_view user_value = ttl_user_value(value);

        if (operation.check_callback)
        {
          if (not_found)
          {
            notify_check_operation_(operation, false, std::nullopt);
          }
          else if (!status.ok())
          {
            notify_check_operation_(operation, false, status.ToString());
          }
          else
          {
            notify_check_operation_(operation, true, std::nullopt);
          }
        }

        if (operation.get_callback)
        {
          if (not_found)
          {
            notify_get_operation_(operation, Generics::ConstSmartMemBuf_var(), std::nullopt);
          }
          else if (!status.ok())
          {
            notify_get_operation_(
              operation,
              Generics::ConstSmartMemBuf_var(),
              status.ToString());
          }
          else
          {
            Generics::ConstSmartMemBuf_var profile(
              new Generics::ConstSmartMemBuf(user_value.data(), user_value.size()));
            notify_get_operation_(
              operation,
              std::move(profile),
              std::nullopt);
          }
        }

        if (operation.get_own_callback)
        {
          if (not_found)
          {
            notify_get_own_operation_(operation, Generics::SmartMemBuf_var(), std::nullopt);
          }
          else if (!status.ok())
          {
            notify_get_own_operation_(
              operation,
              Generics::SmartMemBuf_var(),
              status.ToString());
          }
          else
          {
            Generics::SmartMemBuf_var profile(
              new Generics::SmartMemBuf(user_value.data(), user_value.size()));
            notify_get_own_operation_(
              operation,
              std::move(profile),
              std::nullopt);
          }
        }
      }
      catch(const std::exception& ex)
      {
        notify_failed_operation_(operation, ex.what());
      }
      catch(...)
      {
        notify_failed_operation_(operation, "unknown read completion error");
      }

      ++operation_key_index_it;
    }
  }

  void
  RocksDBBatchingProfileMapImpl::process_write_batch_(Operations& batch, BatchScratch& scratch)
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

    for (auto& operation : batch)
    {
      const Generics::StringViewHashAdapter key(operation.key);
      const auto [it, inserted] = key_indexes.emplace(key, latest_operations.size());
      if (inserted)
      {
        latest_operations.emplace_back(&operation);
      }
      else
      {
        Operation*& current_operation = latest_operations[it->second];
        if (operation.type != OT_TOUCH || current_operation->type == OT_TOUCH)
        {
          current_operation = &operation;
        }
      }
    }

    for (const auto* operation : latest_operations)
    {
      if (operation->type == OT_SAVE || operation->type == OT_TOUCH)
      {
        write_batch.Put(
          operation->key.text(),
          rocksdb::Slice(
            static_cast<const char*>(operation->profile->membuf().data()),
            operation->profile->membuf().size()));
      }
      else if (operation->type == OT_REMOVE)
      {
        write_batch.Delete(operation->key.text());
      }
    }

    rocksdb::WriteOptions write_options;
    write_options.disableWAL = disable_wal_;

    const auto status = db_->Write(write_options, &write_batch);
    if (!status.ok())
    {
      throw ProfileMap<std::string>::Exception(
        "RocksDBBatchingProfileMapImpl::process_write_batch_(): " +
        status.ToString());
    }

    for (auto& operation : batch)
    {
      if (operation.save_callback)
      {
        try
        {
          (*operation.save_callback)(std::nullopt);
        }
        catch(...)
        {}
      }

      if (operation.remove_callback)
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
  RocksDBBatchingProfileMapImpl::enqueue_async_operation_(
    Operation operation,
    const char* function_name) const
  {
    try
    {
      check_background_error_();
    }
    catch(const eh::Exception& ex)
    {
      if (notify_failed_operation_(operation, ex.what()))
      {
        return;
      }

      throw;
    }

    if (processor_->enqueue_operation_(*this, operation))
    {
      return;
    }

    const std::string error = std::string(function_name) + ": object isn't active";
    if (!notify_failed_operation_(operation, error))
    {
      throw ProfileMap<std::string>::Exception(error);
    }
  }

  void
  RocksDBBatchingProfileMapImpl::notify_check_operation_(
    Operation& operation,
    bool result,
    std::optional<std::string> error) noexcept
  {
    auto callback = std::move(*operation.check_callback);
    operation.check_callback.reset();
    try
    {
      callback(result, std::move(error));
    }
    catch(...)
    {}
  }

  void
  RocksDBBatchingProfileMapImpl::notify_get_operation_(
    Operation& operation,
    Generics::ConstSmartMemBuf_var profile,
    std::optional<std::string> error) noexcept
  {
    auto callback = std::move(*operation.get_callback);
    operation.get_callback.reset();
    try
    {
      callback(std::move(profile), std::move(error));
    }
    catch(...)
    {}
  }

  void
  RocksDBBatchingProfileMapImpl::notify_get_own_operation_(
    Operation& operation,
    Generics::SmartMemBuf_var profile,
    std::optional<std::string> error) noexcept
  {
    auto callback = std::move(*operation.get_own_callback);
    operation.get_own_callback.reset();
    try
    {
      callback(std::move(profile), std::move(error));
    }
    catch(...)
    {}
  }

  bool
  RocksDBBatchingProfileMapImpl::notify_failed_operation_(
    Operation& operation,
    const std::string& error) noexcept
  {
    try
    {
      if (operation.check_callback)
      {
        auto callback = std::move(*operation.check_callback);
        operation.check_callback.reset();
        callback(false, error);
        return true;
      }

      if (operation.get_callback)
      {
        auto callback = std::move(*operation.get_callback);
        operation.get_callback.reset();
        callback(Generics::ConstSmartMemBuf_var(), error);
        return true;
      }

      if (operation.get_own_callback)
      {
        auto callback = std::move(*operation.get_own_callback);
        operation.get_own_callback.reset();
        callback(Generics::SmartMemBuf_var(), error);
        return true;
      }

      if (operation.save_callback)
      {
        auto callback = std::move(*operation.save_callback);
        operation.save_callback.reset();
        callback(error);
        return true;
      }

      if (operation.remove_callback)
      {
        auto callback = std::move(*operation.remove_callback);
        operation.remove_callback.reset();
        callback(false, error);
        return true;
      }
    }
    catch(...)
    {
      return true;
    }

    return false;
  }

  void
  RocksDBBatchingProfileMapImpl::notify_failed_operations_(
    Operations& operations,
    const std::string& error) noexcept
  {
    for (auto& operation : operations)
    {
      notify_failed_operation_(operation, error);
    }
  }

  bool
  RocksDBBatchingProfileMapImpl::direct_check_profile_(const std::string& key) const
  {
    std::string value;
    const auto status = db_->Get(rocksdb::ReadOptions(), key, &value);

    if (status.IsNotFound())
    {
      return false;
    }

    if (!status.ok())
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
    if (!has_background_error_.load(std::memory_order_acquire))
    {
      return;
    }

    std::uint64_t error_generation;

    {
      Sync::PosixGuard guard(error_lock_);
      if (!has_background_error_.load(std::memory_order_relaxed))
      {
        return;
      }

      if (background_error_probe_in_progress_ ||
        Generics::Time::get_time_of_day() < background_error_retry_at_)
      {
        throw ProfileMap<std::string>::Exception(
          "RocksDBBatchingProfileMapImpl background error: " +
          background_error_);
      }

      background_error_probe_in_progress_ = true;
      error_generation = background_error_generation_;
    }

    submission_gate_.wait_object();
    processor_->wait_pending_operations_(*this);
    const auto resume_status = db_->Resume();
    std::string error;

    {
      Sync::PosixGuard guard(error_lock_);
      if (error_generation == background_error_generation_ && resume_status.ok())
      {
        if (!stopping_)
        {
          submission_gate_.activate_object();
        }

        background_error_.clear();
        background_error_probe_in_progress_ = false;
        has_background_error_.store(false, std::memory_order_release);
        return;
      }

      background_error_probe_in_progress_ = false;
      background_error_retry_at_ =
        Generics::Time::get_time_of_day() + BACKGROUND_ERROR_RETRY_PERIOD;
      error = background_error_;
    }

    if (!resume_status.ok())
    {
      error += "; RocksDB DB::Resume(): " + resume_status.ToString();
    }

    throw ProfileMap<std::string>::Exception(
      "RocksDBBatchingProfileMapImpl background error: " + error);
  }

  void
  RocksDBBatchingProfileMapImpl::set_background_error_(const std::string& error) noexcept
  {
    Sync::PosixGuard guard(error_lock_);
    if (!has_background_error_.load(std::memory_order_relaxed) ||
      background_error_probe_in_progress_)
    {
      background_error_ = error;
      background_error_retry_at_ =
        Generics::Time::get_time_of_day() + BACKGROUND_ERROR_RETRY_PERIOD;
      ++background_error_generation_;
      background_error_probe_in_progress_ = false;
      has_background_error_.store(true, std::memory_order_release);
      submission_gate_.deactivate_object();
    }
  }

  unsigned long RocksDBBatchingProfileMapImpl::size() const noexcept
  {
    std::uint64_t size = 0;
    if (!db_->GetIntProperty(rocksdb::DB::Properties::kEstimateNumKeys, &size))
    {
      return 0;
    }

    return static_cast<unsigned long>(size);
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
