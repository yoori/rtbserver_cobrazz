#ifndef PROFILEMAP_HASHINDEXPROFILEMAP_TPP
#define PROFILEMAP_HASHINDEXPROFILEMAP_TPP

namespace AdServer
{
namespace ProfilingCommons
{
  /* HashIndexProfileMap */
  template<
    typename KeyType,
    typename BlockIndexType,
    typename MapTraitsType>
  HashIndexProfileMap<KeyType, BlockIndexType, MapTraitsType>::
  HashIndexProfileMap(const char* file)
    /*throw (eh::Exception)*/
  {
    ReferenceCounting::SmartPtr<
      PlainStorage::LayerFactory<Sync::Policy::PosixThreadRW>::WriteRecordLayerT>
      layer = PlainStorage::LayerFactory<Sync::Policy::PosixThreadRW>::
        create_write_record_layer(file);

    write_layer_ = layer;
    block_allocator_ = layer;
    keys_block_map_ = new KeyMap(layer);
  }

  template<
    typename KeyType,
    typename BlockIndexType,
    typename MapTraitsType>
  HashIndexProfileMap<KeyType, BlockIndexType, MapTraitsType>::
  HashIndexProfileMap(
    PlainStorage::WriteBaseLayer<BlockIndexType>* write_layer,
    PlainStorage::BlockAllocator<BlockIndexType>* allocator)
    /*throw (eh::Exception)*/
    : write_layer_(ReferenceCounting::add_ref(write_layer)),
      block_allocator_(ReferenceCounting::add_ref(allocator)),
      keys_block_map_(new KeyMap(write_layer, allocator))
  {}

  template<
    typename KeyType,
    typename BlockIndexType,
    typename MapTraitsType>
  HashIndexProfileMap<KeyType, BlockIndexType, MapTraitsType>::
  HashIndexProfileMap(
    PlainStorage::WriteAllocateBaseLayer<BlockIndexType>* write_allocate_layer)
    /*throw (eh::Exception)*/
    : write_layer_(ReferenceCounting::add_ref(write_allocate_layer)),
      block_allocator_(ReferenceCounting::add_ref(write_allocate_layer)),
      keys_block_map_(new KeyMap(write_allocate_layer))
  {}

  template<
    typename KeyType,
    typename BlockIndexType,
    typename MapTraitsType>
  HashIndexProfileMap<KeyType, BlockIndexType, MapTraitsType>::
  ~HashIndexProfileMap()
    noexcept
  {
    close();
  }

  template<typename KeyType,
    typename BlockIndexType,
    typename MapTraitsType>
  void
  HashIndexProfileMap<KeyType, BlockIndexType, MapTraitsType>::
  load(
    PlainStorage::WriteBaseLayer<BlockIndexType>* write_layer,
    PlainStorage::BlockAllocator<BlockIndexType>* allocator) /*throw (eh::Exception)*/
  {
    write_layer_ = ReferenceCounting::add_ref(write_layer);
    block_allocator_ = ReferenceCounting::add_ref(allocator);
    keys_block_map_->load(write_layer, allocator);
  }

  template<typename KeyType,
    typename BlockIndexType,
    typename MapTraitsType>
  void
  HashIndexProfileMap<KeyType, BlockIndexType, MapTraitsType>::
  close() /*throw (eh::Exception)*/
  {
    write_layer_.reset();
    block_allocator_.reset();
    keys_block_map_->close();
  }

  template<typename KeyType,
    typename BlockIndexType,
    typename MapTraitsType>
  bool
  HashIndexProfileMap<KeyType, BlockIndexType, MapTraitsType>::
  check_profile(const KeyType& key) const
    /*throw (Exception)*/
  {
    static const char* FUN = "HashIndexProfileMap<>::check_profile()";

    try
    {
      KeyBlockLockMap::ReadGuard block_lock(
        keys_block_lock_map_.read_lock(key.hash()));
      PlainStorage::WriteBlock_var write_key_block;
      keys_block_map_->get_write_block(
        write_key_block, key.hash(), false);

      if(!write_key_block.in())
      {
        return false;
      }

      KeyBlock key_block(write_key_block);

      PlainStorage::SizeType pos = key_block.find(key);
      if(pos == 0)
      {
        return false;
      }

      PlainStorage::SizeType key_pos;
      KeyType key;
      BlockIndexType block_index;
      return key_block.get(pos, key_pos, key, block_index, false);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  template<typename KeyType,
    typename BlockIndexType,
    typename MapTraitsType>
  Generics::ConstSmartMemBuf_var
  HashIndexProfileMap<KeyType, BlockIndexType, MapTraitsType>::
  get_profile(
    const KeyType& key,
    Generics::Time*)
    /*throw (Exception)*/
  {
    static const char* FUN = "HashIndexProfileMap<>::get_profile()";

    try
    {
      BlockIndexType block_index;

      {
        KeyBlockLockMap::ReadGuard block_lock(
          keys_block_lock_map_.read_lock(key.hash()));

        PlainStorage::WriteBlock_var write_key_block;
        keys_block_map_->get_write_block(
          write_key_block, key.hash(), false);

        if(!write_key_block.in())
        {
          return Generics::ConstSmartMemBuf_var();
        }

        KeyBlock key_block(write_key_block);

        PlainStorage::SizeType pos = key_block.find(key);
        if(pos == 0)
        {
          return Generics::ConstSmartMemBuf_var();
        }

        PlainStorage::SizeType key_pos;
        KeyType key;
        if(key_block.get(pos, key_pos, key, block_index, false) == 0)
        {
          return Generics::ConstSmartMemBuf_var();
        }
      }

      PlainStorage::ReadBlock_var block =
        write_layer_->get_read_block(block_index);

      if(block.in())
      {
        Generics::SmartMemBuf_var ret(
          new Generics::SmartMemBuf(block->size()));
        block->read(ret->membuf().data(), ret->membuf().size());
        return Generics::transfer_membuf(ret);
      }

      return Generics::ConstSmartMemBuf_var();
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  template<
    typename KeyType,
    typename BlockIndexType,
    typename MapTraitsType>
  void
  HashIndexProfileMap<KeyType, BlockIndexType, MapTraitsType>::
  save_profile(
    const KeyType& key,
    const Generics::ConstSmartMemBuf* mem_buf,
    const Generics::Time&,
    OperationPriority)
    /*throw (Exception)*/
  {
    static const char* FUN = "HashIndexProfileMap<>::save_profile()";

    try
    {
      Hash hash_val = key.hash();

      PlainStorage::WriteBlock_var block;
      BlockIndexType block_index;
      
      {
        KeyBlockLockMap::WriteGuard block_lock(
          keys_block_lock_map_.write_lock(hash_val));

        PlainStorage::WriteBlock_var write_key_block;
        keys_block_map_->get_write_block(write_key_block, hash_val, true);

        KeyBlock key_block(write_key_block);
        bool inited = false;
        if(write_key_block->size() == 0)
        {
          key_block.init(PlainStorage::KeyBlockAdapter_::RESIZE_PORTION);
          inited = true;
        }

        PlainStorage::SizeType pos = 0;
        if(!inited)
        {
          pos = key_block.find(key);
        }

        if(pos == 0)
        {
          typename PlainStorage::BlockAllocator<BlockIndexType>::AllocatedBlock alloc_block;
          block_allocator_->allocate(alloc_block); // REVIEW
          key_block.push_back(key, alloc_block.block_index, true);
          block.swap(alloc_block.block);
        }
        else
        {
          KeyType readed_key;
          PlainStorage::SizeType readed_key_pos;
          key_block.get(pos, readed_key_pos, readed_key, block_index, false);
        }
      }

      if(!block)
      {
        block = write_layer_->get_write_block(block_index);
      }

      block->write(mem_buf->membuf().data(), mem_buf->membuf().size());
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  template<
    typename KeyType,
    typename BlockIndexType,
    typename MapTraitsType>
  bool
  HashIndexProfileMap<KeyType, BlockIndexType, MapTraitsType>::
  remove_profile(const KeyType& key, OperationPriority)
    /*throw (Exception)*/
  {
    static const char* FUN = "HashIndexProfileMap<>::remove_profile()";

    try
    {
      BlockIndexType block_index;

      {
        KeyBlockLockMap::WriteGuard key_block_lock(
          keys_block_lock_map_.write_lock(key.hash()));
        PlainStorage::WriteBlock_var write_key_block;
        keys_block_map_->get_write_block(write_key_block, key.hash(), false);

        if(!write_key_block.in())
        {
          return false;
        }

        KeyBlock key_block(write_key_block);
        PlainStorage::SizeType pos = key_block.find(key);

        if(pos == 0)
        {
          return false;
        }

        PlainStorage::SizeType readed_key_pos;
        KeyType readed_key;
        key_block.get(pos, readed_key_pos, readed_key, block_index, false);
        key_block.erase(pos);
      }

      PlainStorage::WriteBlock_var block = write_layer_->get_write_block(block_index);
      block->deallocate();
      return true;
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  template<
    typename KeyType,
    typename BlockIndexType,
    typename MapTraitsType>
  unsigned long
  HashIndexProfileMap<KeyType, BlockIndexType, MapTraitsType>::
  size() const
    noexcept
  {
    return keys_block_map_->size();
  }

  template<
    typename KeyType,
    typename BlockIndexType,
    typename MapTraitsType>
  unsigned long
  HashIndexProfileMap<KeyType, BlockIndexType, MapTraitsType>::
  area_size() const
    noexcept
  {
    return write_layer_->area_size();
  }

  template<
    typename KeyType,
    typename BlockIndexType,
    typename MapTraitsType>
  void
  HashIndexProfileMap<KeyType, BlockIndexType, MapTraitsType>::
    copy_keys(typename ProfileMap<KeyType>::KeyList& keys) 
    /*throw (Exception)*/
  {
    static const char* FUN = "HashIndexProfileMap::copy_keys()";

    try
    {
      std::list<Hash> hashes;
      keys_block_map_->copy_keys(hashes);
      
      for (std::list<Hash>::const_iterator it = hashes.begin();
           it != hashes.end(); ++it)
      {
        KeyBlockLockMap::ReadGuard block_lock(
          keys_block_lock_map_.read_lock(*it));

        PlainStorage::WriteBlock_var write_key_block;
        keys_block_map_->get_write_block(
          write_key_block, *it, false);

        if (write_key_block.in())
        {
          const KeyBlock key_block(write_key_block);
          key_block.copy_keys(keys);
        }
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }
}
}

#endif /*PROFILEMAP_HASHINDEXPROFILEMAP_TPP*/
