#pragma once

#include <utility>

#include "TransactionMap.hpp"
#include "ProfileMap.hpp"

namespace AdServer::ProfilingCommons
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

    virtual Generics::SmartMemBuf_var
    get_own_profile(Generics::Time* last_access_time = 0)
      /*throw(typename ProfileMap<KeyType>::Exception)*/;

    /**
     * Simply delegate to ExpireProfileMap::save_profile
     */
    virtual void
    save_profile(
      const Generics::ConstSmartMemBuf* mem_buf,
      const Generics::Time& now = Generics::Time::get_time_of_day())
      /*throw(typename ProfileMap<KeyType>::Exception)*/;

    void
    save_profile_async(
      const Generics::ConstSmartMemBuf* mem_buf,
      const Generics::Time& now = Generics::Time::get_time_of_day())
      /*throw(typename ProfileMap<KeyType>::Exception)*/;

    /**
     * Simply delegate to ExpireProfileMap::remove_profile
     */
    virtual
    bool remove_profile()
      /*throw(typename ProfileMap<KeyType>::Exception)*/;

    AdServer::Commons::StartableAwaitable<Generics::ConstSmartMemBuf_var>
    co_get_profile(std::optional<Generics::Time> last_access_time = std::nullopt);

    AdServer::Commons::StartableAwaitable<Generics::SmartMemBuf_var>
    co_get_own_profile(std::optional<Generics::Time> last_access_time = std::nullopt);

    AdServer::Commons::StartableAwaitable<bool>
    co_save_profile(
      const Generics::ConstSmartMemBuf* mem_buf,
      const Generics::Time& now = Generics::Time::get_time_of_day());

    AdServer::Commons::StartableAwaitable<bool>
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
    public virtual AsyncProfileMapToProfileMap<KeyType>,
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
    TransactionProfileMap(
      AsyncProfileMap<KeyType>* base_map,
      unsigned long max_waiters = 0,
      bool create_transaction_on_get = false);

    virtual bool
    check_profile(const KeyType& key) const
      /*throw(typename ProfileMap<KeyType>::Exception)*/;

    virtual Generics::ConstSmartMemBuf_var
    get_profile(const KeyType& key, Generics::Time* last_access_time = 0)
      /*throw(typename ProfileMap<KeyType>::Exception)*/;

    virtual Generics::SmartMemBuf_var
    get_own_profile(const KeyType& key, Generics::Time* last_access_time = 0)
      /*throw(typename ProfileMap<KeyType>::Exception)*/;

    virtual void
    save_profile(
      const KeyType& key,
      const Generics::ConstSmartMemBuf* mem_buf,
      const Generics::Time& now = Generics::Time::get_time_of_day(),
      OperationPriority op_priority = OP_RUNTIME)
      /*throw(typename ProfileMap<KeyType>::Exception)*/;

    virtual bool
    remove_profile(const KeyType& key, OperationPriority op_priority = OP_RUNTIME)
      /*throw(typename ProfileMap<KeyType>::Exception)*/;

    void
    clear_expired(const Generics::Time& expire_time)
      /*throw(typename ProfileMap<KeyType>::Exception)*/ override;

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

    Generics::SmartMemBuf_var
    get_own_profile_async(
      const KeyType& key,
      typename AsyncProfileMap<KeyType>::GetOwnCallback callback,
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

    AdServer::Commons::StartableAwaitable<Transaction_var>
    co_get_transaction(
      const KeyType& key,
      bool check_max_waiters = true,
      OperationPriority op_priority = ProfilingCommons::OP_RUNTIME);

  protected:
    virtual ~TransactionProfileMap() noexcept;

  private:
    using TransactionHolder = typename BaseTransactionMapType::TransactionHolder;

  private:
    virtual Transaction_var
    create_transaction_impl_(
      TransactionHolder* holder,
      const KeyType& key,
      const OperationPriority& arg)
      /*throw(eh::Exception)*/;

    virtual Transaction_var
    create_transaction_impl_(
      TransactionHolder* holder,
      const KeyType& key,
      const OperationPriority& arg,
      AdServer::Commons::AsyncMutex::Guard&& guard)
      /*throw(eh::Exception)*/;

    Generics::ConstSmartMemBuf_var
    get_profile_i_(const KeyType& key, Generics::Time* last_access_time)
      /*throw(typename ProfileMap<KeyType>::Exception)*/;

    Generics::SmartMemBuf_var
    get_own_profile_i_(const KeyType& key, Generics::Time* last_access_time)
      /*throw(typename ProfileMap<KeyType>::Exception)*/;

    void
    save_profile_i_(
      const KeyType& key,
      const Generics::ConstSmartMemBuf* mem_buf,
      const Generics::Time& now,
      OperationPriority op_priority)
      /*throw(typename ProfileMap<KeyType>::Exception)*/;

    bool
    remove_profile_i_(const KeyType& key, OperationPriority op_priority)
      /*throw(typename ProfileMap<KeyType>::Exception)*/;

    AsyncProfileMap<KeyType>*
    async_delegate_map_() const noexcept;

    using SelfVar = ReferenceCounting::SmartPtr<TransactionProfileMap<KeyType>>;

    static AdServer::Commons::StartableAwaitable<void>
    co_save_profile_async_(
      SelfVar self,
      KeyType key,
      Generics::ConstSmartMemBuf_var profile,
      Generics::Time now,
      typename AsyncProfileMap<KeyType>::SaveCallback callback);

    static AdServer::Commons::StartableAwaitable<void>
    co_remove_profile_async_(
      SelfVar self,
      KeyType key,
      OperationPriority op_priority,
      typename AsyncProfileMap<KeyType>::RemoveCallback callback);

  private:
    const bool create_transaction_on_get_;
  };
}

//
// Implementations
//

namespace AdServer::ProfilingCommons
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
  ProfileTransactionImpl<KeyType>::get_profile(Generics::Time* last_access_time)
    /*throw(typename ProfileMap<KeyType>::Exception)*/
  {
    return profile_map_.get_profile_i_(key_, last_access_time);
  }

  template <typename KeyType>
  Generics::SmartMemBuf_var
  ProfileTransactionImpl<KeyType>::get_own_profile(Generics::Time* last_access_time)
    /*throw(typename ProfileMap<KeyType>::Exception)*/
  {
    return profile_map_.get_own_profile_i_(key_, last_access_time);
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
  void
  ProfileTransactionImpl<KeyType>::save_profile_async(
    const Generics::ConstSmartMemBuf* mem_buf,
    const Generics::Time& now)
    /*throw(typename ProfileMap<KeyType>::Exception)*/
  {
    ReferenceCounting::SmartPtr<ProfileTransactionImpl<KeyType>> transaction =
      ReferenceCounting::add_ref(this);
    profile_map_.async_delegate_map_()->save_profile_async(
      key_,
      mem_buf,
      now,
      [transaction = std::move(transaction)](std::optional<std::string>) mutable noexcept
      {
        transaction.reset();
      });
  }

  template <typename KeyType>
  bool
  ProfileTransactionImpl<KeyType>::remove_profile()
    /*throw(typename ProfileMap<KeyType>::Exception)*/
  {
    return profile_map_.remove_profile_i_(key_, op_priority_);
  }

  template <typename KeyType>
  AdServer::Commons::StartableAwaitable<Generics::ConstSmartMemBuf_var>
  ProfileTransactionImpl<KeyType>::co_get_profile(std::optional<Generics::Time> last_access_time)
  {
    co_return co_await profile_map_.async_delegate_map_()->co_get_profile(key_, last_access_time);
  }

  template <typename KeyType>
  AdServer::Commons::StartableAwaitable<Generics::SmartMemBuf_var>
  ProfileTransactionImpl<KeyType>::co_get_own_profile(
    std::optional<Generics::Time> last_access_time)
  {
    co_return co_await profile_map_.async_delegate_map_()->co_get_own_profile(
      key_,
      last_access_time);
  }

  template <typename KeyType>
  AdServer::Commons::StartableAwaitable<bool>
  ProfileTransactionImpl<KeyType>::co_save_profile(
    const Generics::ConstSmartMemBuf* mem_buf,
    const Generics::Time& now)
  {
    co_await profile_map_.async_delegate_map_()->co_save_profile(key_, mem_buf, now);
    co_return true;
  }

  template <typename KeyType>
  AdServer::Commons::StartableAwaitable<bool>
  ProfileTransactionImpl<KeyType>::co_remove_profile()
  {
    co_return co_await profile_map_.async_delegate_map_()->co_remove_profile(key_, op_priority_);
  }

  template <typename KeyType>
  TransactionProfileMap<KeyType>::
  TransactionProfileMap(
    AsyncProfileMap<KeyType>* base_map,
    unsigned long max_waiters,
    bool create_transaction_on_get)
    : AsyncProfileMapToProfileMap<KeyType>(base_map),
      BaseTransactionMapType(max_waiters),
      create_transaction_on_get_(create_transaction_on_get)
  {}

  template <typename KeyType>
  TransactionProfileMap<KeyType>::~TransactionProfileMap() noexcept = default;

  template <typename KeyType>
  typename TransactionProfileMap<KeyType>::Transaction_var
  TransactionProfileMap<KeyType>::create_transaction_impl_(
    TransactionHolder* holder,
    const KeyType& key,
    const OperationPriority& arg)
    /*throw(eh::Exception)*/
  {
    return new ProfileTransactionImplType(*this, holder, key, arg);
  }

  template <typename KeyType>
  typename TransactionProfileMap<KeyType>::Transaction_var
  TransactionProfileMap<KeyType>::create_transaction_impl_(
    TransactionHolder* holder,
    const KeyType& key,
    const OperationPriority& arg,
    AdServer::Commons::AsyncMutex::Guard&& guard)
    /*throw(eh::Exception)*/
  {
    return new ProfileTransactionImplType(*this, holder, key, arg, std::move(guard));
  }

  template <typename KeyType>
  bool
  TransactionProfileMap<KeyType>::check_profile(const KeyType& key) const
    /*throw(typename ProfileMap<KeyType>::Exception)*/
  {
    return AsyncProfileMapToProfileMap<KeyType>::check_profile(key);
  }

  template <typename KeyType>
  Generics::ConstSmartMemBuf_var
  TransactionProfileMap<KeyType>::get_profile(const KeyType& key, Generics::Time* last_access_time)
    /*throw(typename ProfileMap<KeyType>::Exception)*/
  {
    if (create_transaction_on_get_)
    {
      return this->get_transaction(key, false)->get_profile(last_access_time);
    }
    else
    {
      return AsyncProfileMapToProfileMap<KeyType>::get_profile(key, last_access_time);
    }
  }

  template <typename KeyType>
  Generics::SmartMemBuf_var
  TransactionProfileMap<KeyType>::get_own_profile(
    const KeyType& key,
    Generics::Time* last_access_time)
    /*throw(typename ProfileMap<KeyType>::Exception)*/
  {
    if (create_transaction_on_get_)
    {
      return this->get_transaction(key, false)->get_own_profile(last_access_time);
    }
    else
    {
      return AsyncProfileMapToProfileMap<KeyType>::get_own_profile(key, last_access_time);
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
  TransactionProfileMap<KeyType>::remove_profile(const KeyType& key, OperationPriority op_priority)
    /*throw(typename ProfileMap<KeyType>::Exception)*/
  {
    return this->get_transaction(key, false, op_priority)->remove_profile();
  }

  template <typename KeyType>
  AsyncProfileMap<KeyType>*
  TransactionProfileMap<KeyType>::async_delegate_map_() const noexcept
  {
    return this->async_profile_map();
  }

  template <typename KeyType>
  void
  TransactionProfileMap<KeyType>::clear_expired(const Generics::Time& expire_time)
    /*throw(typename ProfileMap<KeyType>::Exception)*/
  {
    AsyncProfileMapToProfileMap<KeyType>::clear_expired(expire_time);
  }

  template <typename KeyType>
  void
  TransactionProfileMap<KeyType>::check_profile_async(
    const KeyType& key,
    typename AsyncProfileMap<KeyType>::CheckCallback callback) const
  {
    async_delegate_map_()->check_profile_async(key, std::move(callback));
  }

  template <typename KeyType>
  Generics::ConstSmartMemBuf_var
  TransactionProfileMap<KeyType>::get_profile_async(
    const KeyType& key,
    typename AsyncProfileMap<KeyType>::GetCallback callback,
    std::optional<Generics::Time> last_access_time)
  {
    return async_delegate_map_()->get_profile_async(key, std::move(callback), last_access_time);
  }

  template <typename KeyType>
  Generics::SmartMemBuf_var
  TransactionProfileMap<KeyType>::get_own_profile_async(
    const KeyType& key,
    typename AsyncProfileMap<KeyType>::GetOwnCallback callback,
    std::optional<Generics::Time> last_access_time)
  {
    return async_delegate_map_()->get_own_profile_async(key, std::move(callback), last_access_time);
  }

  template <typename KeyType>
  AdServer::Commons::StartableAwaitable<void>
  TransactionProfileMap<KeyType>::co_save_profile_async_(
    SelfVar self,
    KeyType key,
    Generics::ConstSmartMemBuf_var profile,
    Generics::Time now,
    typename AsyncProfileMap<KeyType>::SaveCallback callback)
  {
    Transaction_var transaction;
    std::optional<std::string> error;
    try
    {
      transaction = co_await self->co_get_transaction(key, false, OP_RUNTIME);
      co_await transaction->co_save_profile(profile.in(), now);
    }
    catch(const std::exception& ex)
    {
      error = ex.what();
    }
    catch(...)
    {
      error = "unknown save error";
    }

    transaction.reset();

    if (callback)
    {
      callback(std::move(error));
    }
  }

  template <typename KeyType>
  AdServer::Commons::StartableAwaitable<void>
  TransactionProfileMap<KeyType>::co_remove_profile_async_(
    SelfVar self,
    KeyType key,
    OperationPriority op_priority,
    typename AsyncProfileMap<KeyType>::RemoveCallback callback)
  {
    Transaction_var transaction;
    bool result = false;
    std::optional<std::string> error;
    try
    {
      transaction = co_await self->co_get_transaction(key, false, op_priority);
      result = co_await transaction->co_remove_profile();
    }
    catch(const std::exception& ex)
    {
      error = ex.what();
    }
    catch(...)
    {
      error = "unknown remove error";
    }

    transaction.reset();

    if (callback)
    {
      callback(result, std::move(error));
    }
  }

  template <typename KeyType>
  void
  TransactionProfileMap<KeyType>::save_profile_async(
    const KeyType& key,
    const Generics::ConstSmartMemBuf* mem_buf,
    const Generics::Time& now,
    typename AsyncProfileMap<KeyType>::SaveCallback callback)
  {
    Generics::ConstSmartMemBuf_var profile(ReferenceCounting::add_ref(mem_buf));
    auto operation = co_save_profile_async_(
      ReferenceCounting::add_ref(this),
      key,
      std::move(profile),
      now,
      std::move(callback));
    operation.start_detached({});
  }

  template <typename KeyType>
  void
  TransactionProfileMap<KeyType>::remove_profile_async(
    const KeyType& key,
    OperationPriority op_priority,
    typename AsyncProfileMap<KeyType>::RemoveCallback callback)
  {
    auto operation = co_remove_profile_async_(
      ReferenceCounting::add_ref(this), key, op_priority, std::move(callback));
    operation.start_detached({});
  }

  template <typename KeyType>
  void
  TransactionProfileMap<KeyType>::clear_expired_async(
    const Generics::Time& expire_time,
    typename AsyncProfileMap<KeyType>::CompleteCallback complete)
  {
    async_delegate_map_()->clear_expired_async(expire_time, std::move(complete));
  }

  template <typename KeyType>
  typename TransactionProfileMap<KeyType>::Transaction_var
  TransactionProfileMap<KeyType>::get_transaction(
    const KeyType& key,
    bool check_max_waiters,
    OperationPriority op_priority)
    /*throw(MaxWaitersReached, Exception)*/
  {
    return BaseTransactionMapType::get_transaction(key, check_max_waiters, op_priority);
  }

  template <typename KeyType>
  AdServer::Commons::StartableAwaitable<
    typename TransactionProfileMap<KeyType>::Transaction_var>
  TransactionProfileMap<KeyType>::co_get_transaction(
    const KeyType& key,
    bool check_max_waiters,
    OperationPriority op_priority)
  {
    co_return co_await BaseTransactionMapType::co_get_transaction(
      key, check_max_waiters, op_priority);
  }

  template <typename KeyType>
  Generics::ConstSmartMemBuf_var
  TransactionProfileMap<KeyType>::get_profile_i_(
    const KeyType& key,
    Generics::Time* last_access_time)
    /*throw(typename ProfileMap<KeyType>::Exception)*/
  {
    return AsyncProfileMapToProfileMap<KeyType>::get_profile(key, last_access_time);
  }

  template <typename KeyType>
  Generics::SmartMemBuf_var
  TransactionProfileMap<KeyType>::get_own_profile_i_(
    const KeyType& key,
    Generics::Time* last_access_time)
    /*throw(typename ProfileMap<KeyType>::Exception)*/
  {
    return AsyncProfileMapToProfileMap<KeyType>::get_own_profile(key, last_access_time);
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
    AsyncProfileMapToProfileMap<KeyType>::save_profile(key, mem_buf, now, op_priority);
  }

  template <typename KeyType>
  bool
  TransactionProfileMap<KeyType>::remove_profile_i_(
    const KeyType& key,
    OperationPriority op_priority)
    /*throw(typename ProfileMap<KeyType>::Exception)*/
  {
    return AsyncProfileMapToProfileMap<KeyType>::remove_profile(key, op_priority);
  }
}
