// @file PlainStorage/DefaultSyncIndexStrategy.tpp

#ifndef PLAINSTORAGE_DEFAULTSYNCINDEXSTRATEGY_TPP
#define PLAINSTORAGE_DEFAULTSYNCINDEXSTRATEGY_TPP

#include <signal.h>
#include <eh/Exception.hpp>
#include <Generics/ArrayAutoPtr.hpp>

#include "KeyBlockAdapter.hpp"

namespace PlainStorage
{
  template<
    typename KeyType,
    typename KeySerializerType,
    typename BlockIndexType,
    typename BlockIndexSerializerType>
  DefaultSyncIndexStrategy<
    KeyType, KeySerializerType, BlockIndexType, BlockIndexSerializerType>::
  DefaultSyncIndexStrategy(
    WriteBaseLayer<BlockIndexType>* write_layer,
    BlockAllocator<BlockIndexType>* block_allocator)
    /*throw(eh::Exception)*/
    : write_layer_(ReferenceCounting::add_ref(write_layer)),
      block_allocator_(ReferenceCounting::add_ref(block_allocator)),
      need_close_(false)
  {
    try
    {
      BlockIndexSerializerType block_index_serializer;

      bool second_block_exists = false;
      BlockIndexType second_block_index;

      /* get property & set it if not found. */
      PropertyValue prop;
      if(write_layer_->get_property("DefaultSyncIndexStrategy::KeyBlocksIndex", prop) &&
         prop.size() > 0)
      {
        BlockIndexType mb_index;
        block_index_serializer.load(prop.value(), prop.size(), mb_index);
        main_block_ = write_layer_->get_write_block(mb_index);
        KeyBlock key_block(main_block_);
        second_block_exists = key_block.next_block(second_block_index);
      }
      else
      {
        typename BlockAllocator<BlockIndexType>::AllocatedBlock alloc_block;
        block_allocator_->allocate(alloc_block);

        KeyBlock key_block(alloc_block.block);
        key_block.init();
        main_block_ = alloc_block.block;

        SizeType ind_size = block_index_serializer.size(alloc_block.block_index);
        Generics::ArrayAutoPtr<char> ind_buf(ind_size);
        block_index_serializer.save(alloc_block.block_index, ind_buf.get(), ind_size);
        prop.value(ind_buf.get(), ind_size);

        write_layer_->set_property("DefaultSyncIndexStrategy::KeyBlocksIndex", prop);
      }

      if(!second_block_exists)
      {
        typename BlockAllocator<BlockIndexType>::AllocatedBlock alloc_block;
        block_allocator_->allocate(alloc_block);

        KeyBlock key_block(alloc_block.block);
        key_block.init();

        /* cur block will be attached to main at close */
        need_close_ = true;
        cur_block_ = alloc_block.block;
        cur_block_index_ = alloc_block.block_index;
      }
      else
      {
        cur_block_ = write_layer_->get_write_block(second_block_index);
        cur_block_index_ = second_block_index;
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "Can't create DefaultSyncIndexStrategy. "
        "Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  template<
    typename KeyType,
    typename KeyAccessorType,
    typename BlockIndexType,
    typename BlockIndexSerializerType>
  typename DefaultSyncIndexStrategy<
    KeyType, KeyAccessorType, BlockIndexType, BlockIndexSerializerType>::KeyAddition
  DefaultSyncIndexStrategy<
    KeyType, KeyAccessorType, BlockIndexType, BlockIndexSerializerType>::insert(
      const KeyType& key,
      const BlockIndexType& block_index)
      /*throw (eh::Exception, CorruptedRecord)*/
  {
    if (!cur_block_.in())
    {
      Stream::Error ostr;
      ostr << "DefaultSyncIndexStrategy::insert(): "
        "Can't do insert after close";
      throw CorruptedRecord(ostr);
    }

    SyncPolicy::WriteGuard lock(lock_);

    SizeType new_key_pos;
    KeyBlock key_block(cur_block_);

    if(!(new_key_pos = key_block.push_back(key, block_index)))
    {
      close_();

      WriteBlock_var prev_cur_block = cur_block_;
      BlockIndexType prev_cur_block_index = cur_block_index_;

      typename BlockAllocator<BlockIndexType>::AllocatedBlock alloc_block;
      block_allocator_->allocate(alloc_block);
      cur_block_ = alloc_block.block;
      cur_block_index_ = alloc_block.block_index;

      KeyBlock new_key_block(cur_block_);
      new_key_block.init();

      if(!(new_key_pos = new_key_block.push_back(key, block_index)))
      {
        throw BaseException("Can't write first key record to empty block.");
      }

      // save previous used keys block index as next index new keys block
      if(prev_cur_block.in())
      {
        new_key_block.attach_next_block(prev_cur_block_index);
      }

      need_close_ = true;
    }

    /*
    {
      Stream::Error ostr;
      ostr << "inserted into: (" << cur_block_index_ << "), " << new_key_pos;
      std::cout << ostr.str() << std::endl;
    }
    */

    return KeyAddition(cur_block_index_, new_key_pos);
  }

  template<
    typename KeyType,
    typename KeyAccessorType,
    typename BlockIndexType,
    typename BlockIndexSerializerType>
  void
  DefaultSyncIndexStrategy<
    KeyType, KeyAccessorType, BlockIndexType, BlockIndexSerializerType>::
  erase(
    const KeyType& /*key*/,
    const KeyAddition& key_addition,
    const BlockIndexType&)
    /*throw(eh::Exception)*/
  {
    static const char* FUN = "DefaultSyncIndexStrategy<>::erase()";

    try
    {
      WriteBlock_var write_block = write_layer_->get_write_block(
        key_addition.block_index);

      KeyBlock key_block(write_block);

      //SyncPolicy::WriteGuard lock(lock_);
      key_block.erase(key_addition.block_offset);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception at erase from ("
        "index = (" << key_addition.block_index <<
        "), offset = " << key_addition.block_offset << "): " << ex.what();

      throw Exception(ostr);
    }
  }

  template<
    typename KeyType,
    typename KeyAccessorType,
    typename BlockIndexType,
    typename BlockIndexSerializerType>
  void
  DefaultSyncIndexStrategy<
    KeyType, KeyAccessorType, BlockIndexType, BlockIndexSerializerType>::
  load(IndexLoadCallback* index_load_callback)
    /*throw(eh::Exception)*/
  {
    try
    {
      /* loading index on start */
      BlockIndexType current_index;
      WriteBlock_var current_block = main_block_;
      KeyBlock key_block(current_block);

      /* ignore first(main) block - it must be empty */
      while(key_block.next_block(current_index))
      {
        current_block = write_layer_->get_write_block(current_index);
        key_block = KeyBlock(current_block);

        KeyType key;
        BlockIndexType key_block_index;
        SizeType current_pos = 0;
        SizeType key_pos = 0;

        while((current_pos = key_block.get(current_pos, key_pos, key, key_block_index)))
        {
          index_load_callback->load(
            key,
            KeyAddition(current_index, key_pos),
            key_block_index);
        }
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "DefaultSyncIndexStrategy: Can't load index. "
        "Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  template<
    typename KeyType,
    typename KeyAccessorType,
    typename BlockIndexType,
    typename BlockIndexSerializerType>
  void
  DefaultSyncIndexStrategy<
    KeyType, KeyAccessorType, BlockIndexType, BlockIndexSerializerType>::
  close()
    /*throw(eh::Exception)*/
  {
    if(need_close_)
    {
      close_();
    }
    cur_block_.reset();
    main_block_.reset();
  }

  template<
    typename KeyType,
    typename KeyAccessorType,
    typename BlockIndexType,
    typename BlockIndexSerializerType>
  void
  DefaultSyncIndexStrategy<
    KeyType, KeyAccessorType, BlockIndexType, BlockIndexSerializerType>::
  close_()
    /*throw(eh::Exception)*/
  {
    KeyBlock key_block(main_block_);
    key_block.attach_next_block(cur_block_index_);
  }
}

#endif // PLAINSTORAGE_DEFAULTSYNCINDEXSTRATEGY_TPP
