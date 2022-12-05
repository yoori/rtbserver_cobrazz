#ifndef MERGEITERATOR_HPP
#define MERGEITERATOR_HPP

#include <ReferenceCounting/AtomicImpl.hpp>
#include "BaseLevel.hpp"

namespace AdServer
{
namespace ProfilingCommons
{
  /* BaseMergeIterator
   * return all key states sorted by key and level index
   */
  template<typename KeyType, typename IteratorType>
  class BaseMergeIterator:
    public ReferenceCounting::AtomicImpl
  {
  public:
    BaseMergeIterator(
      const std::list<ReferenceCounting::SmartPtr<IteratorType> >& iterators)
      noexcept;

  protected:
    virtual ~BaseMergeIterator() noexcept
    {}

    virtual bool
    get_next_(
      KeyType& key,
      ProfileOperation& operation,
      Generics::Time& access_time)
      /*throw(typename ReadBaseLevel<KeyType>::Exception)*/;

  protected:
    typedef std::list<ReferenceCounting::SmartPtr<IteratorType> >
      IteratorList;

    struct IteratorStateKey
    {
      IteratorStateKey(
        const KeyType& key_val,
        unsigned long index_val)
        : key(key_val),
          index(index_val)
      {}

      bool
      operator<(const IteratorStateKey& right) const
      {
        return key < right.key ||
          (!(right.key < key) && index < right.index);
      };

      KeyType key;
      unsigned int index;
    };

    struct IteratorState
    {
      IteratorState(
        IteratorType* it_val,
        ProfileOperation operation_val,
        const Generics::Time& access_time_val)
        : it(it_val),
          operation(operation_val),
          access_time(access_time_val)
      {}

      IteratorType* it;
      ProfileOperation operation;
      Generics::Time access_time;
    };

    typedef std::map<IteratorStateKey, IteratorState>
      IteratorStateMap;

  protected:
    void
    next_step_()
      /*throw(typename ReadBaseLevel<KeyType>::Exception)*/;

  protected:
    bool initialized_;
    IteratorList iterator_list_;
    IteratorStateMap iterator_map_;
  };

  /* KeyMergeIterator
   *
   */
  template<typename KeyType>
  class KeyMergeIterator:
    public ReadBaseLevel<KeyType>::KeyIterator,
    public BaseMergeIterator<KeyType, typename ReadBaseLevel<KeyType>::KeyIterator>
  {
  public:
    KeyMergeIterator(
      const std::list<typename ReadBaseLevel<KeyType>::KeyIterator_var>&
        iterators)
      noexcept;

    virtual bool
    get_next(
      KeyType& key,
      ProfileOperation& operation,
      Generics::Time& access_time)
      /*throw(typename ReadBaseLevel<KeyType>::Exception)*/;

  protected:
    virtual ~KeyMergeIterator() noexcept
    {}
  };

  /* MergeIterator
   *
   */
  template<typename KeyType>
  class MergeIterator:
    public ReadBaseLevel<KeyType>::Iterator,
    public BaseMergeIterator<KeyType, typename ReadBaseLevel<KeyType>::Iterator>
  {
  public:
    MergeIterator(
      const std::list<typename ReadBaseLevel<KeyType>::Iterator_var>&
        iterators)
      noexcept;

    virtual bool
    get_next(
      KeyType& key,
      ProfileOperation& operation,
      Generics::Time& access_time)
      /*throw(typename ReadBaseLevel<KeyType>::Exception)*/;

    virtual Generics::ConstSmartMemBuf_var
    get_profile()
      /*throw(typename ReadBaseLevel<KeyType>::Exception)*/;

  protected:
    virtual ~MergeIterator() noexcept
    {}
  };

  /* OperationPackIterator
   * pack operations for equal keys
   * for saving case
   */
  template<typename KeyType, typename IterType>
  class OperationPackIteratorBase
  {
  public:
    OperationPackIteratorBase(
      IterType* source_iterator)
      noexcept;

  protected:
    virtual
    ~OperationPackIteratorBase() noexcept
    {}

    bool
    get_next_(
      KeyType& key,
      ProfileOperation& operation,
      Generics::Time& access_time)
      /*throw(typename ReadBaseLevel<KeyType>::Exception)*/;

    virtual void
    begin_chain_handler_() noexcept = 0;

    bool
    pack_key_operations_(
      KeyType& key,
      ProfileOperation& operation,
      Generics::Time& access_time)
      noexcept;

  protected:
    typename ReferenceCounting::SmartPtr<IterType> source_iterator_;
    bool initialized_;
    KeyType cur_key_;
    ProfileOperation cur_high_operation_;
    ProfileOperation cur_low_operation_;
    Generics::Time cur_access_time_;
  };

  template<typename KeyType>
  class OperationPackIterator:
    public OperationPackIteratorBase<KeyType, typename ReadBaseLevel<KeyType>::Iterator>,
    public ReadBaseLevel<KeyType>::Iterator,
    public ReferenceCounting::AtomicImpl
  {
  public:
    OperationPackIterator(
      typename ReadBaseLevel<KeyType>::Iterator* source_iterator)
      noexcept;

    virtual bool
    get_next(
      KeyType& key,
      ProfileOperation& operation,
      Generics::Time& access_time)
      /*throw(typename ReadBaseLevel<KeyType>::Exception)*/;

    virtual Generics::ConstSmartMemBuf_var
    get_profile()
      /*throw(typename ReadBaseLevel<KeyType>::Exception)*/;

  protected:
    virtual
    ~OperationPackIterator() noexcept
    {}

    virtual void
    begin_chain_handler_() noexcept;

  private:
    Generics::ConstSmartMemBuf_var cur_profile_;
  };

  template<typename KeyType>
  class KeyOperationPackIterator:
    public OperationPackIteratorBase<KeyType, typename ReadBaseLevel<KeyType>::KeyIterator>,
    public ReadBaseLevel<KeyType>::KeyIterator,
    public ReferenceCounting::AtomicImpl
  {
  public:
    KeyOperationPackIterator(
      typename ReadBaseLevel<KeyType>::KeyIterator* source_iterator)
      noexcept
      : OperationPackIteratorBase<KeyType, typename ReadBaseLevel<KeyType>::KeyIterator>(source_iterator)
    {}

    virtual bool
    get_next(
      KeyType& key,
      ProfileOperation& operation,
      Generics::Time& access_time)
      /*throw(typename ReadBaseLevel<KeyType>::Exception)*/
    {
      return this->get_next_(key, operation, access_time);
    }

  protected:
    virtual
    ~KeyOperationPackIterator() noexcept
    {}

    virtual void
    begin_chain_handler_() noexcept
    {}
  };

  template<typename KeyType>
  class AccessTimeFilterIterator:
    public ReadBaseLevel<KeyType>::Iterator,
    public ReferenceCounting::AtomicImpl
  {
  public:
    AccessTimeFilterIterator(
      typename ReadBaseLevel<KeyType>::Iterator* source_iterator,
      const Generics::Time& min_access_time)
      noexcept;

    virtual bool
    get_next(
      KeyType& key,
      ProfileOperation& operation,
      Generics::Time& access_time)
      /*throw(typename ReadBaseLevel<KeyType>::Exception)*/;

    virtual Generics::ConstSmartMemBuf_var
    get_profile()
      /*throw(typename ReadBaseLevel<KeyType>::Exception)*/;

  protected:
    virtual ~AccessTimeFilterIterator() noexcept
    {}

  private:
    typename ReadBaseLevel<KeyType>::Iterator_var source_iterator_;
    const Generics::Time min_access_time_;
  };
}
}

namespace AdServer
{
namespace ProfilingCommons
{
  // BaseMergeIterator
  template<typename KeyType, typename IteratorType>
  BaseMergeIterator<KeyType, IteratorType>::BaseMergeIterator(
    const std::list<ReferenceCounting::SmartPtr<IteratorType> >& iterators)
    noexcept
    : initialized_(false),
      iterator_list_(iterators)
  {}

  template<typename KeyType, typename IteratorType>
  bool
  BaseMergeIterator<KeyType, IteratorType>::get_next_(
    KeyType& key,
    ProfileOperation& operation,
    Generics::Time& access_time)
    /*throw(typename ReadBaseLevel<KeyType>::Exception)*/
  {
    if(!initialized_)
    {
      unsigned long cur_index = 0;
      for(typename IteratorList::const_iterator it = iterator_list_.begin();
          it != iterator_list_.end();
          ++it, ++cur_index)
      {
        KeyType cur_key;
        ProfileOperation cur_operation;
        Generics::Time cur_access_time;

        if((*it)->get_next(cur_key, cur_operation, cur_access_time))
        {
          iterator_map_.insert(std::make_pair(
            IteratorStateKey(cur_key, cur_index),
            IteratorState(*it, cur_operation, cur_access_time)));
        }
      }

      initialized_ = true;
    }
    else
    {
      if(iterator_map_.empty())
      {
        return false;
      }

      next_step_();
    }

    if(iterator_map_.empty())
    {
      return false;
    }

    key = iterator_map_.begin()->first.key;
    operation = iterator_map_.begin()->second.operation;
    access_time = iterator_map_.begin()->second.access_time;

    return true;
  }

  template<typename KeyType, typename IteratorType>
  void
  BaseMergeIterator<KeyType, IteratorType>::next_step_()
    /*throw(typename ReadBaseLevel<KeyType>::Exception)*/
  {
    assert(!iterator_map_.empty());

    typename IteratorStateMap::iterator next_it = iterator_map_.begin();
    unsigned int cur_index = next_it->first.index;
    IteratorState next_it_state = next_it->second;
    KeyType cur_key;
    ProfileOperation cur_operation;
    Generics::Time cur_access_time;

    bool ins_it = next_it_state.it->get_next(
      cur_key, cur_operation, cur_access_time);

    iterator_map_.erase(iterator_map_.begin());

    if(ins_it)
    {
      iterator_map_.insert(std::make_pair(
        IteratorStateKey(cur_key, cur_index),
        IteratorState(next_it_state.it, cur_operation, cur_access_time)));
    }
  }

  // KeyMergeIterator
  template<typename KeyType>
  KeyMergeIterator<KeyType>::KeyMergeIterator(
    const std::list<typename ReadBaseLevel<KeyType>::KeyIterator_var>&
      iterators)
    noexcept
    : BaseMergeIterator<KeyType, typename ReadBaseLevel<KeyType>::KeyIterator>(iterators)
  {}

  template<typename KeyType>
  bool
  KeyMergeIterator<KeyType>::get_next(
    KeyType& key,
    ProfileOperation& erased,
    Generics::Time& access_time)
    /*throw(typename ReadBaseLevel<KeyType>::Exception)*/
  {
    return this->get_next_(key, erased, access_time);
  }

  // MergeIterator
  template<typename KeyType>
  MergeIterator<KeyType>::MergeIterator(
    const std::list<typename ReadBaseLevel<KeyType>::Iterator_var>&
      iterators)
    noexcept
    : BaseMergeIterator<KeyType, typename ReadBaseLevel<KeyType>::Iterator>(iterators)
  {}

  template<typename KeyType>
  bool
  MergeIterator<KeyType>::get_next(
    KeyType& key,
    ProfileOperation& operation,
    Generics::Time& access_time)
    /*throw(typename ReadBaseLevel<KeyType>::Exception)*/
  {
    return this->get_next_(key, operation, access_time);
  }

  template<typename KeyType>
  Generics::ConstSmartMemBuf_var
  MergeIterator<KeyType>::get_profile()
    /*throw(typename ReadBaseLevel<KeyType>::Exception)*/
  {
    if(!this->iterator_map_.empty())
    {
      return this->iterator_map_.begin()->second.it->get_profile();
    }

    return Generics::ConstSmartMemBuf_var();
  }

  // KeyOperationPackIterator
  template<typename KeyType, typename IterType>
  OperationPackIteratorBase<KeyType, IterType>::OperationPackIteratorBase(
    IterType* source_iterator)
    noexcept
    : source_iterator_(ReferenceCounting::add_ref(source_iterator)),
      initialized_(false),
      cur_high_operation_(PO_NOT_FOUND),
      cur_low_operation_(PO_NOT_FOUND)
  {}

  template<typename KeyType, typename IterType>
  bool
  OperationPackIteratorBase<KeyType, IterType>::get_next_(
    KeyType& key,
    ProfileOperation& operation,
    Generics::Time& access_time)
    /*throw(typename ReadBaseLevel<KeyType>::Exception)*/
  {
    if(!initialized_)
    {
      initialized_ = true;

      if(!source_iterator_->get_next(
         cur_key_, cur_high_operation_, cur_access_time_))
      {
        return false;
      }
    }
    else if(cur_high_operation_ == PO_NOT_FOUND)
    {
      return false;
    }

    begin_chain_handler_();

    KeyType next_key;
    ProfileOperation next_operation;
    Generics::Time next_access_time;

    while(source_iterator_->get_next(next_key, next_operation, next_access_time))
    {
      if(cur_high_operation_ != PO_NOT_FOUND &&
         next_key == cur_key_)
      {
        cur_low_operation_ = next_operation;
        cur_access_time_ = std::max(cur_access_time_, next_access_time);
      }
      else
      {
        bool use_key = false;
        if(cur_high_operation_ != PO_NOT_FOUND)
        {
          use_key = pack_key_operations_(key, operation, access_time);
        }

        cur_key_ = next_key;
        cur_high_operation_ = next_operation;
        cur_access_time_ = next_access_time;
        cur_low_operation_ = PO_NOT_FOUND;

        if(use_key)
        {
          return true;
        }
        else
        {
          begin_chain_handler_();
        }
      }
    }

    // end reached
    const bool res = pack_key_operations_(key, operation, access_time);
    cur_high_operation_ = PO_NOT_FOUND;
    return res;
  }

  template<typename KeyType, typename IterType>
  bool
  OperationPackIteratorBase<KeyType, IterType>::pack_key_operations_(
    KeyType& key,
    ProfileOperation& operation,
    Generics::Time& access_time)
    noexcept
  {
    if(cur_high_operation_ == PO_INSERT)
    {
      if(cur_low_operation_ == PO_ERASE || cur_low_operation_ == PO_REWRITE)
      {
        // PO_INSERT, ..., PO_REWRITE|PO_ERASE
        operation = PO_REWRITE;
      }
      else
      {
        // PO_INSERT, ..., PO_INSERT|PO_NOT_FOUND
        operation = PO_INSERT;
      }
    }
    else if(cur_high_operation_ == PO_REWRITE)
    {
      if(cur_low_operation_ == PO_INSERT)
      {
        // PO_REWRITE, ..., PO_INSERT
        operation = PO_INSERT;
      }
      else
      {
        // PO_REWRITE, ..., PO_REWRITE|PO_ERASE|PO_NOT_FOUND
        operation = PO_REWRITE;
      }
    }
    else if(cur_low_operation_ == PO_ERASE ||
      cur_low_operation_ == PO_REWRITE ||
      cur_low_operation_ == PO_NOT_FOUND)
      // PO_ERASE, ..., PO_ERASE|PO_REWRITE|PO_NOT_FOUND
    {
      operation = PO_ERASE;
    }
    else // PO_ERASE, ..., PO_INSERT - skip
    {
      return false;
    }

    key = cur_key_;
    access_time = cur_access_time_;
    return true;
  }

  // OperationPackIterator
  template<typename KeyType>
  OperationPackIterator<KeyType>::OperationPackIterator(
    typename ReadBaseLevel<KeyType>::Iterator* source_iterator)
    noexcept
    : OperationPackIteratorBase<KeyType, typename ReadBaseLevel<KeyType>::Iterator>(source_iterator)
  {}

  template<typename KeyType>
  void
  OperationPackIterator<KeyType>::begin_chain_handler_() noexcept
  {
    if(this->cur_high_operation_ == PO_INSERT ||
        this->cur_high_operation_ == PO_REWRITE)
    {
      this->cur_profile_ = this->source_iterator_->get_profile();
    }
    else
    {
      this->cur_profile_ = Generics::ConstSmartMemBuf_var();
    }
  }

  template<typename KeyType>
  bool
  OperationPackIterator<KeyType>::get_next(
    KeyType& key,
    ProfileOperation& operation,
    Generics::Time& access_time)
    /*throw(typename ReadBaseLevel<KeyType>::Exception)*/
  {
    return this->get_next_(key, operation, access_time);
  }

  template<typename KeyType>
  Generics::ConstSmartMemBuf_var
  OperationPackIterator<KeyType>::get_profile()
    /*throw(typename ReadBaseLevel<KeyType>::Exception)*/
  {
    return cur_profile_;
  }

  // AccessTimeFilterIterator
  template<typename KeyType>
  AccessTimeFilterIterator<KeyType>::AccessTimeFilterIterator(
    typename ReadBaseLevel<KeyType>::Iterator* source_iterator,
    const Generics::Time& min_access_time)
    noexcept
    : source_iterator_(ReferenceCounting::add_ref(source_iterator)),
      min_access_time_(min_access_time)
  {}

  template<typename KeyType>
  bool
  AccessTimeFilterIterator<KeyType>::get_next(
    KeyType& key,
    ProfileOperation& operation,
    Generics::Time& access_time)
    /*throw(typename ReadBaseLevel<KeyType>::Exception)*/
  {
    // if record expired transform:
    //   PO_INSERT => skip
    //   PO_REWRITE => PO_ERASE
    //   PO_ERASE => PO_ERASE
    do
    {
      bool res = source_iterator_->get_next(
        key,
        operation,
        access_time);

      if(res && access_time < min_access_time_)
      {
        if(operation != PO_INSERT)
        {
          operation = PO_ERASE;
          return true;
        }
      }
      else
      {
        return res;
      }
    }
    while(true);

    return false;
  }

  template<typename KeyType>
  Generics::ConstSmartMemBuf_var
  AccessTimeFilterIterator<KeyType>::get_profile()
    /*throw(typename ReadBaseLevel<KeyType>::Exception)*/
  {
    return source_iterator_->get_profile();
  }
}
}

#endif /*MERGEITERATOR_HPP*/
