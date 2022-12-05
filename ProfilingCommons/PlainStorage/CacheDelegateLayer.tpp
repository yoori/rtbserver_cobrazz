// @file PlainStorage/CacheDelegateLayer.tpp

#include <iostream>
#include <map>

namespace PlainStorage
{
  /**
   * WriteCacheLayer
   */
  template<typename NextIndexType>
  WriteCacheDelegateLayer<NextIndexType>::WriteCacheDelegateLayer(
    WriteBaseLayer<NextBlockIndex>* next_layer,
    BlockAllocator<NextBlockIndex>* allocator,
    WriteBlockCache<NextBlockIndex>* write_block_cache)
    /*throw(BaseException)*/
    : TransientPropertyReadLayerImpl<NextIndexType, NextIndexType>(next_layer),
      TransientPropertyWriteLayerImpl<NextIndexType, NextIndexType>(next_layer),
      allocator_(ReferenceCounting::add_ref(allocator)),
      write_block_cache_(ReferenceCounting::add_ref(write_block_cache))
  {}

  template<typename NextIndexType>
  WriteCacheDelegateLayer<NextIndexType>::~WriteCacheDelegateLayer()
    noexcept
  {
    write_block_cache_->clean_owner(this);
  }

  template<typename NextIndexType>
  ReadBlock_var
  WriteCacheDelegateLayer<NextIndexType>::get_read_block(
    const NextBlockIndex& block_index)
    /*throw(BaseException, CorruptedRecord)*/
  {
    return get_write_block(block_index);
  }

  template<typename NextIndexType>
  WriteBlock_var
  WriteCacheDelegateLayer<NextIndexType>::get_write_block(
    const NextIndexType& block_index)
    /*throw(BaseException, CorruptedRecord)*/
  {
    WriteBlock_var block_to_free;

    {
      typename BlockResolveLockMap::WriteGuard block_lock(
        block_resolve_locks_.write_lock(block_index));

      WriteBlock_var res =
        write_block_cache_->get(this, block_index);

      if(res)
      {
        return res;
      }

      res = this->next_layer()->get_write_block(block_index);

      block_to_free = write_block_cache_->insert(
        this, block_index, res);

      return res;
    }
  }

  template<typename NextIndexType>
  unsigned long
  WriteCacheDelegateLayer<NextIndexType>::area_size() const noexcept
  {
    return this->next_layer()->area_size();
  }

  template<typename NextIndexType>
  void
  WriteCacheDelegateLayer<NextIndexType>::allocate(
    typename BlockAllocator<NextIndexType>::AllocatedBlock& alloc_result)
    /*throw(BaseException)*/
  {
    allocator_->allocate(alloc_result);

    alloc_result.block = alloc_result.block;

    write_block_cache_->insert(
      this,
      alloc_result.block_index,
      alloc_result.block);
  }
} // namespace PlainStorage
