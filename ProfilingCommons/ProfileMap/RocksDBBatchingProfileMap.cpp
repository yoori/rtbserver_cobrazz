#include <rocksdb/db.h>
#include <rocksdb/iterator.h>
#include <rocksdb/options.h>
#include <rocksdb/table.h>
#include <rocksdb/utilities/db_ttl.h>
#include <rocksdb/write_batch.h>

#include <algorithm>
#include <future>
#include <string_view>

#include <Stream/MemoryStream.hpp>

#include <Commons/ThreadName.hpp>

#include "RocksDBBatchingProfileMap.hpp"

namespace AdServer::ProfilingCommons
{
  struct RocksDBBatchingProfileMapImpl::BatchScratch final
  {
    std::vector<std::string_view> selected_keys;
    std::vector<std::string_view> unique_keys;
    std::vector<std::pair<std::string_view, std::size_t>> key_indexes;
    std::vector<rocksdb::Slice> keys;
    std::vector<std::string> values;
    std::vector<Operation*> latest_operations;
    rocksdb::WriteBatch write_batch;
  };

  namespace
  {
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
  }

  RocksDBBatchingProfileMapImpl::RocksDBBatchingProfileMapImpl(
    const String::SubString& path,
    const Generics::Time& expire_time,
    unsigned long workers_count,
    unsigned long batch_size,
    const Generics::Time& max_delay,
    bool disable_wal)
    : path_(path.str()),
      expire_time_(expire_time),
      workers_count_(std::max(1UL, workers_count)),
      batch_size_(std::max(1UL, batch_size)),
      max_delay_(max_delay),
      disable_wal_(disable_wal)
  {
    static const char* FUN = "RocksDBBatchingProfileMapImpl::RocksDBBatchingProfileMapImpl()";

    rocksdb::Options options;
    options.IncreaseParallelism();
    options.OptimizeLevelStyleCompaction();
    options.create_if_missing = true;
    options.compression = rocksdb::kNoCompression;
    options.target_file_size_multiplier = 2;

    rocksdb::BlockBasedTableOptions table_options;
    table_options.checksum = rocksdb::kNoChecksum;
    options.table_factory.reset(
      rocksdb::NewBlockBasedTableFactory(table_options));

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
    workers_.reserve(workers_count_);

    for(unsigned long i = 0; i < workers_count_; ++i)
    {
      workers_.emplace_back(&RocksDBBatchingProfileMapImpl::worker_loop_, this);
    }
  }

  void
  RocksDBBatchingProfileMapImpl::deactivate_object_()
  {
    queue_cond_.broadcast();
  }

  void
  RocksDBBatchingProfileMapImpl::wait_object_()
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
    }

    notify_failed_operations_(rest, "RocksDBBatchingProfileMapImpl stopped");
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

    if(!enqueue_operation_(std::move(operation)))
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

    if(!enqueue_operation_(std::move(operation)))
    {
      throw ProfileMap<std::string>::Exception(
        "RocksDBBatchingProfileMapImpl::get_profile_async(): "
        "object isn't active");
    }

    return Generics::ConstSmartMemBuf_var();
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

    if(!enqueue_operation_(std::move(operation)))
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

    if(!enqueue_operation_(std::move(operation)))
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
    wait_pending_operations_();

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

  bool
  RocksDBBatchingProfileMapImpl::enqueue_operation_(Operation&& operation) const
  {
    Operations new_operations;
    new_operations.emplace_back(std::move(operation));

    {
      Sync::PosixGuard guard(queue_lock_);
      if(!active())
      {
        return false;
      }

      Operation& operation_ref = new_operations.front();
      operation_ref.sequence = next_operation_sequence_++;

      const bool was_empty =
        read_operations_.empty() && write_operations_.empty();
      Operations& queue = is_write_operation_(operation_ref.type) ?
        write_operations_ :
        read_operations_;
      const bool fills_batch =
        queue.size() + 1 >= batch_size_ && queue.size() < batch_size_;
      queue.splice(queue.end(), new_operations);

      if(!was_empty && !fills_batch)
      {
        return true;
      }
    }

    queue_cond_.signal();
    return true;
  }

  void
  RocksDBBatchingProfileMapImpl::worker_loop_() noexcept
  {
    AdServer::Commons::set_current_thread_name("rocksdb-batching");

    Operations batch;
    BatchScratch scratch;

    while(pop_batch_(batch, scratch))
    {
      try
      {
        process_batch_(batch, scratch);
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
  RocksDBBatchingProfileMapImpl::pop_batch_(
    Operations& batch,
    BatchScratch& scratch) noexcept
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

      collect_batch_(batch, scratch);
      if(!batch.empty())
      {
        const bool write_batch = is_write_operation_(batch.front().type);
        if(write_batch)
        {
          for(const auto& operation : batch)
          {
            in_flight_write_keys_.insert(operation.key);
          }
        }

        if(max_delay_ == Generics::Time::ZERO ||
          batch.size() >= batch_size_)
        {
          ++processing_batches_;
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
          collect_batch_(batch, scratch);
          if(write_batch)
          {
            for(const auto& operation : batch)
            {
              in_flight_write_keys_.insert(operation.key);
            }
          }

          if(batch.size() >= batch_size_ || !signaled)
          {
            break;
          }
        }

        ++processing_batches_;
        return true;
      }

      queue_cond_.wait(queue_lock_);
    }
  }

  void
  RocksDBBatchingProfileMapImpl::collect_batch_(
    Operations& batch,
    BatchScratch& scratch) noexcept
  {
    if(batch.empty())
    {
      scratch.selected_keys.clear();
      scratch.selected_keys.reserve(batch_size_);
    }

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
        batch,
        scratch);
    }
    else if(read_operations_.empty())
    {
      collect_from_queue_(write_operations_, batch, scratch);
    }
    else if(write_operations_.empty())
    {
      collect_from_queue_(read_operations_, batch, scratch);
    }
    else
    {
      Operation& read_head = read_operations_.front();
      Operation& write_head = write_operations_.front();

      if(read_head.sequence < write_head.sequence)
      {
        collect_from_queue_(read_operations_, batch, scratch);
      }
      else
      {
        collect_from_queue_(write_operations_, batch, scratch);
      }
    }
  }

  void
  RocksDBBatchingProfileMapImpl::collect_from_queue_(
    Operations& source,
    Operations& batch,
    BatchScratch& scratch) noexcept
  {
    auto it = source.begin();
    auto& selected_keys = scratch.selected_keys;
    const bool collect_reads = &source == &read_operations_;

    while(it != source.end())
    {
      const std::string_view key(it->key);
      if(collect_reads && in_flight_write_keys_.find(it->key) !=
        in_flight_write_keys_.end())
      {
        ++it;
        continue;
      }

      const auto selected_key_it = std::find(
        selected_keys.begin(),
        selected_keys.end(),
        key);
      if(selected_key_it == selected_keys.end())
      {
        if(selected_keys.size() >= batch_size_)
        {
          ++it;
          continue;
        }

        selected_keys.emplace_back(key);
      }

      auto current = it++;
      batch.splice(batch.end(), source, current);
    }
  }

  void
  RocksDBBatchingProfileMapImpl::complete_batch_(Operations& batch) noexcept
  {
    Sync::PosixGuard guard(queue_lock_);

    if(!batch.empty() && is_write_operation_(batch.front().type))
    {
      for(const auto& operation : batch)
      {
        in_flight_write_keys_.erase(operation.key);
      }
    }

    if(processing_batches_ > 0)
    {
      --processing_batches_;
    }

    queue_cond_.broadcast();
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

    unique_keys.clear();
    key_indexes.clear();
    keys.clear();
    values.clear();

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
    const auto statuses = db_->MultiGet(read_options, keys, &values);

    for(auto& operation : batch)
    {
      try
      {
        const auto key_index_it =
          find_key_index(key_indexes, std::string_view(operation.key));
        const std::size_t key_index = key_index_it->second;
        const auto& status = statuses[key_index];
        const auto& value = values[key_index];

        if(operation.check_callback)
        {
          if(status.IsNotFound())
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
          if(status.IsNotFound())
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
                new Generics::ConstSmartMemBuf(value.data(), value.size())),
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
        latest_operations[it->second] = &operation;
      }
    }

    for(const auto* operation : latest_operations)
    {
      if(operation->type == OT_SAVE)
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

  bool
  RocksDBBatchingProfileMapImpl::is_write_operation_(OperationType type) noexcept
  {
    return type == OT_SAVE || type == OT_REMOVE;
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

  void
  RocksDBBatchingProfileMapImpl::wait_pending_operations_() const
  {
    Sync::PosixGuard guard(queue_lock_);
    while(active() &&
      (!read_operations_.empty() ||
        !write_operations_.empty() ||
        processing_batches_ != 0))
    {
      queue_cond_.wait(queue_lock_);
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
