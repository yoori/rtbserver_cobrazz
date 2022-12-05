#ifndef PLAINSTORAGE_CACHEDELEGATELAYER_HPP
#define PLAINSTORAGE_CACHEDELEGATELAYER_HPP

#include <Commons/LockMap.hpp>
#include "BlockCache.hpp"

namespace PlainStorage
{
  template<typename NextIndexType>
  class WriteCacheDelegateLayer:
    public virtual TransientPropertyWriteLayerImpl<NextIndexType, NextIndexType>,
    public virtual BlockAllocator<NextIndexType>
  {
  public:
    typedef NextIndexType NextBlockIndex;
    typedef NextIndexType BlockIndex;

    /**
     * Init table for blocks
     * @param next_layer The reference to next layer
     * @param allocator Need to be able to allocate the block and immediately
     * put it in the cache
     * @param max_cache_size The maximum size of table that caching write
     * blocks
     */
    WriteCacheDelegateLayer(
      WriteBaseLayer<NextBlockIndex>* next_layer,
      BlockAllocator<NextBlockIndex>* allocator,
      WriteBlockCache<NextBlockIndex>* write_block_cache)
      /*throw(BaseException)*/;

    /**
     * Delegates to get_write_block
     */
    virtual
    ReadBlock_var
    get_read_block(const BlockIndex& index)
      /*throw(BaseException, CorruptedRecord)*/;

    /**
     * Search in the container of opened blocks,
     * if found - returns from it. No - creating a write block and put it into
     * the cache.
     * @param index The index block that we need
     * @return The smart pointer to requested block
     */
    virtual
    WriteBlock_var
    get_write_block(const BlockIndex& index)
      /*throw(BaseException, CorruptedRecord)*/;

    virtual
    unsigned long area_size() const noexcept;

    /**
     * Blocks allocation service.
     * Allocated block is inserted into the cache immediately.
     */
    virtual void
    allocate(
      typename BlockAllocator<NextIndexType>::AllocatedBlock& alloc_result)
      /*throw(BaseException)*/;

  protected:
    typedef AdServer::Commons::StrictLockMap<
      Generics::NumericHashAdapter<NextBlockIndex>,
      Sync::Policy::PosixThread,
      AdServer::Commons::Hash2Args>
      BlockResolveLockMap;

  protected:
    virtual ~WriteCacheDelegateLayer() noexcept;

  protected:
    ReferenceCounting::SmartPtr<
      BlockAllocator<NextIndexType> > allocator_;
    ReferenceCounting::SmartPtr<
      WriteBlockCache<NextIndexType> > write_block_cache_;
    BlockResolveLockMap block_resolve_locks_;
  };
}

#include "CacheDelegateLayer.tpp"

#endif /*PLAINSTORAGE_CACHEDELEGATELAYER_HPP*/
