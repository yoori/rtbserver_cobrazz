#include <toplingdb/rocksdb/db.h>
#include <toplingdb/rocksdb/options.h>
#include <toplingdb/rocksdb/write_batch.h>
#include <toplingdb/topling/side_plugin_repo.h>

#include <algorithm>
#include <ctime>
#include <future>
#include <iomanip>
#include <sstream>
#include <unordered_set>

#include <Stream/MemoryStream.hpp>

#include "ToplingDBProfileMap.hpp"

namespace AdServer::ProfilingCommons
{
  namespace
  {
    constexpr std::size_t TTL_TIMESTAMP_SIZE = sizeof(std::int32_t);

    void
    append_json_string(std::ostream& os, const std::string& value)
    {
      os << '"';
      for(const unsigned char ch : value)
      {
        switch(ch)
        {
        case '\\':
          os << "\\\\";
          break;
        case '"':
          os << "\\\"";
          break;
        case '\b':
          os << "\\b";
          break;
        case '\f':
          os << "\\f";
          break;
        case '\n':
          os << "\\n";
          break;
        case '\r':
          os << "\\r";
          break;
        case '\t':
          os << "\\t";
          break;
        default:
          if(ch < 0x20)
          {
            os << "\\u"
              << std::hex
              << std::setw(4)
              << std::setfill('0')
              << static_cast<int>(ch)
              << std::dec
              << std::setfill(' ');
          }
          else
          {
            os << ch;
          }
          break;
        }
      }
      os << '"';
    }

    std::string
    build_topling_side_plugin_config(const std::string& path)
    {
      std::ostringstream os;
      os <<
        R"json({
  "Cache": {
    "block_cache": {
      "class": "LRUCache",
      "params": {
        "capacity": "512M"
      }
    }
  },
  "TableFactory": {
    "fast": {
      "class": "SingleFastTable",
      "params": {
        "indexType": "MainPatricia",
        "keyPrefixLen": 0
      }
    },
    "bb": {
      "class": "BlockBasedTable",
      "params": {
        "block_size": "4K",
        "block_restart_interval": 16,
        "index_block_restart_interval": 1,
        "metadata_block_size": "4K",
        "block_cache": "${block_cache}",
        "block_cache_compressed": null,
        "persistent_cache": null,
        "filter_policy": null
      }
    },
    "dispatch": {
      "class": "DispatcherTable",
      "params": {
        "default": "fast",
        "readers": {
          "SingleFastTable": "fast",
          "BlockBasedTable": "bb"
        },
        "level_writers": [
          "fast",
          "fast",
          "fast",
          "fast",
          "fast",
          "fast",
          "fast"
        ]
      }
    }
  },
  "CFOptions": {
    "default": {
      "max_write_buffer_number": 4,
      "write_buffer_size": "64M",
      "target_file_size_base": "16M",
      "target_file_size_multiplier": 2,
      "level0_slowdown_writes_trigger": 20,
      "level0_stop_writes_trigger": 36,
      "level0_file_num_compaction_trigger": 4,
      "table_factory": "dispatch",
      "ttl": 0
    }
  },
  "DBOptions": {
    "dbo": {
      "create_if_missing": true,
      "create_missing_column_families": true,
      "fail_if_options_file_error": false,
      "max_background_compactions": 4,
      "max_subcompactions": 1,
      "allow_mmap_reads": true
    }
  },
  "databases": {
    "profile_map": {
      "method": "DB::Open",
      "params": {
        "db_options": "$dbo",
        "cf_options": "$default",
        "path": )json";
      append_json_string(os, path);
      os <<
        R"json(
      }
    }
  },
  "open": "profile_map"
})json";
      return os.str();
    }

    std::string
    pack_value_with_ttl_timestamp(const Generics::ConstSmartMemBuf* profile)
    {
      std::string value(
        static_cast<const char*>(profile->membuf().data()),
        profile->membuf().size());
      const std::int32_t timestamp =
        static_cast<std::int32_t>(std::time(nullptr));
      value.append(
        reinterpret_cast<const char*>(&timestamp),
        sizeof(timestamp));
      return value;
    }

    Generics::ConstSmartMemBuf_var
    unpack_value_with_ttl_timestamp(const std::string& value)
    {
      if(value.size() < TTL_TIMESTAMP_SIZE)
      {
        return Generics::ConstSmartMemBuf_var(
          new Generics::ConstSmartMemBuf(value.data(), value.size()));
      }

      return Generics::ConstSmartMemBuf_var(
        new Generics::ConstSmartMemBuf(
          value.data(),
          value.size() - TTL_TIMESTAMP_SIZE));
    }
  }

  ToplingDBProfileMapImpl::ToplingDBProfileMapImpl(
    const String::SubString& path,
    const Generics::Time& expire_time,
    unsigned long workers_count,
    unsigned long batch_size,
    const Generics::Time& max_delay)
    : path_(path.str()),
      expire_time_(expire_time),
      workers_count_(std::max(1UL, workers_count)),
      batch_size_(std::max(1UL, batch_size)),
      max_delay_(max_delay)
  {
    static const char* FUN = "ToplingDBProfileMapImpl::ToplingDBProfileMapImpl()";

    plugin_repo_.reset(new toplingdb::SidePluginRepo());

    const auto import_status = plugin_repo_->Import(
      build_topling_side_plugin_config(path_));
    if(!import_status.ok())
    {
      Stream::Error ostr;
      ostr << FUN << ": can't import ToplingDB config: "
        << import_status.ToString();
      throw ProfileMap<std::string>::Exception(ostr.str());
    }

    toplingdb::DB* db = nullptr;
    const auto status = plugin_repo_->OpenDB(&db);
    if(!status.ok())
    {
      Stream::Error ostr;
      ostr << FUN << ": can't open DB: " << path_ << ": "
        << status.ToString();
      throw ProfileMap<std::string>::Exception(ostr.str());
    }

    db_ = db;
  }

  ToplingDBProfileMapImpl::~ToplingDBProfileMapImpl() noexcept
  {
    if(plugin_repo_)
    {
      plugin_repo_->CloseAllDB(true);
      db_ = nullptr;
    }
  }

  void
  ToplingDBProfileMapImpl::activate_object_()
  {
    workers_.reserve(workers_count_);

    for(unsigned long i = 0; i < workers_count_; ++i)
    {
      workers_.emplace_back(&ToplingDBProfileMapImpl::worker_loop_, this);
    }
  }

  void
  ToplingDBProfileMapImpl::deactivate_object_()
  {
    queue_cond_.broadcast();
  }

  void
  ToplingDBProfileMapImpl::wait_object_()
  {
    for(auto& worker : workers_)
    {
      worker.join();
    }

    workers_.clear();

    Operations rest;
    {
      Sync::PosixGuard guard(queue_lock_);
      rest.splice(rest.end(), read_operations_);
      rest.splice(rest.end(), write_operations_);
      key_sequences_.clear();
      in_flight_keys_.clear();
    }

    notify_failed_operations_(rest, "ToplingDBProfileMapImpl stopped");
    check_background_error_();
  }

  bool
  ToplingDBProfileMapImpl::check_profile(const std::string& key) const
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
  ToplingDBProfileMapImpl::check_profile_async(
    const std::string& key,
    CheckCallback callback) const
  {
    check_background_error_();

    Operation operation;
    operation.type = OT_CHECK;
    operation.key = key;
    operation.check_callback = std::move(callback);

    if(!enqueue_operation_(std::move(operation)))
    {
      throw ProfileMap<std::string>::Exception(
        "ToplingDBProfileMapImpl::check_profile_async(): object isn't active");
    }
  }

  Generics::ConstSmartMemBuf_var
  ToplingDBProfileMapImpl::get_profile(
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
        const Generics::ConstSmartMemBuf_var& profile,
        std::optional<std::string> error)
      {
        promise.set_value(std::make_pair(profile, error));
      },
      std::nullopt);

    const auto result = future.get();
    if(result.second)
    {
      throw ProfileMap<std::string>::Exception(*result.second);
    }

    return result.first;
  }

  Generics::ConstSmartMemBuf_var
  ToplingDBProfileMapImpl::get_profile_async(
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

    if(!enqueue_operation_(std::move(operation)))
    {
      throw ProfileMap<std::string>::Exception(
        "ToplingDBProfileMapImpl::get_profile_async(): object isn't active");
    }

    return Generics::ConstSmartMemBuf_var();
  }

  void
  ToplingDBProfileMapImpl::save_profile(
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
  ToplingDBProfileMapImpl::save_profile_async(
    const std::string& key,
    const Generics::ConstSmartMemBuf* profile,
    const Generics::Time&,
    SaveCallback callback)
  {
    static const char* FUN = "ToplingDBProfileMapImpl::save_profile_async()";

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

    if(!enqueue_operation_(std::move(operation)))
    {
      throw ProfileMap<std::string>::Exception(
        "ToplingDBProfileMapImpl::save_profile_async(): object isn't active");
    }
  }

  bool
  ToplingDBProfileMapImpl::remove_profile(
    const std::string& key,
    OperationPriority)
  {
    check_background_error_();

    std::promise<std::optional<std::string>> promise;
    std::future<std::optional<std::string>> future = promise.get_future();

    Operation operation;
    operation.type = OT_REMOVE;
    operation.key = key;
    operation.save_callback =
      [&promise](std::optional<std::string> error)
      {
        promise.set_value(std::move(error));
      };

    if(!enqueue_operation_(std::move(operation)))
    {
      return false;
    }

    const auto error = future.get();
    if(error)
    {
      throw ProfileMap<std::string>::Exception(*error);
    }

    return true;
  }

  bool
  ToplingDBProfileMapImpl::enqueue_operation_(Operation&& operation) const
  {
    Sync::PosixGuard guard(queue_lock_);
    if(!active())
    {
      return false;
    }

    operation.sequence = next_operation_sequence_++;
    key_sequences_[operation.key].push_back(operation.sequence);

    const bool was_empty =
      read_operations_.empty() && write_operations_.empty();
    Operations& queue = is_write_operation_(operation.type) ?
      write_operations_ :
      read_operations_;
    const bool fills_batch =
      queue.size() + 1 >= batch_size_ && queue.size() < batch_size_;
    queue.emplace_back(std::move(operation));

    if(was_empty || fills_batch)
    {
      queue_cond_.signal();
    }

    return true;
  }

  void
  ToplingDBProfileMapImpl::worker_loop_() noexcept
  {
    Operations batch;

    while(pop_batch_(batch))
    {
      try
      {
        process_batch_(batch);
      }
      catch(const eh::Exception& ex)
      {
        notify_failed_operations_(batch, ex.what());

        Sync::PosixGuard guard(error_lock_);
        if(background_error_.empty())
        {
          background_error_ = ex.what();
        }
      }
      catch(...)
      {
        notify_failed_operations_(batch, "unknown background error");

        Sync::PosixGuard guard(error_lock_);
        if(background_error_.empty())
        {
          background_error_ = "unknown background error";
        }
      }

      complete_batch_(batch);
      batch.clear();
    }
  }

  bool
  ToplingDBProfileMapImpl::pop_batch_(Operations& batch) noexcept
  {
    Sync::PosixGuard guard(queue_lock_);

    while(true)
    {
      while(read_operations_.empty() && write_operations_.empty() && active())
      {
        queue_cond_.wait(queue_lock_);
      }

      if(read_operations_.empty() && write_operations_.empty() && !active())
      {
        return false;
      }

      collect_batch_(batch);
      if(!batch.empty())
      {
        if(max_delay_ == Generics::Time::ZERO ||
          batch.size() >= batch_size_)
        {
          return true;
        }

        const Generics::Time deadline =
          Generics::Time::get_time_of_day() + max_delay_;
        while(batch.size() < batch_size_)
        {
          const bool signaled = queue_cond_.timed_wait(
            queue_lock_,
            &deadline,
            false);
          collect_batch_(batch);
          if(batch.size() >= batch_size_ || !signaled)
          {
            break;
          }
        }

        return true;
      }

      queue_cond_.wait(queue_lock_);
    }
  }

  void
  ToplingDBProfileMapImpl::collect_batch_(Operations& batch) noexcept
  {
    if(read_operations_.empty() && write_operations_.empty())
    {
      return;
    }

    if(!batch.empty())
    {
      collect_from_queue_(
        is_write_operation_(batch.front().type) ?
          write_operations_ :
          read_operations_,
        batch);
    }
    else if(read_operations_.empty())
    {
      collect_from_queue_(write_operations_, batch);
    }
    else if(write_operations_.empty())
    {
      collect_from_queue_(read_operations_, batch);
    }
    else
    {
      Operation& read_head = read_operations_.front();
      Operation& write_head = write_operations_.front();

      if(read_head.sequence < write_head.sequence)
      {
        collect_from_queue_(read_operations_, batch);
      }
      else
      {
        collect_from_queue_(write_operations_, batch);
      }
    }
  }

  void
  ToplingDBProfileMapImpl::collect_from_queue_(
    Operations& source,
    Operations& batch) noexcept
  {
    auto it = source.begin();
    while(it != source.end() &&
      batch.size() < batch_size_)
    {
      const auto key_sequence_it = key_sequences_.find(it->key);
      const bool is_next_for_key =
        key_sequence_it != key_sequences_.end() &&
        !key_sequence_it->second.empty() &&
        key_sequence_it->second.front() == it->sequence;

      if(is_next_for_key && in_flight_keys_.find(it->key) == in_flight_keys_.end())
      {
        auto current = it++;
        in_flight_keys_.insert(current->key);
        batch.splice(batch.end(), source, current);
      }
      else
      {
        ++it;
      }
    }
  }

  void
  ToplingDBProfileMapImpl::complete_batch_(Operations& batch) noexcept
  {
    Sync::PosixGuard guard(queue_lock_);

    for(const auto& operation : batch)
    {
      in_flight_keys_.erase(operation.key);

      auto key_sequence_it = key_sequences_.find(operation.key);
      if(key_sequence_it != key_sequences_.end())
      {
        if(!key_sequence_it->second.empty() &&
          key_sequence_it->second.front() == operation.sequence)
        {
          key_sequence_it->second.pop_front();
        }

        if(key_sequence_it->second.empty())
        {
          key_sequences_.erase(key_sequence_it);
        }
      }
    }

    if(!read_operations_.empty() || !write_operations_.empty())
    {
      queue_cond_.broadcast();
    }
  }

  void
  ToplingDBProfileMapImpl::process_batch_(Operations& batch)
  {
    if(batch.empty())
    {
      return;
    }

    if(is_write_operation_(batch.front().type))
    {
      process_write_batch_(batch);
    }
    else
    {
      process_read_batch_(batch);
    }
  }

  void
  ToplingDBProfileMapImpl::process_read_batch_(Operations& batch)
  {
    logical_read_operations_.fetch_add(batch.size(), std::memory_order_relaxed);
    physical_read_operations_.fetch_add(1, std::memory_order_relaxed);

    std::vector<toplingdb::Slice> keys;
    keys.reserve(batch.size());

    for(const auto& operation : batch)
    {
      keys.emplace_back(operation.key);
    }

    std::vector<std::string> values(keys.size());
    const auto statuses = db_->MultiGet(toplingdb::ReadOptions(), keys, &values);

    auto status_it = statuses.begin();
    auto value_it = values.begin();
    for(auto& operation : batch)
    {
      try
      {
        if(operation.check_callback)
        {
          if(status_it->IsNotFound())
          {
            (*operation.check_callback)(false, std::nullopt);
          }
          else if(!status_it->ok())
          {
            (*operation.check_callback)(false, status_it->ToString());
          }
          else
          {
            (*operation.check_callback)(true, std::nullopt);
          }
        }

        if(operation.get_callback)
        {
          if(status_it->IsNotFound())
          {
            (*operation.get_callback)(Generics::ConstSmartMemBuf_var(), std::nullopt);
          }
          else if(!status_it->ok())
          {
            (*operation.get_callback)(
              Generics::ConstSmartMemBuf_var(),
              status_it->ToString());
          }
          else
          {
            (*operation.get_callback)(
              unpack_value_with_ttl_timestamp(*value_it),
              std::nullopt);
          }
        }
      }
      catch(...)
      {}

      ++status_it;
      ++value_it;
    }
  }

  void
  ToplingDBProfileMapImpl::process_write_batch_(Operations& batch)
  {
    logical_write_operations_.fetch_add(batch.size(), std::memory_order_relaxed);
    physical_write_operations_.fetch_add(1, std::memory_order_relaxed);

    toplingdb::WriteBatch write_batch;

    for(const auto& operation : batch)
    {
      if(operation.type == OT_SAVE)
      {
        const std::string value =
          pack_value_with_ttl_timestamp(operation.profile.in());
        write_batch.Put(
          operation.key,
          toplingdb::Slice(value.data(), value.size()));
      }
      else if(operation.type == OT_REMOVE)
      {
        write_batch.Delete(operation.key);
      }
    }

    const auto status = db_->Write(toplingdb::WriteOptions(), &write_batch);
    if(!status.ok())
    {
      throw ProfileMap<std::string>::Exception(
        "ToplingDBProfileMapImpl::process_write_batch_(): " +
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
    }
  }

  void
  ToplingDBProfileMapImpl::notify_failed_operations_(
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

        if(operation.save_callback)
        {
          (*operation.save_callback)(error);
        }
      }
      catch(...)
      {}
    }
  }

  bool
  ToplingDBProfileMapImpl::direct_check_profile_(const std::string& key) const
  {
    std::string value;
    const auto status = db_->Get(toplingdb::ReadOptions(), key, &value);

    if(status.IsNotFound())
    {
      return false;
    }

    if(!status.ok())
    {
      throw ProfileMap<std::string>::Exception(
        "ToplingDBProfileMapImpl::direct_check_profile_(): " +
        status.ToString());
    }

    return true;
  }

  bool
  ToplingDBProfileMapImpl::is_write_operation_(OperationType type) noexcept
  {
    return type == OT_SAVE || type == OT_REMOVE;
  }

  void
  ToplingDBProfileMapImpl::check_background_error_() const
  {
    Sync::PosixGuard guard(error_lock_);
    if(!background_error_.empty())
    {
      throw ProfileMap<std::string>::Exception(
        "ToplingDBProfileMapImpl background error: " + background_error_);
    }
  }

  unsigned long
  ToplingDBProfileMapImpl::size() const noexcept
  {
    return 1;
  }

  unsigned long
  ToplingDBProfileMapImpl::area_size() const noexcept
  {
    return static_cast<unsigned long>(expire_time_.tv_sec >= 0 ? 1 : 1);
  }

  ProfileMap<std::string>::Stats
  ToplingDBProfileMapImpl::stats() const noexcept
  {
    return {
      logical_read_operations_.load(std::memory_order_relaxed),
      logical_write_operations_.load(std::memory_order_relaxed),
      physical_read_operations_.load(std::memory_order_relaxed),
      physical_write_operations_.load(std::memory_order_relaxed)
    };
  }
}
