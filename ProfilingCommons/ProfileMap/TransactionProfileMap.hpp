/**
 * @file TransactionProfileMap.hpp
 */

#pragma once

#include <utility>

#include "TransactionMap.hpp"
#include "ProfileMap.hpp"
#include "DelegateProfileMap.hpp"

namespace AdServer
{
namespace ProfilingCommons
{
  template <typename KeyType>
  class TransactionProfileMap;

  template <typename KeyType>
  class ProfileTransactionImpl: public TransactionBase
  {
  public:
    typedef OperationPriority ArgType;

  public:
    ProfileTransactionImpl(
      TransactionProfileMap<KeyType>& profile_map,
      TransactionBase::TransactionHolderBase* holder,
      const KeyType& key,
      OperationPriority op_priority)
      noexcept;

    ProfileTransactionImpl(
      TransactionProfileMap<KeyType>& profile_map,
      TransactionBase::TransactionHolderBase* holder,
      const KeyType& key,
      OperationPriority op_priority,
      AdServer::Commons::AsyncMutex::Guard&& guard)
      noexcept;

    /**
     * Simply delegate to ExpireProfileMap::get_profile
     */
    virtual
    Generics::ConstSmartMemBuf_var
    get_profile(Generics::Time* last_access_time = 0)
      /*throw(typename ProfileMap<KeyType>::Exception)*/;

    /**
     * Simply delegate to ExpireProfileMap::save_profile
     */
    virtual void
    save_profile(
      const Generics::ConstSmartMemBuf* mem_buf,
      const Generics::Time& now = Generics::Time::get_time_of_day())
      /*throw(typename ProfileMap<KeyType>::Exception)*/;

    /**
     * Simply delegate to ExpireProfileMap::remove_profile
     */
    virtual
    bool remove_profile()
      /*throw(typename ProfileMap<KeyType>::Exception)*/;

    AdServer::Commons::Task<Generics::ConstSmartMemBuf_var>
    co_get_profile(
      std::optional<Generics::Time> last_access_time = std::nullopt);

    AdServer::Commons::Task<bool>
    co_save_profile(
      const Generics::ConstSmartMemBuf* mem_buf,
      const Generics::Time& now = Generics::Time::get_time_of_day());

    AdServer::Commons::Task<bool>
    co_remove_profile();

  protected:
    /**
     * Virtual empty destructor, protected as RC-object
     */
    virtual
    ~ProfileTransactionImpl() noexcept;

  private:
    TransactionProfileMap<KeyType>& profile_map_;
    KeyType key_;
    OperationPriority op_priority_;
  };

  /**
   * ProfileMapType is RC-object
   */
  template <typename KeyType>
  class TransactionProfileMap:
    public virtual DelegateProfileMap<KeyType>,
    public virtual AsyncProfileMap<KeyType>,
    public virtual ReferenceCounting::AtomicImpl,
    protected virtual TransactionMap<KeyType, ProfileTransactionImpl<KeyType> >
  {
    friend class ProfileTransactionImpl<KeyType>;

  public:
    DECLARE_EXCEPTION(Exception, typename ProfileMap<KeyType>::Exception);

    typedef ProfileTransactionImpl<KeyType> ProfileTransactionImplType;

    typedef TransactionMap<KeyType, ProfileTransactionImplType>
      BaseTransactionMapType;

    typedef typename BaseTransactionMapType::MaxWaitersReached
      MaxWaitersReached;

    typedef typename BaseTransactionMapType::Transaction
      Transaction;

    typedef typename BaseTransactionMapType::Transaction_var
      Transaction_var;

  public:
    TransactionProfileMap(ProfileMap<KeyType>* base_map,
      unsigned long max_waiters = 0,
      bool create_transaction_on_get = false)
      noexcept;

    virtual bool
    check_profile(const KeyType& key) const
      /*throw(typename ProfileMap<KeyType>::Exception)*/;

    virtual Generics::ConstSmartMemBuf_var
    get_profile(
      const KeyType& key,
      Generics::Time* last_access_time = 0)
      /*throw(typename ProfileMap<KeyType>::Exception)*/;

    virtual void
    save_profile(
      const KeyType& key,
      const Generics::ConstSmartMemBuf* mem_buf,
      const Generics::Time& now = Generics::Time::get_time_of_day(),
      OperationPriority op_priority = OP_RUNTIME)
      /*throw(typename ProfileMap<KeyType>::Exception)*/;

    virtual bool
    remove_profile(
      const KeyType& key,
      OperationPriority op_priority = OP_RUNTIME)
      /*throw(typename ProfileMap<KeyType>::Exception)*/;

    void
    check_profile_async(
      const KeyType& key,
      typename AsyncProfileMap<KeyType>::CheckCallback callback) const
      override;

    Generics::ConstSmartMemBuf_var
    get_profile_async(
      const KeyType& key,
      typename AsyncProfileMap<KeyType>::GetCallback callback,
      std::optional<Generics::Time> last_access_time = std::nullopt)
      override;

    void
    save_profile_async(
      const KeyType& key,
      const Generics::ConstSmartMemBuf* mem_buf,
      const Generics::Time& now = Generics::Time::get_time_of_day(),
      typename AsyncProfileMap<KeyType>::SaveCallback callback =
        typename AsyncProfileMap<KeyType>::SaveCallback())
      override;

    void
    remove_profile_async(
      const KeyType& key,
      OperationPriority op_priority = OP_RUNTIME,
      typename AsyncProfileMap<KeyType>::RemoveCallback callback =
        typename AsyncProfileMap<KeyType>::RemoveCallback())
      override;

    void
    clear_expired_async(
      const Generics::Time& expire_time,
      typename AsyncProfileMap<KeyType>::CompleteCallback complete =
        typename AsyncProfileMap<KeyType>::CompleteCallback())
      override;

    Transaction_var
    get_transaction(
      const KeyType& key,
      bool check_max_waiters = true,
      OperationPriority op_priority = ProfilingCommons::OP_RUNTIME)
      /*throw(MaxWaitersReached, Exception)*/;

    AdServer::Commons::Task<Transaction_var>
    co_get_transaction(
      const KeyType& key,
      bool check_max_waiters = true,
      OperationPriority op_priority = ProfilingCommons::OP_RUNTIME);

  protected:
    virtual ~TransactionProfileMap() noexcept {}

  private:
    typedef typename BaseTransactionMapType::TransactionHolder
      TransactionHolder;

  private:
    virtual Transaction_var
    create_transaction_impl_(
      TransactionHolder* holder,
      const KeyType& key,
      const OperationPriority& arg)
      /*throw(eh::Exception)*/
    {
      return new ProfileTransactionImplType(*this, holder, key, arg);
    }

    virtual Transaction_var
    create_transaction_impl_(
      TransactionHolder* holder,
      const KeyType& key,
      const OperationPriority& arg,
      AdServer::Commons::AsyncMutex::Guard&& guard)
      /*throw(eh::Exception)*/
    {
      return new ProfileTransactionImplType(
        *this,
        holder,
        key,
        arg,
        std::move(guard));
    }

    Generics::ConstSmartMemBuf_var
    get_profile_i_(
      const KeyType& key,
      Generics::Time* last_access_time)
      /*throw(typename ProfileMap<KeyType>::Exception)*/;

    void
    save_profile_i_(
      const KeyType& key,
      const Generics::ConstSmartMemBuf* mem_buf,
      const Generics::Time& now,
      OperationPriority op_priority)
      /*throw(typename ProfileMap<KeyType>::Exception)*/;

    bool
    remove_profile_i_(
      const KeyType& key,
      OperationPriority op_priority)
      /*throw(typename ProfileMap<KeyType>::Exception)*/;

    AsyncProfileMap<KeyType>*
    async_delegate_map_() const noexcept;

  private:
    const bool create_transaction_on_get_;
  };
}
}

//
// Implementations
//

namespace AdServer
{
namespace ProfilingCommons
{
  /* ProfileTransactionImpl class */
  template <typename KeyType>
  ProfileTransactionImpl<KeyType>::
  ProfileTransactionImpl(
    TransactionProfileMap<KeyType>& profile_map,
    TransactionBase::TransactionHolderBase* holder,
    const KeyType& key,
    OperationPriority op_priority)
    noexcept
    : TransactionBase(holder),
      profile_map_(profile_map),
      key_(key),
      op_priority_(op_priority)
  {}

  template <typename KeyType>
  ProfileTransactionImpl<KeyType>::
  ProfileTransactionImpl(
    TransactionProfileMap<KeyType>& profile_map,
    TransactionBase::TransactionHolderBase* holder,
    const KeyType& key,
    OperationPriority op_priority,
    AdServer::Commons::AsyncMutex::Guard&& guard)
    noexcept
    : TransactionBase(holder, std::move(guard)),
      profile_map_(profile_map),
      key_(key),
      op_priority_(op_priority)
  {}

  template <typename KeyType>
  ProfileTransactionImpl<KeyType>::~ProfileTransactionImpl() noexcept
  {}

  template <typename KeyType>
  Generics::ConstSmartMemBuf_var
  ProfileTransactionImpl<KeyType>::get_profile(
    Generics::Time* last_access_time)
    /*throw(typename ProfileMap<KeyType>::Exception)*/
  {
    return profile_map_.get_profile_i_(key_, last_access_time);
  }

  template <typename KeyType>
  void
  ProfileTransactionImpl<KeyType>::save_profile(
    const Generics::ConstSmartMemBuf* mem_buf,
    const Generics::Time& now)
    /*throw(typename ProfileMap<KeyType>::Exception)*/
  {
    profile_map_.save_profile_i_(key_, mem_buf, now, op_priority_);
  }

  template <typename KeyType>
  bool
  ProfileTransactionImpl<KeyType>::remove_profile()
    /*throw(typename ProfileMap<KeyType>::Exception)*/
  {
    return profile_map_.remove_profile_i_(key_, op_priority_);
  }

  template <typename KeyType>
  AdServer::Commons::Task<Generics::ConstSmartMemBuf_var>
  ProfileTransactionImpl<KeyType>::co_get_profile(
    std::optional<Generics::Time> last_access_time)
  {
    if(auto* async_map = profile_map_.async_delegate_map_())
    {
      co_return co_await async_map->co_get_profile(key_, last_access_time);
    }

    Generics::Time access_time;
    co_return profile_map_.get_profile_i_(
      key_,
      last_access_time ? &access_time : nullptr);
  }

  template <typename KeyType>
  AdServer::Commons::Task<bool>
  ProfileTransactionImpl<KeyType>::co_save_profile(
    const Generics::ConstSmartMemBuf* mem_buf,
    const Generics::Time& now)
  {
    if(auto* async_map = profile_map_.async_delegate_map_())
    {
      co_await async_map->co_save_profile(key_, mem_buf, now);
    }
    else
    {
      profile_map_.save_profile_i_(key_, mem_buf, now, op_priority_);
    }
    co_return true;
  }

  template <typename KeyType>
  AdServer::Commons::Task<bool>
  ProfileTransactionImpl<KeyType>::co_remove_profile()
  {
    if(auto* async_map = profile_map_.async_delegate_map_())
    {
      co_return co_await async_map->co_remove_profile(key_, op_priority_);
    }

    co_return profile_map_.remove_profile_i_(key_, op_priority_);
  }

  template <typename KeyType>
  TransactionProfileMap<KeyType>::
  TransactionProfileMap(ProfileMap<KeyType>* base_map,
    unsigned long max_waiters,
    bool create_transaction_on_get)
    noexcept
    : DelegateProfileMap<KeyType>(base_map),
      BaseTransactionMapType(max_waiters),
      create_transaction_on_get_(create_transaction_on_get)
  {}

  template <typename KeyType>
  bool
  TransactionProfileMap<KeyType>::check_profile(
    const KeyType& key) const
    /*throw(typename ProfileMap<KeyType>::Exception)*/
  {
    return this->no_add_ref_delegate_map_()->check_profile(key);
  }

  template <typename KeyType>
  Generics::ConstSmartMemBuf_var
  TransactionProfileMap<KeyType>::get_profile(
    const KeyType& key,
    Generics::Time* last_access_time)
    /*throw(typename ProfileMap<KeyType>::Exception)*/
  {
    if(create_transaction_on_get_)
    {
      return this->get_transaction(key, false)->get_profile(last_access_time);
    }
    else
    {
      return this->no_add_ref_delegate_map_()->get_profile(
        key, last_access_time);
    }
  }

  template <typename KeyType>
  void
  TransactionProfileMap<KeyType>::save_profile(
    const KeyType& key,
    const Generics::ConstSmartMemBuf* mem_buf,
    const Generics::Time& now,
    OperationPriority op_priority)
    /*throw(typename ProfileMap<KeyType>::Exception)*/
  {
    this->get_transaction(key, false, op_priority)->save_profile(mem_buf, now);
  }

  template <typename KeyType>
  bool
  TransactionProfileMap<KeyType>::remove_profile(
    const KeyType& key,
    OperationPriority op_priority)
    /*throw(typename ProfileMap<KeyType>::Exception)*/
  {
    return this->get_transaction(key, false, op_priority)->remove_profile();
  }

  template <typename KeyType>
  AsyncProfileMap<KeyType>*
  TransactionProfileMap<KeyType>::async_delegate_map_() const noexcept
  {
    return dynamic_cast<AsyncProfileMap<KeyType>*>(
      this->no_add_ref_delegate_map_());
  }

  template <typename KeyType>
  void
  TransactionProfileMap<KeyType>::check_profile_async(
    const KeyType& key,
    typename AsyncProfileMap<KeyType>::CheckCallback callback) const
  {
    if(auto* async_map = async_delegate_map_())
    {
      async_map->check_profile_async(key, std::move(callback));
      return;
    }

    bool result = false;
    std::optional<std::string> error;
    try
    {
      result = check_profile(key);
    }
    catch(const std::exception& ex)
    {
      error = ex.what();
    }
    catch(...)
    {
      error = "unknown check error";
    }

    if(callback)
    {
      callback(result, std::move(error));
    }
  }

  template <typename KeyType>
  Generics::ConstSmartMemBuf_var
  TransactionProfileMap<KeyType>::get_profile_async(
    const KeyType& key,
    typename AsyncProfileMap<KeyType>::GetCallback callback,
    std::optional<Generics::Time> last_access_time)
  {
    if(auto* async_map = async_delegate_map_())
    {
      return async_map->get_profile_async(
        key,
        std::move(callback),
        last_access_time);
    }

    Generics::ConstSmartMemBuf_var result;
    std::optional<std::string> error;
    try
    {
      Generics::Time access_time;
      result = get_profile(
        key,
        last_access_time ? &access_time : nullptr);
    }
    catch(const std::exception& ex)
    {
      error = ex.what();
    }
    catch(...)
    {
      error = "unknown get error";
    }

    if(callback)
    {
      callback(result, std::move(error));
    }

    return result;
  }

  template <typename KeyType>
  void
  TransactionProfileMap<KeyType>::save_profile_async(
    const KeyType& key,
    const Generics::ConstSmartMemBuf* mem_buf,
    const Generics::Time& now,
    typename AsyncProfileMap<KeyType>::SaveCallback callback)
  {
    std::optional<std::string> error;
    try
    {
      save_profile(key, mem_buf, now);
    }
    catch(const std::exception& ex)
    {
      error = ex.what();
    }
    catch(...)
    {
      error = "unknown save error";
    }

    if(callback)
    {
      callback(std::move(error));
    }
  }

  template <typename KeyType>
  void
  TransactionProfileMap<KeyType>::remove_profile_async(
    const KeyType& key,
    OperationPriority op_priority,
    typename AsyncProfileMap<KeyType>::RemoveCallback callback)
  {
    bool result = false;
    std::optional<std::string> error;
    try
    {
      result = remove_profile(key, op_priority);
    }
    catch(const std::exception& ex)
    {
      error = ex.what();
    }
    catch(...)
    {
      error = "unknown remove error";
    }

    if(callback)
    {
      callback(result, std::move(error));
    }
  }

  template <typename KeyType>
  void
  TransactionProfileMap<KeyType>::clear_expired_async(
    const Generics::Time& expire_time,
    typename AsyncProfileMap<KeyType>::CompleteCallback complete)
  {
    if(auto* async_map = async_delegate_map_())
    {
      async_map->clear_expired_async(expire_time, std::move(complete));
      return;
    }

    try
    {
      this->clear_expired(expire_time);
    }
    catch(...)
    {}

    if(complete)
    {
      complete();
    }
  }

  template <typename KeyType>
  typename TransactionProfileMap<KeyType>::Transaction_var
  TransactionProfileMap<KeyType>::get_transaction(
    const KeyType& key,
    bool check_max_waiters,
    OperationPriority op_priority)
    /*throw(MaxWaitersReached, Exception)*/
  {
    this->no_add_ref_delegate_map_()->wait_preconditions(key, op_priority);

    return BaseTransactionMapType::get_transaction(
      key,
      check_max_waiters,
      op_priority);
  }

  template <typename KeyType>
  AdServer::Commons::Task<
    typename TransactionProfileMap<KeyType>::Transaction_var>
  TransactionProfileMap<KeyType>::co_get_transaction(
    const KeyType& key,
    bool check_max_waiters,
    OperationPriority op_priority)
  {
    this->no_add_ref_delegate_map_()->wait_preconditions(key, op_priority);

    co_return co_await BaseTransactionMapType::co_get_transaction(
      key,
      check_max_waiters,
      op_priority);
  }

  template <typename KeyType>
  Generics::ConstSmartMemBuf_var
  TransactionProfileMap<KeyType>::get_profile_i_(
    const KeyType& key,
    Generics::Time* last_access_time)
    /*throw(typename ProfileMap<KeyType>::Exception)*/
  {
    return this->no_add_ref_delegate_map_()->get_profile(
      key, last_access_time);
  }

  template <typename KeyType>
  void
  TransactionProfileMap<KeyType>::save_profile_i_(
    const KeyType& key,
    const Generics::ConstSmartMemBuf* mem_buf,
    const Generics::Time& now,
    OperationPriority op_priority)
    /*throw(typename ProfileMap<KeyType>::Exception)*/
  {
    this->no_add_ref_delegate_map_()->save_profile(key, mem_buf, now, op_priority);
  }

  template <typename KeyType>
  bool
  TransactionProfileMap<KeyType>::remove_profile_i_(
    const KeyType& key,
    OperationPriority op_priority)
    /*throw(typename ProfileMap<KeyType>::Exception)*/
  {
    return this->no_add_ref_delegate_map_()->remove_profile(key, op_priority);
  }
}
}
