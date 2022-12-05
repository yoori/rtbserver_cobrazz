#ifndef ROCKSDBPROFILEMAP_HPP_
#define ROCKSDBPROFILEMAP_HPP_

#include <memory>
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
    public ProfileMap<std::string>,
    public ReferenceCounting::AtomicImpl
  {
  public:
    RocksDBProfileMapImpl(
      const String::SubString& path,
      const Generics::Time& expire_time);

    virtual
    ~RocksDBProfileMapImpl() noexcept;

    virtual bool
    check_profile(const std::string& key) const;

    virtual Generics::ConstSmartMemBuf_var
    get_profile(
      const std::string& key,
      Generics::Time* last_access_time = 0);

    virtual void
    save_profile(
      const std::string& key,
      const Generics::ConstSmartMemBuf* profile,
      const Generics::Time& now = Generics::Time::get_time_of_day(),
      OperationPriority op_priority = OP_RUNTIME);

    virtual bool
    remove_profile(
      const std::string& key,
      OperationPriority op_priority = OP_RUNTIME);

    virtual unsigned long
    size() const noexcept;

    virtual unsigned long
    area_size() const noexcept;

  private:
    const std::string path_;
    rocksdb::DBWithTTL* db_;
  };

  struct DefaultKeyStringAdapter
  {
    template<typename Type>
    std::string
    operator()(const Type& key)
    {
      return key;
    }
  };

  template<typename KeyType, typename KeyAdapterType = DefaultKeyStringAdapter>
  class RocksDBProfileMap:
    public ProfileMap<KeyType>,
    public ReferenceCounting::AtomicImpl
  {
  public:
    RocksDBProfileMap(
      const String::SubString& path,
      const Generics::Time& expire_time);

    virtual bool
    check_profile(const KeyType& key) const;

    virtual Generics::ConstSmartMemBuf_var
    get_profile(
      const KeyType& key,
      Generics::Time* last_access_time = 0);

    virtual void
    save_profile(
      const KeyType& key,
      const Generics::ConstSmartMemBuf* profile,
      const Generics::Time& now = Generics::Time::get_time_of_day(),
      OperationPriority op_priority = OP_RUNTIME);

    virtual bool
    remove_profile(
      const KeyType& key,
      OperationPriority op_priority = OP_RUNTIME);

    virtual unsigned long
    size() const noexcept;

    virtual unsigned long
    area_size() const noexcept;

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
    const Generics::Time& expire_time)
    : impl_(new RocksDBProfileMapImpl(path, expire_time))
  {}

  template<typename KeyType, typename KeyAdapterType>
  bool
  RocksDBProfileMap<KeyType, KeyAdapterType>::check_profile(const KeyType& key)
    const
  {
    return impl_->check_profile(key_adapter_(key));
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
  bool
  RocksDBProfileMap<KeyType, KeyAdapterType>::remove_profile(
    const KeyType& key,
    OperationPriority op_priority)
  {
    return impl_->remove_profile(key_adapter_(key), op_priority);
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
}
}

#endif /*ROCKSDBPROFILEMAP_HPP_*/
