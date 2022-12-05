#ifndef PROFILING_COMMONS_CHUNKED_EXPIRE_PROFILE_MAP_TPP
#define PROFILING_COMMONS_CHUNKED_EXPIRE_PROFILE_MAP_TPP

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
  void
  ChunkedProfileMap<KeyType, ProfileMapType, KeyHashType>::
  copy_keys(typename ProfileMap<KeyType>::KeyList& keys)
    /*throw(Exception)*/
  {
    try
    {
      for(typename ChunkList::const_iterator chunk_it = chunks_.begin();
          chunk_it != chunks_.end();
          ++chunk_it)
      {
        if(chunk_it->in())
        {
          (*chunk_it)->copy_keys(keys);
        }
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
      for(typename ChunkList::const_iterator chunk_it = chunks_.begin();
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
      
    for(typename ChunkList::const_iterator chunk_it = chunks_.begin();
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
      
    for(typename ChunkList::const_iterator chunk_it = chunks_.begin();
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
  ChunkList&
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

#endif /*PROFILING_COMMONS_CHUNKED_EXPIRE_PROFILE_MAP_TPP*/
