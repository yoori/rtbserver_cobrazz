#pragma once

namespace AdServer
{
namespace ProfilingCommons
{
  /** ChunkedProfileMap */
  template<typename KeyType, typename ProfileMapType, typename KeyHashType>
  ChunkedProfileMap<KeyType, ProfileMapType, KeyHashType>::
  ChunkedProfileMap(
    unsigned long common_chunks_number,
    const ChunkIdToProfileMap& chunks,
    const KeyHashType& key_hash)
    /*throw(ChunkedProfileMap<KeyType, ProfileMapType, KeyHashType>::Exception,
      typename ProfileMapType::Exception)*/
    : key_hash_(key_hash),
      common_chunks_number_(common_chunks_number)
  {
    chunks_.resize(common_chunks_number);

    for(typename ChunkIdToProfileMap::const_iterator chunk_it =
          chunks.begin();
        chunk_it != chunks.end(); ++chunk_it)
    {
      if(chunk_it->first >= common_chunks_number)
      {
        throw Exception("incorrect chunk number");
      }

      chunks_[chunk_it->first] = chunk_it->second;
    }
  }

  template<typename KeyType, typename ProfileMapType, typename KeyHashType>
  void
  ChunkedProfileMap<KeyType, ProfileMapType, KeyHashType>::
  wait_preconditions(const KeyType& key, OperationPriority priority) const
    /*throw(ChunkNotFound, Exception)*/
  {
    return get_chunk_(key_hash_(key) % common_chunks_number_)->wait_preconditions(
      key,
      priority);
  }

  template<typename KeyType, typename ProfileMapType, typename KeyHashType>
  bool
  ChunkedProfileMap<KeyType, ProfileMapType, KeyHashType>::
  dispose_profile(const KeyType& key) const
    noexcept
  {
    return get_chunk_(key_hash_(key) % common_chunks_number_).in();
  }

  template<typename KeyType, typename ProfileMapType, typename KeyHashType>
  bool
  ChunkedProfileMap<KeyType, ProfileMapType, KeyHashType>::
  check_profile(const KeyType& key) const
    /*throw(ChunkNotFound, Exception)*/
  {
    try
    {
      return get_chunk(key_hash_(key) % common_chunks_number_)->check_profile(
        key);
    }
    catch(const ChunkNotFound&)
    {
      throw;
    }
    catch(const eh::Exception& ex)
    {
      throw Exception(ex.what());
    }
  }
  
  template<typename KeyType, typename ProfileMapType, typename KeyHashType>
  void
  ChunkedProfileMap<KeyType, ProfileMapType, KeyHashType>::
  check_profile_async(
    const KeyType& key,
    typename AsyncProfileMap<KeyType>::CheckCallback callback) const
  {
    try
    {
      get_chunk(key_hash_(key) % common_chunks_number_)->check_profile_async(
        key,
        std::move(callback));
    }
    catch(const ChunkNotFound& ex)
    {
      if(callback) callback(false, ex.what());
    }
    catch(const eh::Exception& ex)
    {
      if(callback) callback(false, ex.what());
    }
  }

  template<typename KeyType, typename ProfileMapType, typename KeyHashType>
  Generics::ConstSmartMemBuf_var
  ChunkedProfileMap<KeyType, ProfileMapType, KeyHashType>::
  get_profile(
    const KeyType& key,
    Generics::Time* last_access_time)
    /*throw(ChunkNotFound, Exception)*/
  {
    try
    {
      return get_chunk(key_hash_(key) % common_chunks_number_)->get_profile(
        key, last_access_time);
    }
    catch(const ChunkNotFound&)
    {
      throw;
    }
    catch(const eh::Exception& ex)
    {
      throw Exception(ex.what());
    }
  }

  template<typename KeyType, typename ProfileMapType, typename KeyHashType>
  Generics::ConstSmartMemBuf_var
  ChunkedProfileMap<KeyType, ProfileMapType, KeyHashType>::
  get_profile_async(
    const KeyType& key,
    typename AsyncProfileMap<KeyType>::GetCallback callback,
    std::optional<Generics::Time> last_access_time)
  {
    try
    {
      return get_chunk(key_hash_(key) % common_chunks_number_)->get_profile_async(
        key,
        std::move(callback),
        last_access_time);
    }
    catch(const ChunkNotFound& ex)
    {
      if(callback) callback(Generics::ConstSmartMemBuf_var(), ex.what());
    }
    catch(const eh::Exception& ex)
    {
      if(callback) callback(Generics::ConstSmartMemBuf_var(), ex.what());
    }

    return Generics::ConstSmartMemBuf_var();
  }

  template<typename KeyType, typename ProfileMapType, typename KeyHashType>
  void
  ChunkedProfileMap<KeyType, ProfileMapType, KeyHashType>::
  save_profile(
    const KeyType& key,
    const Generics::ConstSmartMemBuf* mem_buf,
    const Generics::Time& now,
    OperationPriority priority)
    /*throw(ChunkNotFound, Exception)*/
  {
    try
    {
      return get_chunk(key_hash_(key) % common_chunks_number_)->save_profile(
        key, mem_buf, now, priority);
    }
    catch(const ChunkNotFound&)
    {
      throw;
    }
    catch(const eh::Exception& ex)
    {
      throw Exception(ex.what());
    }
  }

  template<typename KeyType, typename ProfileMapType, typename KeyHashType>
  void
  ChunkedProfileMap<KeyType, ProfileMapType, KeyHashType>::
  save_profile_async(
    const KeyType& key,
    const Generics::ConstSmartMemBuf* mem_buf,
    const Generics::Time& now,
    typename AsyncProfileMap<KeyType>::SaveCallback callback)
  {
    try
    {
      get_chunk(key_hash_(key) % common_chunks_number_)->save_profile_async(
        key,
        mem_buf,
        now,
        std::move(callback));
    }
    catch(const ChunkNotFound& ex)
    {
      if(callback) callback(ex.what());
    }
    catch(const eh::Exception& ex)
    {
      if(callback) callback(ex.what());
    }
  }

  template<typename KeyType, typename ProfileMapType, typename KeyHashType>
  bool
  ChunkedProfileMap<KeyType, ProfileMapType, KeyHashType>::
  remove_profile(const KeyType& key, OperationPriority priority)
    /*throw(ChunkNotFound, Exception)*/
  {
    try
    {
      return get_chunk(key_hash_(key) % common_chunks_number_)->remove_profile(
        key, priority);
    }
    catch(const ChunkNotFound&)
    {
      throw;
    }
    catch(const eh::Exception& ex)
    {
      throw Exception(ex.what());
    }
  }

  template<typename KeyType, typename ProfileMapType, typename KeyHashType>
  void
  ChunkedProfileMap<KeyType, ProfileMapType, KeyHashType>::
  remove_profile_async(
    const KeyType& key,
    OperationPriority priority,
    typename AsyncProfileMap<KeyType>::RemoveCallback callback)
  {
    try
    {
      get_chunk(key_hash_(key) % common_chunks_number_)->remove_profile_async(
        key,
        priority,
        std::move(callback));
    }
    catch(const ChunkNotFound& ex)
    {
      if(callback) callback(false, ex.what());
    }
    catch(const eh::Exception& ex)
    {
      if(callback) callback(false, ex.what());
    }
  }

  template<typename KeyType, typename ProfileMapType, typename KeyHashType>
  typename ProfileMapType::Transaction_var
  ChunkedProfileMap<KeyType, ProfileMapType, KeyHashType>::
  get_transaction(
    const KeyType& key,
    bool check_max_waiters,
    OperationPriority op_priority)
    /*throw(ChunkNotFound,
      typename BaseProfileMap::MaxWaitersReached,
      typename BaseProfileMap::Exception)*/
  {
    return get_chunk(key_hash_(key) % common_chunks_number_)->get_transaction(
      key, check_max_waiters, op_priority);
  }

  template<typename KeyType, typename ProfileMapType, typename KeyHashType>
  AdServer::Commons::SyncCoro<typename ProfileMapType::Transaction_var>
  ChunkedProfileMap<KeyType, ProfileMapType, KeyHashType>::
  co_get_transaction(
    const KeyType& key,
    bool check_max_waiters,
    OperationPriority op_priority)
  {
    co_return co_await get_chunk(
      key_hash_(key) % common_chunks_number_)->co_get_transaction(
        key, check_max_waiters, op_priority);
  }

  template<typename KeyType, typename ProfileMapType, typename KeyHashType>
  void
  ChunkedProfileMap<KeyType, ProfileMapType, KeyHashType>::
  clear_expired_async(
    const Generics::Time& expire_time,
    typename AsyncProfileMap<KeyType>::CompleteCallback complete)
  {
    try
    {
      std::shared_ptr<std::atomic_size_t> remaining =
        std::make_shared<std::atomic_size_t>(0);
      for(typename ChunkArray::const_iterator chunk_it = chunks_.begin();
          chunk_it != chunks_.end();
          ++chunk_it)
      {
        if(chunk_it->in())
        {
          remaining->fetch_add(1, std::memory_order_relaxed);
        }
      }

      if(remaining->load(std::memory_order_relaxed) == 0)
      {
        if(complete) complete();
        return;
      }

      auto shared_complete = std::make_shared<typename AsyncProfileMap<KeyType>::CompleteCallback>(
        std::move(complete));
      for(typename ChunkArray::const_iterator chunk_it = chunks_.begin();
          chunk_it != chunks_.end();
          ++chunk_it)
      {
        if(chunk_it->in())
        {
          (*chunk_it)->clear_expired_async(
            expire_time,
            [remaining, shared_complete]() mutable
            {
              if(remaining->fetch_sub(1, std::memory_order_acq_rel) == 1 &&
                *shared_complete)
              {
                (*shared_complete)();
              }
            });
        }
      }
    }
    catch(...)
    {
      if(complete) complete();
    }
  }

  template<typename KeyType, typename ProfileMapType, typename KeyHashType>
  void
  ChunkedProfileMap<KeyType, ProfileMapType, KeyHashType>::
  process_keys(
    std::function<void(const KeyType&)> process_key,
    std::function<void(void)> process_complete)
    /*throw(Exception)*/
  {
    try
    {
      for(typename ChunkArray::const_iterator chunk_it = chunks_.begin();
          chunk_it != chunks_.end();
          ++chunk_it)
      {
        if(chunk_it->in())
        {
          (*chunk_it)->process_keys(process_key, std::function<void(void)>());
        }
      }

      if(process_complete)
      {
        process_complete();
      }
    }
    catch(const eh::Exception& ex)
    {
      throw Exception(ex.what());
    }
  }

  template<typename KeyType, typename ProfileMapType, typename KeyHashType>
  void
  ChunkedProfileMap<KeyType, ProfileMapType, KeyHashType>::
  clear_expired(const Generics::Time& expire_time)
    /*throw(Exception)*/
  {
    try
    {
      for(typename ChunkArray::const_iterator chunk_it = chunks_.begin();
          chunk_it != chunks_.end();
          ++chunk_it)
      {
        if(chunk_it->in())
        {
          (*chunk_it)->clear_expired(expire_time);
        }
      }
    }
    catch(const eh::Exception& ex)
    {
      throw Exception(ex.what());
    }
  }

  template<typename KeyType, typename ProfileMapType, typename KeyHashType>
  unsigned long
  ChunkedProfileMap<KeyType, ProfileMapType, KeyHashType>::
  size() const noexcept
  {
    unsigned long res = 0;
      
    for(typename ChunkArray::const_iterator chunk_it = chunks_.begin();
        chunk_it != chunks_.end();
        ++chunk_it)
    {
      if(chunk_it->in())
      {
        res += (*chunk_it)->size();
      }
    }

    return res;
  }

  template<typename KeyType, typename ProfileMapType, typename KeyHashType>
  unsigned long
  ChunkedProfileMap<KeyType, ProfileMapType, KeyHashType>::
  area_size() const noexcept
  {
    unsigned long res = 0;
      
    for(typename ChunkArray::const_iterator chunk_it = chunks_.begin();
        chunk_it != chunks_.end();
        ++chunk_it)
    {
      if(chunk_it->in())
      {
        res += (*chunk_it)->area_size();
      }
    }

    return res;
  }

  template<typename KeyType, typename ProfileMapType, typename KeyHashType>
  const ChunkIdSet&
  ChunkedProfileMap<KeyType, ProfileMapType, KeyHashType>::
  chunk_ids() const noexcept
  {
    return chunk_ids_;
  }

  template<typename KeyType, typename ProfileMapType, typename KeyHashType>
  typename ChunkedProfileMap<KeyType, ProfileMapType, KeyHashType>::
    BaseProfileMap_var
  ChunkedProfileMap<KeyType, ProfileMapType, KeyHashType>::
  get_chunk(unsigned long chunk_id) const
    /*throw(ChunkNotFound)*/
  {
    BaseProfileMap_var res = get_chunk_(chunk_id);

    if(!res.in())
    {
      Stream::Error ostr;
      ostr << "chunk #" << chunk_id << " isn't defined in container";
      throw ChunkNotFound(ostr);
    }

    return res;
  }

  template<typename KeyType, typename ProfileMapType, typename KeyHashType>
  const typename ChunkedProfileMap<KeyType, ProfileMapType, KeyHashType>::
  ChunkArray&
  ChunkedProfileMap<KeyType, ProfileMapType, KeyHashType>::chunks() const noexcept
  {
    return chunks_;
  }

  template<typename KeyType, typename ProfileMapType, typename KeyHashType>
  typename ChunkedProfileMap<KeyType, ProfileMapType, KeyHashType>::
    BaseProfileMap_var
  ChunkedProfileMap<KeyType, ProfileMapType, KeyHashType>::
  get_chunk_(unsigned long chunk_id) const noexcept
  {
    if(chunk_id > common_chunks_number_ || !chunks_[chunk_id].in())
    {
      return BaseProfileMap_var();
    }

    return chunks_[chunk_id];
  }
}
}
