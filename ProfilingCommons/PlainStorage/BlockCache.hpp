#ifndef PLAINSTORAGE_BLOCKCACHE_HPP
#define PLAINSTORAGE_BLOCKCACHE_HPP

#include <list>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <Generics/GnuHashTable.hpp>
#include <Sync/SyncPolicy.hpp>

#include "BaseLayer.hpp"

namespace PlainStorage
{
  class PointerHashAdapter
  {
  public:
    PointerHashAdapter() noexcept
      : value_()
    {}

    PointerHashAdapter(const void* value) noexcept
      : value_(value)
    {}

    bool
    operator==(const PointerHashAdapter& right) const noexcept
    {
      return value_ == right.value_;
    }

    size_t hash() const noexcept
    {
      return static_cast<const char*>(value_) - static_cast<const char*>(0);
    }

    const void* value() const noexcept
    {
      return value_;
    }

  protected:
    const void* value_;
  };

  /* WriteBlockCache
   */
  template<typename IndexType>
  class WriteBlockCache: public ReferenceCounting::AtomicImpl
  {
  public:
    WriteBlockCache(unsigned long max_block_count)
      noexcept;

    WriteBlock_var
    get(const void* owner, const IndexType& index)
      noexcept;

    /**
     * @return block that displaced from cache
     */
    WriteBlock_var
    insert(const void* owner,
      const IndexType& index,
      WriteBlock* write_block);

    void
    clean_owner(const void* owner)
      noexcept;

  protected:
    typedef Sync::Policy::PosixThread SyncPolicy;

    struct Order
    {
      Order(const void* owner_val, IndexType index_val)
        : owner(owner_val), index(index_val)
      {}

      const void* owner;
      IndexType index;
    };

    typedef std::list<Order> OrderList;

    struct BlockHolder
    {
      BlockHolder(WriteBlock* write_block)
        : block(ReferenceCounting::add_ref(write_block))
      {}

      WriteBlock_var block;
      typename OrderList::iterator order_it;
    };

    typedef Generics::GnuHashTable<
      Generics::NumericHashAdapter<IndexType>,
      BlockHolder>
      BlockMap;

    typedef Generics::GnuHashTable<
      PointerHashAdapter,
      BlockMap>
      OwnerBlockMap;

  protected:
    virtual ~WriteBlockCache() noexcept
    {}

    void
    erase_(WriteBlock_var& release_block, const void* owner, const IndexType& index)
      noexcept;

  protected:
    const unsigned long max_block_count_;

    mutable SyncPolicy::Mutex lock_;
    OwnerBlockMap blocks_;
    OrderList block_order_;
    unsigned long block_order_size_;
  };
}

namespace PlainStorage
{
  class WriteBlockImpl: public WriteBlock
  {
  };

  template<typename IndexType>
  WriteBlockCache<IndexType>::WriteBlockCache(
    unsigned long max_block_count)
    noexcept
    : max_block_count_(max_block_count),
      block_order_size_(0)
  {}

  template<typename IndexType>
  WriteBlock_var
  WriteBlockCache<IndexType>::get(
    const void* owner, const IndexType& index)
    noexcept
  {
    WriteBlock_var ret;
    SyncPolicy::WriteGuard lock(lock_);

    //assert(block_order_size_ == block_order_.size());

    typename OwnerBlockMap::const_iterator owner_it =
      blocks_.find(owner);

    if(owner_it != blocks_.end())
    {
      typename BlockMap::const_iterator block_it =
        owner_it->second.find(index);

      if(block_it != owner_it->second.end())
      {
        block_order_.splice(
          block_order_.end(), block_order_, block_it->second.order_it);

        //assert(block_order_size_ == block_order_.size());

        ret = block_it->second.block;
      }
    }

    //assert(block_order_size_ == block_order_.size());

    return ret;
  }

  template<typename IndexType>
  void
  WriteBlockCache<IndexType>::erase_(
    WriteBlock_var& release_block,
    const void* owner,
    const IndexType& index)
    noexcept
  {
    typename OwnerBlockMap::iterator owner_it =
      blocks_.find(owner);

    if(owner_it != blocks_.end())
    {
      typename BlockMap::iterator block_it =
        owner_it->second.find(index);

      if(block_it != owner_it->second.end())
      {
        release_block.swap(block_it->second.block);
        owner_it->second.erase(block_it);
      }
    }
  }

  template<typename IndexType>
  WriteBlock_var
  WriteBlockCache<IndexType>::insert(
    const void* owner,
    const IndexType& index,
    WriteBlock* write_block)
  {
    WriteBlock_var ret;
    OrderList block_order_to_free;

    SyncPolicy::WriteGuard lock(lock_);

    //assert(block_order_size_ == block_order_.size());

    typename OwnerBlockMap::iterator owner_it =
      blocks_.find(owner);

    if(owner_it == blocks_.end())
    {
      owner_it = blocks_.insert(
        std::make_pair(owner, BlockMap())).first;
    }

    std::pair<typename BlockMap::iterator, bool> ins_block =
      owner_it->second.insert(
        std::make_pair(
          Generics::NumericHashAdapter<IndexType>(index),
          BlockHolder(write_block)));

    if(ins_block.second)
    {
      typename OrderList::iterator order_it =
        block_order_.insert(block_order_.end(), Order(owner, index));
      ins_block.first->second.order_it = order_it;

      ++block_order_size_;

      if(block_order_size_ > max_block_count_)
      {
        --block_order_size_;
        const Order erase_order = block_order_.front();
        erase_(ret, erase_order.owner, erase_order.index);
        block_order_to_free.splice(
          block_order_to_free.end(),
          block_order_,
          block_order_.begin());

        //assert(block_order_size_ == block_order_.size());
      }
    }
    else
    {
      block_order_.splice(
        block_order_.end(), block_order_, ins_block.first->second.order_it);
    }

    //assert(block_order_size_ == block_order_.size());

    return ret;
  }

  template<typename IndexType>
  void
  WriteBlockCache<IndexType>::clean_owner(const void* owner)
    noexcept
  {
    BlockMap blocks_to_free;
    OrderList block_order_to_free;

    SyncPolicy::WriteGuard lock(lock_);

    //assert(block_order_size_ == block_order_.size());

    typename OwnerBlockMap::iterator owner_it =
      blocks_.find(owner);

    if(owner_it != blocks_.end())
    {
      blocks_to_free.swap(owner_it->second);
      blocks_.erase(owner_it);

      for(typename BlockMap::const_iterator it = blocks_to_free.begin();
          it != blocks_to_free.end(); ++it)
      {
        block_order_to_free.splice(
          block_order_to_free.end(),
          block_order_,
          it->second.order_it);
      }

      block_order_size_ -= blocks_to_free.size();
    }

    //assert(block_order_size_ == block_order_.size());
  }
}

#endif /*PLAINSTORAGE_BLOCKCACHE_HPP*/
