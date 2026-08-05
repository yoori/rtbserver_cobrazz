#pragma once

#include <atomic>
#include <cstdint>
#include <list>
#include <memory>
#include <optional>
#include <string>

#include <String/SubString.hpp>

#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Sync/PosixLock.hpp>
#include <Generics/ActiveObject.hpp>

#include "ProfileMap.hpp"

namespace rocksdb
{
  class DBWithTTL;
}

namespace AdServer::ProfilingCommons
{
  class RocksDBProfileMapProcessor;
  struct RocksDBProfileMapProcessorQueue;

  struct DefaultRocksDBBatchingKeyAdapter
  {
    template<typename Type>
    std::string
    operator()(const Type& key) const
    {
      return key;
    }

    template<typename Type>
    Type
    key_from_string(const std::string& key) const
    {
      return key;
    }
  };

  class RocksDBBatchingProfileMapImpl:
    public AsyncProfileMap<std::string>,
    public virtual ProfileMap<std::string>,
    public Generics::SimpleActiveObject,
    public ReferenceCounting::AtomicImpl
  {
  public:
    using CheckCallback = std::function<void(bool, std::optional<std::string>)>;
    using GetCallback = std::function<void (
      Generics::ConstSmartMemBuf_var,
      std::optional<std::string> error)>;
    using GetOwnCallback = std::function<void (
      Generics::SmartMemBuf_var,
      std::optional<std::string> error)>;
    using SaveCallback = std::function<void(std::optional<std::string> error)>;
    using RemoveCallback = std::function<void(
      bool,
      std::optional<std::string> error)>;
    using CompleteCallback = std::function<void()>;

    RocksDBBatchingProfileMapImpl(
      const String::SubString& path,
      const Generics::Time& expire_time,
      unsigned long workers_count = 2,
      unsigned long batch_size = 128,
      const Generics::Time& max_delay = Generics::Time::ZERO,
      bool disable_wal = false);

    RocksDBBatchingProfileMapImpl(
      std::shared_ptr<RocksDBProfileMapProcessor> processor,
      const String::SubString& path,
      const Generics::Time& expire_time,
      unsigned long batch_size = 128,
      const Generics::Time& max_delay = Generics::Time::ZERO,
      bool disable_wal = false);

    ~RocksDBBatchingProfileMapImpl() noexcept override;

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

    Generics::SmartMemBuf_var
    get_own_profile(
      const std::string& key,
      Generics::Time* last_access_time = 0) override;

    Generics::ConstSmartMemBuf_var
    get_profile_async(
      const std::string& key,
      GetCallback callback,
      std::optional<Generics::Time> last_access_time = std::nullopt
      ) override;

    Generics::SmartMemBuf_var
    get_own_profile_async(
      const std::string& key,
      GetOwnCallback callback,
      std::optional<Generics::Time> last_access_time = std::nullopt) override;

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

    void
    remove_profile_async(
      const std::string& key,
      OperationPriority op_priority = OP_RUNTIME,
      RemoveCallback callback = RemoveCallback()) override;

    void
    clear_expired_async(
      const Generics::Time& expire_time,
      CompleteCallback complete = CompleteCallback()) override;

    void
    process_keys(
      std::function<void(const std::string&)> process_key,
      std::function<void(void)> process_complete)
      /*throw(Exception)*/ override;

    unsigned long
    size() const noexcept override;

    unsigned long
    area_size() const noexcept override;

    Stats
    stats() const noexcept override;

  protected:
    void activate_object_() override;
    void deactivate_object_() override;
    void wait_object_() override;

  private:
    friend class RocksDBProfileMapProcessor;
    friend struct RocksDBProfileMapProcessorQueue;

    enum OperationType
    {
      OT_CHECK,
      OT_GET,
      OT_TOUCH,
      OT_SAVE,
      OT_REMOVE
    };

    struct Operation final
    {
      OperationType type;
      Generics::Time enqueue_time;
      std::string key;
      Generics::ConstSmartMemBuf_var profile;
      std::optional<CheckCallback> check_callback;
      std::optional<GetCallback> get_callback;
      std::optional<GetOwnCallback> get_own_callback;
      std::optional<SaveCallback> save_callback;
      std::optional<RemoveCallback> remove_callback;
    };

    using Operations = std::list<Operation>;
    struct BatchScratch;

    static std::shared_ptr<BatchScratch> create_batch_scratch_();

    void process_batch_(Operations& batch, BatchScratch& scratch);

    void process_read_batch_(Operations& batch, BatchScratch& scratch);

    void process_write_batch_(
      Operations& batch,
      BatchScratch& scratch);

    void notify_failed_operations_(
      Operations& operations,
      const std::string& error) noexcept;

    bool direct_check_profile_(const std::string& key) const;
    static bool is_write_operation_(OperationType type) noexcept;

    void check_background_error_() const;

  private:
    const std::string path_;
    const Generics::Time expire_time_;
    const unsigned long batch_size_;
    const Generics::Time max_delay_;
    const bool disable_wal_;

    const std::shared_ptr<RocksDBProfileMapProcessor> processor_;
    std::shared_ptr<RocksDBProfileMapProcessorQueue> processor_queue_;
    bool owns_processor_;

    std::unique_ptr<rocksdb::DBWithTTL> db_;
    mutable std::atomic<std::uint64_t> logical_read_operations_{0};
    mutable std::atomic<std::uint64_t> logical_write_operations_{0};
    mutable std::atomic<std::uint64_t> physical_read_operations_{0};
    mutable std::atomic<std::uint64_t> physical_write_operations_{0};

    mutable Sync::PosixMutex error_lock_;
    std::string background_error_;
  };

  inline bool
  RocksDBBatchingProfileMapImpl::is_write_operation_(
    OperationType type) noexcept
  {
    return type == OT_TOUCH || type == OT_SAVE || type == OT_REMOVE;
  }

  template<typename KeyType, typename KeyAdapterType = DefaultRocksDBBatchingKeyAdapter>
  class RocksDBBatchingProfileMap:
    public AsyncProfileMap<KeyType>,
    public virtual ProfileMap<KeyType>,
    public virtual Generics::RefCountableActiveObject,
    public ReferenceCounting::AtomicImpl
  {
  public:
    RocksDBBatchingProfileMap(
      const String::SubString& path,
      const Generics::Time& expire_time,
      unsigned long workers_count = 2,
      unsigned long batch_size = 128,
      const Generics::Time& max_delay = Generics::Time::ZERO,
      bool disable_wal = false);

    RocksDBBatchingProfileMap(
      std::shared_ptr<RocksDBProfileMapProcessor> processor,
      const String::SubString& path,
      const Generics::Time& expire_time,
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

    Generics::SmartMemBuf_var
    get_own_profile(
      const KeyType& key,
      Generics::Time* last_access_time = 0) override;

    Generics::ConstSmartMemBuf_var
    get_profile_async(
      const KeyType& key,
      std::function<void (
        Generics::ConstSmartMemBuf_var,
        std::optional<std::string> error)> callback,
      std::optional<Generics::Time> last_access_time = std::nullopt) override;

    Generics::SmartMemBuf_var
    get_own_profile_async(
      const KeyType& key,
      typename AsyncProfileMap<KeyType>::GetOwnCallback callback,
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

    void
    remove_profile_async(
      const KeyType& key,
      OperationPriority op_priority = OP_RUNTIME,
      typename AsyncProfileMap<KeyType>::RemoveCallback callback =
        typename AsyncProfileMap<KeyType>::RemoveCallback()) override;

    void
    clear_expired_async(
      const Generics::Time& expire_time,
      typename AsyncProfileMap<KeyType>::CompleteCallback complete =
        typename AsyncProfileMap<KeyType>::CompleteCallback()) override;

    void
    process_keys(
      std::function<void(const KeyType&)> process_key,
      std::function<void(void)> process_complete)
      /*throw(Exception)*/ override;

    unsigned long
    size() const noexcept override;

    unsigned long
    area_size() const noexcept override;

    typename ProfileMap<KeyType>::Stats
    stats() const noexcept override;

    void activate_object();
    void deactivate_object();
    void wait_object();
    bool active() const;

  private:
    KeyAdapterType key_adapter_;
    std::shared_ptr<RocksDBBatchingProfileMapImpl> impl_;
  };
}

namespace AdServer::ProfilingCommons
{
  template<typename KeyType, typename KeyAdapterType>
  RocksDBBatchingProfileMap<KeyType, KeyAdapterType>::RocksDBBatchingProfileMap(
    const String::SubString& path,
    const Generics::Time& expire_time,
    unsigned long workers_count,
    unsigned long batch_size,
    const Generics::Time& max_delay,
    bool disable_wal)
    : impl_(new RocksDBBatchingProfileMapImpl(
        path,
        expire_time,
        workers_count,
        batch_size,
        max_delay,
        disable_wal))
  {}

  template<typename KeyType, typename KeyAdapterType>
  RocksDBBatchingProfileMap<KeyType, KeyAdapterType>::RocksDBBatchingProfileMap(
    std::shared_ptr<RocksDBProfileMapProcessor> processor,
    const String::SubString& path,
    const Generics::Time& expire_time,
    unsigned long batch_size,
    const Generics::Time& max_delay,
    bool disable_wal)
    : impl_(new RocksDBBatchingProfileMapImpl(
        std::move(processor),
        path,
        expire_time,
        batch_size,
        max_delay,
        disable_wal))
  {}

  template<typename KeyType, typename KeyAdapterType>
  bool
  RocksDBBatchingProfileMap<KeyType, KeyAdapterType>::check_profile(const KeyType& key) const
  {
    return impl_->check_profile(key_adapter_(key));
  }

  template<typename KeyType, typename KeyAdapterType>
  void
  RocksDBBatchingProfileMap<KeyType, KeyAdapterType>::check_profile_async(
    const KeyType& key,
    std::function<void(bool, std::optional<std::string>)> callback) const
  {
    impl_->check_profile_async(key_adapter_(key), std::move(callback));
  }

  template<typename KeyType, typename KeyAdapterType>
  Generics::ConstSmartMemBuf_var
  RocksDBBatchingProfileMap<KeyType, KeyAdapterType>::get_profile(
    const KeyType& key,
    Generics::Time* last_access_time)
  {
    return impl_->get_profile(key_adapter_(key), last_access_time);
  }

  template<typename KeyType, typename KeyAdapterType>
  Generics::SmartMemBuf_var
  RocksDBBatchingProfileMap<KeyType, KeyAdapterType>::get_own_profile(
    const KeyType& key,
    Generics::Time* last_access_time)
  {
    return impl_->get_own_profile(key_adapter_(key), last_access_time);
  }

  template<typename KeyType, typename KeyAdapterType>
  Generics::ConstSmartMemBuf_var
  RocksDBBatchingProfileMap<KeyType, KeyAdapterType>::get_profile_async(
    const KeyType& key,
    std::function<void (
      Generics::ConstSmartMemBuf_var,
      std::optional<std::string> error)> callback,
    std::optional<Generics::Time> last_access_time)
  {
    return impl_->get_profile_async(
      key_adapter_(key),
      std::move(callback),
      last_access_time);
  }

  template<typename KeyType, typename KeyAdapterType>
  Generics::SmartMemBuf_var
  RocksDBBatchingProfileMap<KeyType, KeyAdapterType>::get_own_profile_async(
    const KeyType& key,
    typename AsyncProfileMap<KeyType>::GetOwnCallback callback,
    std::optional<Generics::Time> last_access_time)
  {
    return impl_->get_own_profile_async(
      key_adapter_(key),
      std::move(callback),
      last_access_time);
  }

  template<typename KeyType, typename KeyAdapterType>
  void
  RocksDBBatchingProfileMap<KeyType, KeyAdapterType>::save_profile(
    const KeyType& key,
    const Generics::ConstSmartMemBuf* profile,
    const Generics::Time& now,
    OperationPriority op_priority)
  {
    impl_->save_profile(key_adapter_(key), profile, now, op_priority);
  }

  template<typename KeyType, typename KeyAdapterType>
  void
  RocksDBBatchingProfileMap<KeyType, KeyAdapterType>::save_profile_async(
    const KeyType& key,
    const Generics::ConstSmartMemBuf* profile,
    const Generics::Time& now,
    typename AsyncProfileMap<KeyType>::SaveCallback callback)
  {
    impl_->save_profile_async(key_adapter_(key), profile, now, std::move(callback));
  }

  template<typename KeyType, typename KeyAdapterType>
  bool
  RocksDBBatchingProfileMap<KeyType, KeyAdapterType>::remove_profile(
    const KeyType& key,
    OperationPriority op_priority)
  {
    return impl_->remove_profile(key_adapter_(key), op_priority);
  }

  template<typename KeyType, typename KeyAdapterType>
  void
  RocksDBBatchingProfileMap<KeyType, KeyAdapterType>::remove_profile_async(
    const KeyType& key,
    OperationPriority op_priority,
    typename AsyncProfileMap<KeyType>::RemoveCallback callback)
  {
    impl_->remove_profile_async(
      key_adapter_(key),
      op_priority,
      std::move(callback));
  }

  template<typename KeyType, typename KeyAdapterType>
  void
  RocksDBBatchingProfileMap<KeyType, KeyAdapterType>::clear_expired_async(
    const Generics::Time& expire_time,
    typename AsyncProfileMap<KeyType>::CompleteCallback complete)
  {
    impl_->clear_expired_async(expire_time, std::move(complete));
  }

  template<typename KeyType, typename KeyAdapterType>
  void
  RocksDBBatchingProfileMap<KeyType, KeyAdapterType>::process_keys(
    std::function<void(const KeyType&)> process_key,
    std::function<void(void)> process_complete)
    /*throw(Exception)*/
  {
    impl_->process_keys(
      [this, &process_key](const std::string& key)
      {
        process_key(key_adapter_.template key_from_string<KeyType>(key));
      },
      std::move(process_complete));
  }

  template<typename KeyType, typename KeyAdapterType>
  unsigned long
  RocksDBBatchingProfileMap<KeyType, KeyAdapterType>::size() const noexcept
  {
    return impl_->size();
  }

  template<typename KeyType, typename KeyAdapterType>
  unsigned long
  RocksDBBatchingProfileMap<KeyType, KeyAdapterType>::area_size() const noexcept
  {
    return impl_->area_size();
  }

  template<typename KeyType, typename KeyAdapterType>
  typename ProfileMap<KeyType>::Stats
  RocksDBBatchingProfileMap<KeyType, KeyAdapterType>::stats() const noexcept
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
  RocksDBBatchingProfileMap<KeyType, KeyAdapterType>::activate_object()
  {
    impl_->activate_object();
  }

  template<typename KeyType, typename KeyAdapterType>
  void
  RocksDBBatchingProfileMap<KeyType, KeyAdapterType>::deactivate_object()
  {
    impl_->deactivate_object();
  }

  template<typename KeyType, typename KeyAdapterType>
  void
  RocksDBBatchingProfileMap<KeyType, KeyAdapterType>::wait_object()
  {
    impl_->wait_object();
  }

  template<typename KeyType, typename KeyAdapterType>
  bool
  RocksDBBatchingProfileMap<KeyType, KeyAdapterType>::active() const
  {
    return impl_->active();
  }
}
