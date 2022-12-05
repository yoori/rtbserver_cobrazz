namespace AdServer
{
namespace ProfilingCommons
{
  // MemLevelHolder
  template<typename KeyType>
  MemLevelHolder<KeyType>::MemLevelHolder()
    noexcept
    : size_(0),
      area_size_(0),
      merge_free_size_(0)
  {}

  template<typename KeyType>
  CheckProfileResult
  MemLevelHolder<KeyType>::check_profile_i(
    const KeyType& key) const
    /*throw(typename ReadBaseLevel<KeyType>::Exception)*/
  {
    CheckProfileResult result;

    typename ProfileHolderMap::const_iterator it = profiles_.find(key);
    if(it != profiles_.end())
    {
      result.operation = static_cast<ProfileOperation>(it->second.operation);
      
      if(it->second.operation == PO_ERASE)
      {
        result.size = it->second.next_size;
      }
      else
      {
        result.size = it->second.mem_buf->membuf().size();
      }
    }
    else
    {
      result.operation = PO_NOT_FOUND;
      result.size = 0;
    }

    return result;
  }

  template<typename KeyType>
  GetProfileResult
  MemLevelHolder<KeyType>::get_profile_i(
    const KeyType& key) const
    /*throw(typename ReadBaseLevel<KeyType>::Exception)*/
  {
    GetProfileResult result;

    typename ProfileHolderMap::const_iterator it = profiles_.find(key);
    if(it != profiles_.end())
    {
      result.mem_buf = it->second.mem_buf;
      result.operation = static_cast<ProfileOperation>(it->second.operation);
    }
    else
    {
      result.operation = PO_NOT_FOUND;
    }

    return result;
  }

  template<typename KeyType>
  unsigned long
  MemLevelHolder<KeyType>::size_i() const
    noexcept
  {
    return size_;
  }

  template<typename KeyType>
  uint64_t
  MemLevelHolder<KeyType>::area_size_i() const
    noexcept
  {
    return area_size_;
  }

  template<typename KeyType>
  uint64_t
  MemLevelHolder<KeyType>::merge_free_size_i() const
    noexcept
  {
    return merge_free_size_;
  }

  template<typename KeyType>
  Generics::Time
  MemLevelHolder<KeyType>::min_access_time_i() const
    noexcept
  {
    return min_access_time_;
  }

  template<typename KeyType>
  bool
  MemLevelHolder<KeyType>::get_first_i(
    KeyType& key,
    Generics::ConstSmartMemBuf_var& mem_buf) const
    noexcept
  {
    typename ProfileHolderMap::const_iterator it = profiles_.begin();
    while(it != profiles_.end())
    {
      if(it->second.erased)
      {
        key = it->first;
        mem_buf = it->second.mem_buf;
        return true;
      }

      ++it;
    }

    return false;
  }

  template<typename KeyType>
  unsigned long
  MemLevelHolder<KeyType>::eval_area_size_(
    const ProfileHolder& holder)
    noexcept
  {
    return sizeof(ProfileHolder) + (
      holder.mem_buf.in() ? holder.mem_buf->membuf().size() : 0);
  }

  // ReadMemLevel::KeyIteratorImpl
  template<typename KeyType>
  ReadMemLevel<KeyType>::KeyIteratorImpl::KeyIteratorImpl(
    const ReadMemLevel<KeyType>* read_mem_level)
    noexcept
    : read_mem_level_(ReferenceCounting::add_ref(read_mem_level)),
      profiles_it_(read_mem_level_->profiles_.end())
  {}

  template<typename KeyType>
  bool
  ReadMemLevel<KeyType>::KeyIteratorImpl::get_next(
    KeyType& key,
    ProfileOperation& operation,
    Generics::Time& access_time)
    noexcept
  {
    if(profiles_it_ == read_mem_level_->profiles_.end())
    {
      profiles_it_ = read_mem_level_->profiles_.begin();
    }
    else
    {
      ++profiles_it_;
    }

    if(profiles_it_ != read_mem_level_->profiles_.end())
    {
      key = profiles_it_->first;
      operation = static_cast<ProfileOperation>(profiles_it_->second.operation);
      access_time = Generics::Time(profiles_it_->second.access_time);
      return true;
    }

    return false;
  }

  // ReadMemLevel::IteratorImpl
  template<typename KeyType>
  ReadMemLevel<KeyType>::IteratorImpl::IteratorImpl(
    const ReadMemLevel<KeyType>* read_mem_level)
    noexcept
    : read_mem_level_(ReferenceCounting::add_ref(read_mem_level)),
      profiles_it_(read_mem_level_->profiles_.end())
  {}

  template<typename KeyType>
  bool
  ReadMemLevel<KeyType>::IteratorImpl::get_next(
    KeyType& key,
    ProfileOperation& operation,
    Generics::Time& access_time)
    noexcept
  {
    if(profiles_it_ == read_mem_level_->profiles_.end())
    {
      profiles_it_ = read_mem_level_->profiles_.begin();
    }
    else
    {
      ++profiles_it_;
    }

    if(profiles_it_ != read_mem_level_->profiles_.end())
    {
      key = profiles_it_->first;
      operation = static_cast<ProfileOperation>(profiles_it_->second.operation);
      access_time = Generics::Time(profiles_it_->second.access_time);
      return true;
    }

    return false;
  }

  template<typename KeyType>
  Generics::ConstSmartMemBuf_var
  ReadMemLevel<KeyType>::IteratorImpl::get_profile()
    noexcept
  {
    assert(profiles_it_ != read_mem_level_->profiles_.end());
    return profiles_it_->second.mem_buf;
  }

  template<typename KeyType>
  CheckProfileResult
  ReadMemLevel<KeyType>::check_profile(const KeyType& key)
    const /*throw(typename ReadBaseLevel<KeyType>::Exception)*/
  {
    return this->check_profile_i(key);
  }

  template<typename KeyType>
  GetProfileResult
  ReadMemLevel<KeyType>::get_profile(
    const KeyType& key)
    const /*throw(typename ReadBaseLevel<KeyType>::Exception)*/
  {
    return this->get_profile_i(key);
  }

  template<typename KeyType>
  typename ReadBaseLevel<KeyType>::KeyIterator_var
  ReadMemLevel<KeyType>::get_key_iterator() const
    noexcept
  {
    return new KeyIteratorImpl(this);
  }

  template<typename KeyType>
  typename ReadBaseLevel<KeyType>::Iterator_var
  ReadMemLevel<KeyType>::get_iterator(
    unsigned long /*read_buffer_size*/) const
    noexcept
  {
    return new IteratorImpl(this);
  }

  template<typename KeyType>
  unsigned long
  ReadMemLevel<KeyType>::size() const noexcept
  {
    return this->size_i();
  }

  template<typename KeyType>
  unsigned long
  ReadMemLevel<KeyType>::area_size() const noexcept
  {
    return this->area_size_i();
  }

  template<typename KeyType>
  unsigned long
  ReadMemLevel<KeyType>::merge_free_size() const noexcept
  {
    return this->merge_free_size_i();
  }

  template<typename KeyType>
  Generics::Time
  ReadMemLevel<KeyType>::min_access_time() const noexcept
  {
    return this->min_access_time_i();
  }
}
}
