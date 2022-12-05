/**
 * @file ProfilingCommons/TransactionMap.hpp
 * TransactionMap must guarantee that only one Transaction object
 * can be created for each key. And lock application if it try get
 * already opened transaction, and wait it close.
 */
#ifndef TRANSACTION_MAP_HPP
#define TRANSACTION_MAP_HPP

#include <ReferenceCounting/AtomicImpl.hpp>
#include <Sync/SyncPolicy.hpp>
#include <Commons/AtomicInt.hpp>

namespace AdServer
{
  namespace ProfilingCommons
  {
    class TransactionBase : public virtual ReferenceCounting::AtomicImpl
    {
      typedef Sync::Policy::PosixThread SyncPolicy;

    public:
      class TransactionHolderBase:
        public virtual ReferenceCounting::AtomicImpl
      {
      public:
        TransactionHolderBase(): lock_count_(1)
        {}

        SyncPolicy::Mutex lock_;
        Algs::AtomicInt lock_count_;

      protected:
        virtual ~TransactionHolderBase() noexcept;
      };

      typedef ReferenceCounting::SmartPtr<TransactionHolderBase>
        TransactionHolderBase_var;

      /**
       * lock holder->lock_ for write
       */
      TransactionBase(TransactionHolderBase* holder) noexcept;

    protected:
      virtual ~TransactionBase() noexcept;

    private:
      TransactionHolderBase_var holder_;
      SyncPolicy::WriteGuard locker_;
    };

    /**
     * TransactionImplType must inherit TransactionBase.
     * TransactionMap must guarantee that only one Transaction object can be
     * created for each key. And lock application if it try get already
     * opened transaction, and wait it close.
     */
    template <typename KeyType, typename TransactionImplType>
    class TransactionMap
    {
    public:
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);
      DECLARE_EXCEPTION(MaxWaitersReached, Exception);

      typedef TransactionImplType
        Transaction;

      typedef ReferenceCounting::SmartPtr<TransactionImplType>
        Transaction_var;

      typedef typename TransactionImplType::ArgType TransactionArgType;

    public:
      TransactionMap(
        unsigned long max_waiters = 0,
        unsigned long portions = 1024) noexcept;

      Transaction_var
      get_transaction(
        const KeyType& key,
        bool check_max_waiters = true,
        const TransactionArgType& arg = TransactionArgType())
        /*throw(MaxWaitersReached, Exception)*/;

      virtual ~TransactionMap() noexcept;

    protected:
      /**
       * Override AtomicImpl : delegate transaction close into map,
       * if need delete object for erase from open_transaction_map_.
       */
      class TransactionHolder: public TransactionBase::TransactionHolderBase
      {
      public:
        TransactionHolder(TransactionMap& transactions_map, const KeyType& key)
          noexcept;

      protected:
        virtual ~TransactionHolder() noexcept;

        virtual bool
        remove_ref_no_delete_() const noexcept;

      private:
        KeyType key_;
        TransactionMap& transactions_map_;
      };

      typedef Sync::Policy::PosixThread SyncPolicy;

      typedef ReferenceCounting::SmartPtr<TransactionHolder>
        TransactionHolder_var;

      typedef std::map<KeyType, TransactionHolder*> OpenedTransactionMap;

      struct Portion: public ReferenceCounting::AtomicImpl
      {
        SyncPolicy::Mutex open_transaction_map_lock;        
        OpenedTransactionMap open_transaction_map;

      protected:
        virtual ~Portion() noexcept
        {}
      };

      typedef ReferenceCounting::SmartPtr<Portion> Portion_var;

    protected:
      /**
       * child classes must override this method,
       * can be implemented as strategy
       */
      virtual Transaction_var
      create_transaction_impl_(
        TransactionHolder* holder,
        const KeyType& key,
        const TransactionArgType& arg)
        /*throw(eh::Exception)*/ = 0;

      Portion*
      get_portion_(const KeyType& key)
        noexcept;

    private:
      /**
       * remove Transaction from open_transaction_map_
       */
      void
      close_i_(Portion* portion, const KeyType& key) noexcept;

    private:
      const unsigned long max_waiters_;
      std::vector<Portion_var> portions_;
      //SyncPolicy::Mutex open_transaction_map_lock_;
      //typedef std::map<KeyType, TransactionHolder*> OpenedTransactionMap;
      //OpenedTransactionMap open_transaction_map_;
    };
  }
}

//
// Implementation
//

namespace AdServer
{
  namespace ProfilingCommons
  {
    inline
    TransactionBase::TransactionHolderBase::~TransactionHolderBase() noexcept
    {}

    inline
    TransactionBase::TransactionBase(TransactionHolderBase* holder)
      noexcept
      : holder_(ReferenceCounting::add_ref(holder)),
        locker_(holder->lock_)
    {}

    inline
    TransactionBase::~TransactionBase() noexcept
    {
      holder_->lock_count_ += -1;
    }

    template <typename KeyType, typename TransactionImplType>
    TransactionMap<KeyType, TransactionImplType>::TransactionHolder::
      TransactionHolder(TransactionMap& transactions_map, const KeyType& key)
      noexcept
      : key_(key),
        transactions_map_(transactions_map)
    {}

    template <typename KeyType, typename TransactionImplType>
    TransactionMap<KeyType, TransactionImplType>::TransactionHolder::
      ~TransactionHolder() noexcept
    {}

    template <typename KeyType, typename TransactionImplType>
    bool
    TransactionMap<KeyType, TransactionImplType>::TransactionHolder::
      remove_ref_no_delete_() const noexcept
    {
      // We must first lock open_transaction_map_lock_ to avoid
      // using holder_ in get_transaction, it can be removed here.
      // 1) map_lock_ 2) remove_ref 3) remove open_transac if need
      TransactionMap<KeyType, TransactionImplType>::Portion* portion =
        transactions_map_.get_portion_(key_);
      SyncPolicy::WriteGuard lock(portion->open_transaction_map_lock);
      if (ReferenceCounting::AtomicImpl::remove_ref_no_delete_())
      {
        transactions_map_.close_i_(portion, key_);
        return true;
      }

      return false;
    }

    template <typename KeyType, typename TransactionImplType>
    TransactionMap<KeyType, TransactionImplType>::
    TransactionMap(unsigned long max_waiters, unsigned long portions)
      noexcept
      : max_waiters_(max_waiters)
    {
      portions_.resize(portions);
      for(auto it = portions_.begin(); it != portions_.end(); ++it)
      {
        *it = new Portion();
      }
    }
    
    /**
     * lock open_transaction_map_lock_,  find key:
     * If not exists simple create Transaction -> TransactionHolder and
     * insert holder into open_transaction_map_.
     * If exists save _var to holder and create Transaction object with
     * this holder, outside open_transaction_map_lock_ zone.
     */
    template <typename KeyType, typename TransactionImplType>
    typename TransactionMap<KeyType, TransactionImplType>::Transaction_var
    TransactionMap<KeyType, TransactionImplType>::
    get_transaction(
      const KeyType& key,
      bool check_max_waiters,
      const TransactionArgType& arg)
      /*throw(MaxWaitersReached, Exception)*/
    {
      static const char* FUN = "TransactionMap::get_transaction()";

      Portion* portion = get_portion_(key);
      try
      {
        TransactionHolder_var holder;
        unsigned long lock_count;
        bool max_waiters_reached = false;

        {
          SyncPolicy::WriteGuard lock(portion->open_transaction_map_lock);
          typename OpenedTransactionMap::iterator it =
            portion->open_transaction_map.find(key);
          if (it == portion->open_transaction_map.end())  // not exists
          {
            holder = new TransactionHolder(*this, key);
            Transaction_var transac(
              create_transaction_impl_(holder, key, arg));
            portion->open_transaction_map.insert(std::make_pair(key, holder.in()));
            return transac;
          }
          // will be created additional transaction object with this holder,
          // we must add reference to this holder, because transaction use it
          holder = ReferenceCounting::add_ref(it->second);
          lock_count = it->second->lock_count_;
          if(check_max_waiters &&
             max_waiters_ != 0 &&
             lock_count >= max_waiters_)
          {
            max_waiters_reached = true;
          }
          else
          {
            it->second->lock_count_ += 1;
          }
        }

        if(max_waiters_reached)
        {
          // throw outside lock
          Stream::Error ostr;
          ostr << FUN << ": already opened " << lock_count <<
            " transactions, max waiters = " << max_waiters_;
          throw MaxWaitersReached(ostr);
        }

        try
        {
          // exist already opened transaction
          return create_transaction_impl_(holder, key, arg);
        }
        catch(...)
        {
          holder->lock_count_ += -1;
          throw;
        }
      }
      catch (const MaxWaitersReached&)
      {
        throw;
      }
      catch (const eh::Exception& ex)
      {
        Stream::Error ostr;
        ostr << FUN << ": cannot create transaction: " << ex.what();
        throw Exception(ostr);
      }
    }

    template <typename KeyType, typename TransactionImplType>
    TransactionMap<KeyType, TransactionImplType>::~TransactionMap() noexcept
    {}

    template <typename KeyType, typename TransactionImplType>
    void
    TransactionMap<KeyType, TransactionImplType>::close_i_(
      Portion* portion, const KeyType& key)
      noexcept
    {
      typename OpenedTransactionMap::iterator it =
        portion->open_transaction_map.find(key);
      if (it != portion->open_transaction_map.end())
      {
        portion->open_transaction_map.erase(it);
      }
    }

    template <typename KeyType, typename TransactionImplType>
    typename TransactionMap<KeyType, TransactionImplType>::Portion*
    TransactionMap<KeyType, TransactionImplType>::get_portion_(const KeyType& key)
      noexcept
    {
      return portions_[key.hash() % portions_.size()].in();
    }
  }
}

#endif /*TRANSACTION_MAP_HPP*/
