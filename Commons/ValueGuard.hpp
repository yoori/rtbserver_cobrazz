#ifndef ADSERVER_COMMONS_VALUEGUARD_HPP
#define ADSERVER_COMMONS_VALUEGUARD_HPP

#include <ReferenceCounting/SmartPtr.hpp>
#include <Sync/SyncPolicy.hpp>

namespace AdServer
{
  namespace Commons
  {
    template <typename Type,
      typename SyncPolicyType = Sync::Policy::PosixThreadRW>
    class ValueGuard : private Generics::Uncopyable
    {
    public:
      typedef Type ValueType;

      template <typename... Args>
      explicit
      ValueGuard(Args... args) /*throw(eh::Exception)*/;

      ValueGuard&
      operator =(Type&& new_value) /*throw(eh::Exception)*/;

      template <typename T>
      ValueGuard&
      operator =(T&& new_value) /*throw(eh::Exception)*/;

      Type
      get() const /*throw(eh::Exception)*/;

    private:
      mutable typename SyncPolicyType::Mutex lock_;
      Type value_;
    };
  }
}

namespace AdServer
{
  namespace Commons
  {
    template <typename Type, typename SyncPolicyType>
    template <typename... Args>
    ValueGuard<Type, SyncPolicyType>::ValueGuard(Args... args)
      /*throw(eh::Exception)*/
      : value_(std::forward<Args>(args)...)
    {
    }

    template <typename Type, typename SyncPolicyType>
    ValueGuard<Type, SyncPolicyType>&
    ValueGuard<Type, SyncPolicyType>::operator =(Type&& new_value)
      /*throw(eh::Exception)*/
    {
      typename SyncPolicyType::WriteGuard guard(lock_);
      value_.swap(new_value);
      return *this;
    }

    template <typename Type, typename SyncPolicyType>
    template <typename T>
    ValueGuard<Type, SyncPolicyType>&
    ValueGuard<Type, SyncPolicyType>::operator =(T&& new_value)
      /*throw(eh::Exception)*/
    {
      typename SyncPolicyType::WriteGuard guard(lock_);
      value_ = std::forward<T>(new_value);
      return *this;
    }

    template <typename Type, typename SyncPolicyType>
    Type
    ValueGuard<Type, SyncPolicyType>::get() const /*throw(eh::Exception)*/
    {
      typename SyncPolicyType::ReadGuard guard(lock_);
      return value_;
    }
  }
}

#endif /*ADSERVER_COMMONS_VALUEGUARD_HPP*/
