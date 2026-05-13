#pragma once

#include <atomic>
#include <cstdint>
#include <deque>
#include <list>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <optional>

#include <String/SubString.hpp>

#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Sync/Condition.hpp>
#include <Sync/PosixLock.hpp>
#include <Generics/ActiveObject.hpp>

#include "ProfileMap.hpp"

namespace toplingdb
{
  class DB;
  class SidePluginRepo;
}

namespace AdServer::ProfilingCommons
{
  struct DefaultToplingDBKeyAdapter
  {
    template<typename Type>
    std::string
    operator()(const Type& key) const
    {
      return key;
    }
  };

  class ToplingDBProfileMapImpl:
    public AsyncProfileMap<std::string>,
    public virtual ProfileMap<std::string>,
    public Generics::SimpleActiveObject,
    public ReferenceCounting::AtomicImpl
  {
  public:
    using CheckCallback = std::function<void(bool, std::optional<std::string>)>;
    using GetCallback = std::function<void (
      const Generics::ConstSmartMemBuf_var&,
      std::optional<std::string> error)>;
    using SaveCallback = std::function<void(std::optional<std::string> error)>;

    ToplingDBProfileMapImpl(
      const String::SubString& path,
      const Generics::Time& expire_time,
      unsigned long workers_count = 4,
      unsigned long batch_size = 128,
      const Generics::Time& max_delay = Generics::Time::ZERO,
      bool disable_wal = false);

    ~ToplingDBProfileMapImpl() noexcept override;

    bool
    check_profile(const std::string& key) const override;

    void
    check_profile_async(
      const std::string& key,
      CheckCallback callback) const override;

    Generics::ConstSmartMemBuf_var
    get_profile(
      const std::string& key,
      Generics::Time* last_access_time = 0) override;

    Generics::ConstSmartMemBuf_var
    get_profile_async(
      const std::string& key,
      GetCallback callback,
      std::optional<Generics::Time> last_access_time = std::nullopt
      ) override;

    void
    save_profile(
      const std::string& key,
      const Generics::ConstSmartMemBuf* profile,
      const Generics::Time& now = Generics::Time::get_time_of_day(),
      OperationPriority op_priority = OP_RUNTIME) override;

    void
    save_profile_async(
      const std::string& key,
      const Generics::ConstSmartMemBuf* profile,
      const Generics::Time& now = Generics::Time::get_time_of_day(),
      SaveCallback callback = SaveCallback()) override;

    bool
    remove_profile(
      const std::string& key,
      OperationPriority op_priority = OP_RUNTIME) override;

    unsigned long
    size() const noexcept override;

    unsigned long
    area_size() const noexcept override;

    Stats
    stats() const noexcept override;

    void
    flush();

  protected:
    void activate_object_() override;
    void deactivate_object_() override;
    void wait_object_() override;

  private:
    enum OperationType
    {
      OT_CHECK,
      OT_GET,
      OT_SAVE,
      OT_REMOVE
    };

    struct Operation final
    {
      OperationType type;
      std::uint64_t sequence = 0;
      std::string key;
      Generics::ConstSmartMemBuf_var profile;
      std::optional<CheckCallback> check_callback;
      std::optional<GetCallback> get_callback;
      std::optional<SaveCallback> save_callback;
    };

    using Operations = std::list<Operation>;
    using KeySequences = std::unordered_map<std::string, std::deque<std::uint64_t>>;

    bool enqueue_operation_(Operation&& operation) const;
    void worker_loop_() noexcept;
    bool pop_batch_(Operations& batch) noexcept;
    void collect_batch_(Operations& batch) noexcept;
    void collect_from_queue_(
      Operations& source,
      Operations& batch) noexcept;
    void complete_batch_(Operations& batch) noexcept;
    void process_batch_(Operations& batch);

    void process_read_batch_(Operations& batch);
    void process_write_batch_(Operations& batch);

    void notify_failed_operations_(
      Operations& operations,
      const std::string& error) noexcept;

    bool direct_check_profile_(const std::string& key) const;
    static bool is_write_operation_(OperationType type) noexcept;

    void check_background_error_() const;

  private:
    const std::string path_;
    const Generics::Time expire_time_;
    const unsigned long workers_count_;
    const unsigned long batch_size_;
    const Generics::Time max_delay_;
    const bool disable_wal_;

    toplingdb::DB* db_ = nullptr;
    std::unique_ptr<toplingdb::SidePluginRepo> plugin_repo_;
    mutable std::atomic<std::uint64_t> logical_read_operations_{0};
    mutable std::atomic<std::uint64_t> logical_write_operations_{0};
    mutable std::atomic<std::uint64_t> physical_read_operations_{0};
    mutable std::atomic<std::uint64_t> physical_write_operations_{0};

    mutable Sync::PosixMutex queue_lock_;
    mutable Sync::Conditional queue_cond_;
    mutable Operations read_operations_;
    mutable Operations write_operations_;
    mutable std::unordered_set<std::string> in_flight_keys_;
    mutable KeySequences key_sequences_;
    mutable std::uint64_t next_operation_sequence_ = 0;

    mutable Sync::PosixMutex error_lock_;
    std::string background_error_;

    std::vector<std::thread> workers_;
  };

  template<typename KeyType, typename KeyAdapterType = DefaultToplingDBKeyAdapter>
  class ToplingDBProfileMap:
    public AsyncProfileMap<KeyType>,
    public virtual ProfileMap<KeyType>,
    public ReferenceCounting::AtomicImpl
  {
  public:
    ToplingDBProfileMap(
      const String::SubString& path,
      const Generics::Time& expire_time,
      unsigned long workers_count = 4,
      unsigned long batch_size = 128,
      const Generics::Time& max_delay = Generics::Time::ZERO,
      bool disable_wal = false);

    bool
    check_profile(const KeyType& key) const override;

    void
    check_profile_async(
      const KeyType& key,
      std::function<void(bool, std::optional<std::string>)> callback) const override;

    Generics::ConstSmartMemBuf_var
    get_profile(
      const KeyType& key,
      Generics::Time* last_access_time = 0) override;

    Generics::ConstSmartMemBuf_var
    get_profile_async(
      const KeyType& key,
      std::function<void (
        const Generics::ConstSmartMemBuf_var&,
        std::optional<std::string> error)> callback,
      std::optional<Generics::Time> last_access_time = std::nullopt) override;

    void
    save_profile(
      const KeyType& key,
      const Generics::ConstSmartMemBuf* profile,
      const Generics::Time& now = Generics::Time::get_time_of_day(),
      OperationPriority op_priority = OP_RUNTIME) override;

    void
    save_profile_async(
      const KeyType& key,
      const Generics::ConstSmartMemBuf* profile,
      const Generics::Time& now = Generics::Time::get_time_of_day(),
      typename AsyncProfileMap<KeyType>::SaveCallback callback =
        typename AsyncProfileMap<KeyType>::SaveCallback()) override;

    bool
    remove_profile(
      const KeyType& key,
      OperationPriority op_priority = OP_RUNTIME) override;

    unsigned long
    size() const noexcept override;

    unsigned long
    area_size() const noexcept override;

    typename ProfileMap<KeyType>::Stats
    stats() const noexcept override;

    void flush();

    void activate_object();
    void deactivate_object();
    void wait_object();

  private:
    KeyAdapterType key_adapter_;
    std::shared_ptr<ToplingDBProfileMapImpl> impl_;
  };
}

namespace AdServer::ProfilingCommons
{
  template<typename KeyType, typename KeyAdapterType>
  ToplingDBProfileMap<KeyType, KeyAdapterType>::ToplingDBProfileMap(
    const String::SubString& path,
    const Generics::Time& expire_time,
    unsigned long workers_count,
    unsigned long batch_size,
    const Generics::Time& max_delay,
    bool disable_wal)
    : impl_(new ToplingDBProfileMapImpl(
        path,
        expire_time,
        workers_count,
        batch_size,
        max_delay,
        disable_wal))
  {}

  template<typename KeyType, typename KeyAdapterType>
  bool
  ToplingDBProfileMap<KeyType, KeyAdapterType>::check_profile(const KeyType& key) const
  {
    return impl_->check_profile(key_adapter_(key));
  }

  template<typename KeyType, typename KeyAdapterType>
  void
  ToplingDBProfileMap<KeyType, KeyAdapterType>::check_profile_async(
    const KeyType& key,
    std::function<void(bool, std::optional<std::string>)> callback) const
  {
    impl_->check_profile_async(key_adapter_(key), std::move(callback));
  }

  template<typename KeyType, typename KeyAdapterType>
  Generics::ConstSmartMemBuf_var
  ToplingDBProfileMap<KeyType, KeyAdapterType>::get_profile(
    const KeyType& key,
    Generics::Time* last_access_time)
  {
    return impl_->get_profile(key_adapter_(key), last_access_time);
  }

  template<typename KeyType, typename KeyAdapterType>
  Generics::ConstSmartMemBuf_var
  ToplingDBProfileMap<KeyType, KeyAdapterType>::get_profile_async(
    const KeyType& key,
    std::function<void (
      const Generics::ConstSmartMemBuf_var&,
      std::optional<std::string> error)> callback,
    std::optional<Generics::Time> last_access_time)
  {
    return impl_->get_profile_async(
      key_adapter_(key),
      std::move(callback),
      last_access_time);
  }

  template<typename KeyType, typename KeyAdapterType>
  void
  ToplingDBProfileMap<KeyType, KeyAdapterType>::save_profile(
    const KeyType& key,
    const Generics::ConstSmartMemBuf* profile,
    const Generics::Time& now,
    OperationPriority op_priority)
  {
    impl_->save_profile(key_adapter_(key), profile, now, op_priority);
  }

  template<typename KeyType, typename KeyAdapterType>
  void
  ToplingDBProfileMap<KeyType, KeyAdapterType>::save_profile_async(
    const KeyType& key,
    const Generics::ConstSmartMemBuf* profile,
    const Generics::Time& now,
    typename AsyncProfileMap<KeyType>::SaveCallback callback)
  {
    impl_->save_profile_async(key_adapter_(key), profile, now, std::move(callback));
  }

  template<typename KeyType, typename KeyAdapterType>
  bool
  ToplingDBProfileMap<KeyType, KeyAdapterType>::remove_profile(
    const KeyType& key,
    OperationPriority op_priority)
  {
    return impl_->remove_profile(key_adapter_(key), op_priority);
  }

  template<typename KeyType, typename KeyAdapterType>
  unsigned long
  ToplingDBProfileMap<KeyType, KeyAdapterType>::size() const noexcept
  {
    return impl_->size();
  }

  template<typename KeyType, typename KeyAdapterType>
  unsigned long
  ToplingDBProfileMap<KeyType, KeyAdapterType>::area_size() const noexcept
  {
    return impl_->area_size();
  }

  template<typename KeyType, typename KeyAdapterType>
  typename ProfileMap<KeyType>::Stats
  ToplingDBProfileMap<KeyType, KeyAdapterType>::stats() const noexcept
  {
    const auto stats = impl_->stats();
    return {
      stats.logical_read_operations,
      stats.logical_write_operations,
      stats.physical_read_operations,
      stats.physical_write_operations
    };
  }

  template<typename KeyType, typename KeyAdapterType>
  void
  ToplingDBProfileMap<KeyType, KeyAdapterType>::flush()
  {
    impl_->flush();
  }

  template<typename KeyType, typename KeyAdapterType>
  void
  ToplingDBProfileMap<KeyType, KeyAdapterType>::activate_object()
  {
    impl_->activate_object();
  }

  template<typename KeyType, typename KeyAdapterType>
  void
  ToplingDBProfileMap<KeyType, KeyAdapterType>::deactivate_object()
  {
    impl_->deactivate_object();
  }

  template<typename KeyType, typename KeyAdapterType>
  void
  ToplingDBProfileMap<KeyType, KeyAdapterType>::wait_object()
  {
    impl_->wait_object();
  }
}
