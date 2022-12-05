#ifndef PROFILEMAP_MEMINDEXPROFILEMAP_TPP
#define PROFILEMAP_MEMINDEXPROFILEMAP_TPP

#include <eh/Exception.hpp>
#include <ProfilingCommons/PlainStorage/DefaultSyncIndexStrategy.tpp>
#include <ProfilingCommons/PlainStorage/LayerFactory.hpp>

namespace AdServer
{
namespace ProfilingCommons
{
  /**
   * MapBase
   */
  template<typename KeyType, typename BlockIndexType,
    typename MapTraitsType, template<typename, typename> class ContainerType>
  MapBase<KeyType, BlockIndexType, MapTraitsType, ContainerType>::MapBase(
    const char* file,
    InterruptCallback* interrupt)
    /*throw (eh::Exception)*/
  {
    ReferenceCounting::SmartPtr<
      PlainStorage::WriteAllocateBaseLayer<BlockIndexType> >
      layer = PlainStorage::LayerFactory<Sync::Policy::PosixThreadRW>::
        create_write_record_layer(file);

    load(layer, layer, interrupt);
  }

  template<typename KeyType, typename BlockIndexType,
    typename MapTraitsType, template<typename, typename> class ContainerType>
  MapBase<KeyType, BlockIndexType, MapTraitsType, ContainerType>::MapBase(
    PlainStorage::WriteBaseLayer<BlockIndexType>* write_layer,
    PlainStorage::BlockAllocator<BlockIndexType>* allocator,
    InterruptCallback* interrupt) /*throw (eh::Exception)*/
  {
    load(write_layer, allocator, interrupt);
  }

  template<typename KeyType, typename BlockIndexType,
    typename MapTraitsType, template<typename, typename> class ContainerType>
  MapBase<KeyType, BlockIndexType, MapTraitsType, ContainerType>::MapBase(
    PlainStorage::WriteAllocateBaseLayer<BlockIndexType>* write_allocate_layer,
    InterruptCallback* interrupt)
    /*throw (eh::Exception)*/
  {
    load(write_allocate_layer, write_allocate_layer, interrupt);
  }

  template<typename KeyType, typename BlockIndexType,
    typename MapTraitsType, template<typename, typename> class ContainerType>
  MapBase<KeyType, BlockIndexType, MapTraitsType, ContainerType>::~MapBase()
    noexcept
  {
    close();
  }

  template<typename KeyType, typename BlockIndexType,
    typename MapTraitsType, template<typename, typename> class ContainerType>
  bool
  MapBase<KeyType, BlockIndexType, MapTraitsType, ContainerType>::
  check_block(const KeyType& key) const /*throw(Exception)*/
  {
    SyncPolicy::ReadGuard lock(index_container_lock_);
    typename IndexContainer::const_iterator it = index_container_.find(key);
    return it != index_container_.end();
  }

  template<typename KeyType, typename BlockIndexType,
    typename MapTraitsType, template<typename, typename> class ContainerType>
  PlainStorage::ReadBlock_var
  MapBase<KeyType, BlockIndexType, MapTraitsType, ContainerType>::
  get_read_block(const KeyType& key) /*throw(Exception)*/
  {
    static const char* FUN = "MapBase<>::get_read_block()";

    try
    {
      BlockIndexType block_index;

      {
        SyncPolicy::ReadGuard lock(index_container_lock_);
        typename IndexContainer::const_iterator it = index_container_.find(key);
        if(it == index_container_.end())
        {
          return PlainStorage::ReadBlock_var();
        }

        block_index = it->second.index;
      }

      return write_layer_->get_read_block(block_index);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  template<typename KeyType, typename BlockIndexType,
    typename MapTraitsType, template<typename, typename> class ContainerType>
  void
  MapBase<KeyType, BlockIndexType, MapTraitsType, ContainerType>::
  get_write_block(
    PlainStorage::WriteBlock_var& res,
    const KeyType& key,
    bool create_if_not_found)
    /*throw(Exception)*/
  {
    static const char* FUN = "MapBase<>::get_write_block()";

    try
    {
      bool found = false;
      BlockIndexType block_index;

      {
        SyncPolicy::ReadGuard lock(index_container_lock_);
        typename IndexContainer::iterator it = index_container_.find(key);

        if(it != index_container_.end())
        {
          found = true;
          block_index = it->second.index;
        }
      }

      if(found)
      {
        res = write_layer_->get_write_block(block_index);
      }
      else if(create_if_not_found)
      {
        ValueType ins_val = allocate_(res, key);
        bool inserted = true;

        {
          SyncPolicy::WriteGuard lock(index_container_lock_);
          std::pair<typename IndexContainer::iterator, bool> ins =
             index_container_.insert(std::make_pair(key, ins_val));
          if(!ins.second)
          {
            block_index = ins.first->second.index;
            inserted = false;
          }
        }

        if(!inserted)
        {
          res->deallocate();
          res = write_layer_->get_write_block(block_index);
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

  template<typename KeyType, typename BlockIndexType,
    typename MapTraitsType, template<typename, typename> class ContainerType>
  bool
  MapBase<KeyType, BlockIndexType, MapTraitsType, ContainerType>::
  remove_block(const KeyType& key)
    /*throw (Exception)*/
  {
    static const char* FUN = "MapBase<>::remove_block()";

    ValueType val;

    {
      SyncPolicy::WriteGuard lock(index_container_lock_);
      typename IndexContainer::iterator it = index_container_.find(key);

      if(it == index_container_.end())
      {
        return false;
      }

      val = it->second;
      index_container_.erase(it);
    }

    /*
    std::cerr << "erase('" << key << "'): ";
    val.key_add.print(std::cerr);
    std::cerr << std::endl;
    */

    try
    {
      sync_index_strategy_->erase(key, val.key_add, val.index);
      PlainStorage::WriteBlock_var block = write_layer_->get_write_block(val.index);
      block->deallocate();
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }

    return true;
  }

  template<typename KeyType, typename BlockIndexType,
    typename MapTraitsType, template<typename, typename> class ContainerType>
  unsigned long
  MapBase<KeyType, BlockIndexType, MapTraitsType, ContainerType>::
  size() const noexcept
  {
    typename SyncPolicy::ReadGuard lock(index_container_lock_);
    return index_container_.size();
  }

  template<typename KeyType, typename BlockIndexType,
    typename MapTraitsType, template<typename, typename> class ContainerType>
  unsigned long
  MapBase<KeyType, BlockIndexType, MapTraitsType, ContainerType>::
  area_size() const noexcept
  {
    return write_layer_->area_size();
  }

  template<typename KeyType, typename BlockIndexType,
    typename MapTraitsType, template<typename, typename> class ContainerType>
  bool
  MapBase<KeyType, BlockIndexType, MapTraitsType, ContainerType>::
  empty() const noexcept
  {
    typename SyncPolicy::ReadGuard lock(index_container_lock_);
    return index_container_.empty();
  }

  template<typename KeyType, typename BlockIndexType,
    typename MapTraitsType, template<typename, typename> class ContainerType>
  void
  MapBase<KeyType, BlockIndexType, MapTraitsType, ContainerType>::load(
    PlainStorage::WriteBaseLayer<BlockIndexType>* write_layer,
    PlainStorage::BlockAllocator<BlockIndexType>* allocator,
    InterruptCallback* /*interrupt*/)
    /*throw(eh::Exception)*/
  {
    write_layer_ = ReferenceCounting::add_ref(write_layer);
    block_allocator_ = ReferenceCounting::add_ref(allocator);

    sync_index_strategy_ = typename ThisType::SyncIndexStrategyPtr(
      new typename ThisType::SyncIndexStrategy(
        write_layer_,
        block_allocator_));

    sync_index_strategy_->load(this);
  }

  template<typename KeyType, typename BlockIndexType,
    typename MapTraitsType, template<typename, typename> class ContainerType>
  void
  MapBase<KeyType, BlockIndexType, MapTraitsType, ContainerType>::close()
    /*throw(eh::Exception)*/
  {
    if(sync_index_strategy_.get())
    {
      sync_index_strategy_->close();
      sync_index_strategy_.reset();
      block_allocator_.reset();
      write_layer_.reset();
    }
  }

  template<typename KeyType, typename BlockIndexType,
    typename MapTraitsType, template<typename, typename> class ContainerType>
  void
  MapBase<KeyType, BlockIndexType, MapTraitsType, ContainerType>::load(
    const KeyType& key,
    const typename SyncIndexStrategy::KeyAddition& key_add,
    const BlockIndexType& block_index)
    /*throw(eh::Exception)*/
  {
    ValueType val(block_index, key_add);
    index_container_.insert(std::make_pair(key, val));
  }

  template<typename KeyType, typename BlockIndexType,
    typename MapTraitsType, template<typename, typename> class ContainerType>
  typename MapBase<KeyType, BlockIndexType, MapTraitsType, ContainerType>::ValueType
  MapBase<KeyType, BlockIndexType, MapTraitsType, ContainerType>::
  allocate_(PlainStorage::WriteBlock_var& block, const KeyType& key)
    /*throw(Exception)*/
  {
    static const char* FUN = "PlainStorage::MapBase<>::allocate_()";

    try
    {
      typename PlainStorage::BlockAllocator<
        BlockIndexType>::AllocatedBlock alloc_block;
      block_allocator_->allocate(alloc_block);
      block = alloc_block.block;

      return ValueType(
        alloc_block.block_index,
        sync_index_strategy_->insert(key, alloc_block.block_index));
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": " << ex.what();
      throw Exception(ostr);
    }
  }

  template<typename KeyType, typename BlockIndexType,
    typename MapTraitsType, template<typename, typename> class ContainerType>
  void
  MapBase<KeyType, BlockIndexType, MapTraitsType, ContainerType>::
  copy_keys(std::list<KeyType>& keys)
    /*throw (eh::Exception)*/
  {
    typename SyncPolicy::ReadGuard lock(index_container_lock_);

    for (typename IndexContainer::const_iterator it = 
         index_container_.begin();
         it != index_container_.end(); ++it)
    {
      keys.push_back(it->first);
    }
  }

  /**
   * MemIndexProfileMap
   */
  template<typename KeyType, typename BlockIndexType,
    typename MapTraitsType, template<typename, typename> class ContainerType>
  MemIndexProfileMap<KeyType, BlockIndexType, MapTraitsType, ContainerType>::MemIndexProfileMap(
    const char* file,
    InterruptCallback* interrupt) /*throw (eh::Exception)*/
    : MapBaseType(file, interrupt)
  {}

  template<typename KeyType, typename BlockIndexType,
    typename MapTraitsType, template<typename, typename> class ContainerType>
  MemIndexProfileMap<KeyType, BlockIndexType, MapTraitsType, ContainerType>::MemIndexProfileMap(
    PlainStorage::WriteBaseLayer<BlockIndexType>* write_layer,
    PlainStorage::BlockAllocator<BlockIndexType>* allocator,
    InterruptCallback* interrupt) /*throw (eh::Exception)*/
    : MapBaseType(write_layer, allocator, interrupt)
  {}

  template<typename KeyType, typename BlockIndexType,
    typename MapTraitsType, template<typename, typename> class ContainerType>
  MemIndexProfileMap<KeyType, BlockIndexType, MapTraitsType, ContainerType>::MemIndexProfileMap(
    PlainStorage::WriteAllocateBaseLayer<BlockIndexType>* write_allocate_layer,
    InterruptCallback* interrupt)
    /*throw (eh::Exception)*/
    : MapBaseType(write_allocate_layer, write_allocate_layer, interrupt)
  {}

  template<typename KeyType, typename BlockIndexType,
    typename MapTraitsType, template<typename, typename> class ContainerType>
  MemIndexProfileMap<KeyType, BlockIndexType, MapTraitsType, ContainerType>::~MemIndexProfileMap()
    noexcept
  {}

  template<typename KeyType, typename BlockIndexType,
    typename MapTraitsType, template<typename, typename> class ContainerType>
  bool
  MemIndexProfileMap<KeyType, BlockIndexType, MapTraitsType, ContainerType>::
  check_profile(const KeyType& key) const /*throw(Exception)*/
  {
    return this->check_block(key);
  }

  template<typename KeyType, typename BlockIndexType,
    typename MapTraitsType, template<typename, typename> class ContainerType>
  Generics::ConstSmartMemBuf_var
  MemIndexProfileMap<KeyType, BlockIndexType, MapTraitsType, ContainerType>::
  get_profile(
    const KeyType& key,
    Generics::Time*)
    /*throw(Exception)*/
  {
    static const char* FUN = "MemIndexProfileMap<>::get_profile()";

    try
    {
      PlainStorage::ReadBlock_var block = this->get_read_block(key);
      if(block.in())
      {
        Generics::SmartMemBuf_var ret(new Generics::SmartMemBuf(block->size()));
        block->read(ret->membuf().data(), ret->membuf().size());
        return Generics::transfer_membuf(ret);
      }
      return Generics::ConstSmartMemBuf_var();
    }
    catch(const eh::Exception& ex)
    {
      // remove key from index without blocks deallocation
      this->index_container_.erase(key);

      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  template<typename KeyType, typename BlockIndexType,
    typename MapTraitsType, template<typename, typename> class ContainerType>
  bool
  MemIndexProfileMap<KeyType, BlockIndexType, MapTraitsType, ContainerType>::
  remove_profile(const KeyType& key, OperationPriority)
    /*throw(Exception)*/
  {
    static const char* FUN = "MemIndexProfileMap<>::remove_profile()";

    try
    {
      return this->remove_block(key);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  template<typename KeyType, typename BlockIndexType,
    typename MapTraitsType, template<typename, typename> class ContainerType>
  unsigned long
  MemIndexProfileMap<KeyType, BlockIndexType, MapTraitsType, ContainerType>::
  size() const noexcept
  {
    return MapBaseType::size();
  }

  template<typename KeyType, typename BlockIndexType,
    typename MapTraitsType, template<typename, typename> class ContainerType>
  unsigned long
  MemIndexProfileMap<KeyType, BlockIndexType, MapTraitsType, ContainerType>::
  area_size() const noexcept
  {
    return MapBaseType::area_size();
  }

  template<typename KeyType, typename BlockIndexType,
    typename MapTraitsType, template<typename, typename> class ContainerType>
  bool
  MemIndexProfileMap<KeyType, BlockIndexType, MapTraitsType, ContainerType>::
  empty() const noexcept
  {
    return MapBaseType::empty();
  }

  template<typename KeyType, typename BlockIndexType,
    typename MapTraitsType, template<typename, typename> class ContainerType>
  void
  MemIndexProfileMap<KeyType, BlockIndexType, MapTraitsType, ContainerType>::
  save_profile(
    const KeyType& key,
    const Generics::ConstSmartMemBuf* mem_buf,
    const Generics::Time& /*now*/,
    OperationPriority)
    /*throw (Exception)*/
  {
    static const char* FUN = "MemIndexProfileMap<>::save_profile()";

    try
    {
//    std::cerr << "save('" << key << "')" << std::endl;
      PlainStorage::WriteBlock_var block;
      this->get_write_block(block, key);
      block->write(mem_buf->membuf().data(), mem_buf->membuf().size());
    }
    catch(const eh::Exception& ex)
    {
      // remove key from index without blocks deallocation
      this->index_container_.erase(key);

      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  template<typename KeyType, typename BlockIndexType,
    typename MapTraitsType, template<typename, typename> class ContainerType>
  void
  MemIndexProfileMap<KeyType, BlockIndexType, MapTraitsType, ContainerType>::load(
    PlainStorage::WriteBaseLayer<BlockIndexType>* write_layer,
    PlainStorage::BlockAllocator<BlockIndexType>* allocator,
    InterruptCallback* interrupt)
    /*throw(eh::Exception)*/
  {
    MapBaseType::load(write_layer, allocator, interrupt);
  }

  template<typename KeyType, typename BlockIndexType,
    typename MapTraitsType, template<typename, typename> class ContainerType>
  void
  MemIndexProfileMap<KeyType, BlockIndexType, MapTraitsType, ContainerType>::close()
    /*throw(eh::Exception)*/
  {
    MapBaseType::close();
  }

  template<typename KeyType, typename BlockIndexType,
    typename MapTraitsType, template<typename, typename> class ContainerType>  
  void
  MemIndexProfileMap<KeyType, BlockIndexType, MapTraitsType, ContainerType>::
  copy_keys(typename ProfileMap<KeyType>::KeyList& keys)
    /*throw(Exception)*/
  {
    static const char* FUN = "MemIndexProfileMap::copy_keys()";

    try
    {
      typename MapBaseType::SyncPolicy::ReadGuard lock(
        this->index_container_lock_);

      for(typename MapBase<KeyType, BlockIndexType, MapTraitsType, ContainerType>::
          IndexContainer::const_iterator it =
            this->index_container_.begin();
          it != this->index_container_.end(); ++it)
      {
        keys.push_back(it->first);
      }
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw Exception(ostr);
    }
  }

  template<typename KeyType, typename BlockIndexType,
    typename MapTraitsType, template<typename, typename> class ContainerType>  
  std::unique_ptr<KeyType>
  MemIndexProfileMap<KeyType, BlockIndexType, MapTraitsType, ContainerType>::
  next_key(const KeyType* key)
    noexcept
  {
    static const char* FUN = "MemIndexProfileMap::next_key()";

    try
    {
      typename MapBaseType::SyncPolicy::ReadGuard lock(this->index_container_lock_);

      if(key)
      {
        typename MapBaseType::IndexContainer::const_iterator it =
          this->index_container_.find(*key);

        if(it != this->index_container_.end() &&
           ++it != this->index_container_.end())
        {
          return std::unique_ptr<KeyType>(new KeyType(it->first));
        }
      }
      else if(!this->index_container_.empty())
      {
        return std::unique_ptr<KeyType>(new KeyType(
          this->index_container_.begin()->first));
      }
      
      return std::unique_ptr<KeyType>();
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

#endif // PROFILEMAP_MEMINDEXPROFILEMAP_TPP
