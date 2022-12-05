#ifndef BOUNDEDCACHE_HPP
#define BOUNDEDCACHE_HPP

#include <eh/Exception.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <Generics/BoundedMap.hpp>
#include <Commons/LockMap.hpp>

namespace AdServer
{
  /* BoundedCacheConfiguration requirements:
   *   Exception
   *
   *   Holder
   *
   *   bool update_required(const KeyType& key, const Holder& val) noexcept
   *   Holder update(const KeyType& key)
   *   size_type size(const Holder&) const noexcept
   *   ValueType adapt(const Holder&) const noexcept
   */
  template<
    typename KeyType,
    typename ValueType,
    typename ConfigurationType,
    template <typename, typename> class MapType = ReferenceCounting::HashTable>
  class BoundedCache: public ReferenceCounting::AtomicImpl
  {
  public:
    BoundedCache(
      unsigned long bound,
      const Generics::Time& timeout,
      const ConfigurationType& configuration = ConfigurationType())
      /*throw(eh::Exception)*/;

    ValueType
    get(const KeyType& key)
      /*throw(typename ConfigurationType::Exception)*/;

  private:
    typedef typename ConfigurationType::Holder Holder;

    class SizePolicy
    {
    public:
      SizePolicy(const ConfigurationType& conf)
        : configuration_(conf)
      {}

      size_t operator()(const KeyType&, const Holder& holder) const noexcept
      {
        return configuration_.size(holder);
      }

    private:
      const ConfigurationType& configuration_;
    };

    typedef Generics::BoundedMap<
      KeyType,
      Holder,
      SizePolicy,
      Sync::Policy::PosixThread,
      MapType<KeyType, typename Generics::BoundedMapTypes<KeyType, Holder>::Item> >
      ElementMap;

    typedef AdServer::Commons::LockMap<
      KeyType, Sync::Policy::PosixThread>
      KeyLockMap;

  private:
    virtual
    ~BoundedCache() noexcept
    {}

    typename ElementMap::iterator
    update_(typename ElementMap::iterator& it, const KeyType& key)
      /*throw(typename ConfigurationType::Exception)*/;

  private:
    ConfigurationType configuration_;
    ElementMap map_;
    KeyLockMap update_lock_map_;
  };
}

namespace AdServer
{
  template<
    typename KeyType,
    typename ValueType,
    typename ConfigurationType,
    template <typename, typename> class MapType>
  BoundedCache<KeyType, ValueType, ConfigurationType, MapType>::
  BoundedCache(
    unsigned long bound,
    const Generics::Time& timeout,
    const ConfigurationType& configuration)
    /*throw(eh::Exception)*/
    : configuration_(configuration),
      map_(bound, timeout, SizePolicy(configuration_))
  {}

  template<
    typename KeyType,
    typename ValueType,
    typename ConfigurationType,
    template <typename, typename> class MapType>
  ValueType
  BoundedCache<KeyType, ValueType, ConfigurationType, MapType>::
  get(const KeyType& key)
    /*throw(typename ConfigurationType::Exception)*/
  {
    typename ElementMap::iterator it = map_.find(key);
    if(it != map_.end())
    {
      if(configuration_.update_required(it->first, it->second))
      {
        it = update_(it, key);
      }
    }
    else
    {
      it = update_(it, key);
    }

    return configuration_.adapt(it->second);
  }

  template<
    typename KeyType,
    typename ValueType,
    typename ConfigurationType,
    template <typename, typename> class MapType>
  typename BoundedCache<KeyType, ValueType, ConfigurationType, MapType>::
    ElementMap::iterator
  BoundedCache<KeyType, ValueType, ConfigurationType, MapType>::
  update_(typename ElementMap::iterator& it, const KeyType& key)
    /*throw(typename ConfigurationType::Exception)*/
  {
    typename KeyLockMap::WriteGuard lock = update_lock_map_.write_lock(key);
    if(it != map_.end())
    {
      map_.erase(it);
    }

    Holder h(configuration_.update(key, it != map_.end() ? &it->second: 0));
    return map_.insert(typename ElementMap::value_type(key, h)).first;
  }
}

#endif /*BOUNDEDCACHE_HPP*/
