#pragma once

#include <atomic>
#include <memory>
#include <optional>
#include <utility>
#include <vector>
#include <String/SubString.hpp>

#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>

#include "ProfileMap.hpp"

namespace rocksdb
{
  class DBWithTTL;
}

namespace AdServer
{
namespace ProfilingCommons
{
  /**
   * class RocksDBProfileMap
   */
  class RocksDBProfileMapImpl:
    public AsyncProfileMap<std::string>,
    public ProfileMap<std::string>,
    public ReferenceCounting::AtomicImpl
  {
  public:
    RocksDBProfileMapImpl(
      const String::SubString& path,
      const Generics::Time& expire_time,
      bool disable_wal = false);

    virtual
    ~RocksDBProfileMapImpl() noexcept;

    virtual bool
    check_profile(const std::string& key) const;

    virtual void
    check_profile_async(
      const std::string& key,
      CheckCallback callback) const;

    virtual Generics::ConstSmartMemBuf_var
    get_profile(
      const std::string& key,
      Generics::Time* last_access_time = 0);

    virtual Generics::SmartMemBuf_var
    get_own_profile(
      const std::string& key,
      Generics::Time* last_access_time = 0);

    virtual Generics::ConstSmartMemBuf_var
    get_profile_async(
      const std::string& key,
      GetCallback callback,
      std::optional<Generics::Time> last_access_time = std::nullopt);

    virtual Generics::SmartMemBuf_var
    get_own_profile_async(
      const std::string& key,
      GetOwnCallback callback,
      std::optional<Generics::Time> last_access_time = std::nullopt);

    virtual void
    save_profile(
      const std::string& key,
      const Generics::ConstSmartMemBuf* profile,
      const Generics::Time& now = Generics::Time::get_time_of_day(),
      OperationPriority op_priority = OP_RUNTIME);

    virtual void
    save_profile_async(
      const std::string& key,
      const Generics::ConstSmartMemBuf* profile,
      const Generics::Time& now = Generics::Time::get_time_of_day(),
      SaveCallback callback = SaveCallback());

    virtual bool
    remove_profile(
      const std::string& key,
      OperationPriority op_priority = OP_RUNTIME);

    virtual void
    remove_profile_async(
      const std::string& key,
      OperationPriority op_priority = OP_RUNTIME,
      RemoveCallback callback = RemoveCallback());

    virtual void
    clear_expired_async(
      const Generics::Time& expire_time,
      CompleteCallback complete = CompleteCallback());

    virtual void
    process_keys(
      std::function<void(const std::string&)> process_key,
      std::function<void(void)> process_complete)
      /*throw(Exception)*/;

    virtual unsigned long
    size() const noexcept;

    virtual unsigned long
    area_size() const noexcept;

    Stats
    stats() const noexcept override;

    void
    flush();

  private:
    const std::string path_;
    rocksdb::DBWithTTL* db_;
    const bool disable_wal_;
    mutable std::atomic<std::uint64_t> logical_read_operations_{0};
    mutable std::atomic<std::uint64_t> logical_write_operations_{0};
    mutable std::atomic<std::uint64_t> physical_read_operations_{0};
    mutable std::atomic<std::uint64_t> physical_write_operations_{0};
  };

  struct DefaultKeyStringAdapter
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

  template<typename KeyType, typename KeyAdapterType = DefaultKeyStringAdapter>
  class RocksDBProfileMap:
    public AsyncProfileMap<KeyType>,
    public ProfileMap<KeyType>,
    public ReferenceCounting::AtomicImpl
  {
  public:
    RocksDBProfileMap(
      const String::SubString& path,
      const Generics::Time& expire_time,
      bool disable_wal = false);

    virtual bool
    check_profile(const KeyType& key) const;

    virtual void
    check_profile_async(
      const KeyType& key,
      typename AsyncProfileMap<KeyType>::CheckCallback callback) const;

    virtual Generics::ConstSmartMemBuf_var
    get_profile(
      const KeyType& key,
      Generics::Time* last_access_time = 0);

    virtual Generics::SmartMemBuf_var
    get_own_profile(
      const KeyType& key,
      Generics::Time* last_access_time = 0);

    virtual Generics::ConstSmartMemBuf_var
    get_profile_async(
      const KeyType& key,
      typename AsyncProfileMap<KeyType>::GetCallback callback,
      std::optional<Generics::Time> last_access_time = std::nullopt);

    virtual Generics::SmartMemBuf_var
    get_own_profile_async(
      const KeyType& key,
      typename AsyncProfileMap<KeyType>::GetOwnCallback callback,
      std::optional<Generics::Time> last_access_time = std::nullopt);

    virtual void
    save_profile(
      const KeyType& key,
      const Generics::ConstSmartMemBuf* profile,
      const Generics::Time& now = Generics::Time::get_time_of_day(),
      OperationPriority op_priority = OP_RUNTIME);

    virtual void
    save_profile_async(
      const KeyType& key,
      const Generics::ConstSmartMemBuf* profile,
      const Generics::Time& now = Generics::Time::get_time_of_day(),
      typename AsyncProfileMap<KeyType>::SaveCallback callback =
        typename AsyncProfileMap<KeyType>::SaveCallback());

    virtual bool
    remove_profile(
      const KeyType& key,
      OperationPriority op_priority = OP_RUNTIME);

    virtual void
    remove_profile_async(
      const KeyType& key,
      OperationPriority op_priority = OP_RUNTIME,
      typename AsyncProfileMap<KeyType>::RemoveCallback callback =
        typename AsyncProfileMap<KeyType>::RemoveCallback());

    virtual void
    clear_expired_async(
      const Generics::Time& expire_time,
      typename AsyncProfileMap<KeyType>::CompleteCallback complete =
        typename AsyncProfileMap<KeyType>::CompleteCallback());

    virtual void
    process_keys(
      std::function<void(const KeyType&)> process_key,
      std::function<void(void)> process_complete)
      /*throw(Exception)*/;

    virtual unsigned long
    size() const noexcept;

    virtual unsigned long
    area_size() const noexcept;

    typename ProfileMap<KeyType>::Stats
    stats() const noexcept override;

    void
    flush();

  private:
    KeyAdapterType key_adapter_;
    std::shared_ptr<RocksDBProfileMapImpl> impl_;
  };
}
}

namespace AdServer
{
namespace ProfilingCommons
{
  template<typename KeyType, typename KeyAdapterType>
  RocksDBProfileMap<KeyType, KeyAdapterType>::RocksDBProfileMap(
    const String::SubString& path,
    const Generics::Time& expire_time,
    bool disable_wal)
    : impl_(new RocksDBProfileMapImpl(path, expire_time, disable_wal))
  {}

  template<typename KeyType, typename KeyAdapterType>
  bool
  RocksDBProfileMap<KeyType, KeyAdapterType>::check_profile(const KeyType& key)
    const
  {
    return impl_->check_profile(key_adapter_(key));
  }

  template<typename KeyType, typename KeyAdapterType>
  void
  RocksDBProfileMap<KeyType, KeyAdapterType>::check_profile_async(
    const KeyType& key,
    typename AsyncProfileMap<KeyType>::CheckCallback callback) const
  {
    impl_->check_profile_async(key_adapter_(key), std::move(callback));
  }

  template<typename KeyType, typename KeyAdapterType>
  Generics::ConstSmartMemBuf_var
  RocksDBProfileMap<KeyType, KeyAdapterType>::get_profile(
    const KeyType& key,
    Generics::Time* last_access_time)
  {
    return impl_->get_profile(key_adapter_(key), last_access_time);
  }

  template<typename KeyType, typename KeyAdapterType>
  Generics::SmartMemBuf_var
  RocksDBProfileMap<KeyType, KeyAdapterType>::get_own_profile(
    const KeyType& key,
    Generics::Time* last_access_time)
  {
    return impl_->get_own_profile(key_adapter_(key), last_access_time);
  }

  template<typename KeyType, typename KeyAdapterType>
  Generics::ConstSmartMemBuf_var
  RocksDBProfileMap<KeyType, KeyAdapterType>::get_profile_async(
    const KeyType& key,
    typename AsyncProfileMap<KeyType>::GetCallback callback,
    std::optional<Generics::Time> last_access_time)
  {
    return impl_->get_profile_async(
      key_adapter_(key),
      std::move(callback),
      last_access_time);
  }

  template<typename KeyType, typename KeyAdapterType>
  Generics::SmartMemBuf_var
  RocksDBProfileMap<KeyType, KeyAdapterType>::get_own_profile_async(
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
  RocksDBProfileMap<KeyType, KeyAdapterType>::save_profile(
    const KeyType& key,
    const Generics::ConstSmartMemBuf* profile,
    const Generics::Time& now,
    OperationPriority op_priority)
  {
    impl_->save_profile(
      key_adapter_(key),
      profile,
      now,
      op_priority);
  }

  template<typename KeyType, typename KeyAdapterType>
  void
  RocksDBProfileMap<KeyType, KeyAdapterType>::save_profile_async(
    const KeyType& key,
    const Generics::ConstSmartMemBuf* profile,
    const Generics::Time& now,
    typename AsyncProfileMap<KeyType>::SaveCallback callback)
  {
    impl_->save_profile_async(
      key_adapter_(key),
      profile,
      now,
      std::move(callback));
  }

  template<typename KeyType, typename KeyAdapterType>
  bool
  RocksDBProfileMap<KeyType, KeyAdapterType>::remove_profile(
    const KeyType& key,
    OperationPriority op_priority)
  {
    return impl_->remove_profile(key_adapter_(key), op_priority);
  }

  template<typename KeyType, typename KeyAdapterType>
  void
  RocksDBProfileMap<KeyType, KeyAdapterType>::remove_profile_async(
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
  RocksDBProfileMap<KeyType, KeyAdapterType>::clear_expired_async(
    const Generics::Time& expire_time,
    typename AsyncProfileMap<KeyType>::CompleteCallback complete)
  {
    impl_->clear_expired_async(expire_time, std::move(complete));
  }

  template<typename KeyType, typename KeyAdapterType>
  void
  RocksDBProfileMap<KeyType, KeyAdapterType>::process_keys(
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
  RocksDBProfileMap<KeyType, KeyAdapterType>::size() const noexcept
  {
    return impl_->size();
  }

  template<typename KeyType, typename KeyAdapterType>
  unsigned long
  RocksDBProfileMap<KeyType, KeyAdapterType>::area_size() const noexcept
  {
    return impl_->area_size();
  }

  template<typename KeyType, typename KeyAdapterType>
  typename ProfileMap<KeyType>::Stats
  RocksDBProfileMap<KeyType, KeyAdapterType>::stats() const noexcept
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
  RocksDBProfileMap<KeyType, KeyAdapterType>::flush()
  {
    impl_->flush();
  }
}
}
