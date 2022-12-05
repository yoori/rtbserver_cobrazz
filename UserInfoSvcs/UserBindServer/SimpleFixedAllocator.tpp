namespace AdServer
{
namespace UserInfoSvcs
{
  SimpleFixedAllocator::SimpleFixedAllocator(unsigned long alloc_size)
    noexcept
    : alloc_size_(alloc_size),
      buf_size_(((1024*1024 / alloc_size > 2) ? (1024*1024 / alloc_size) : 2) * alloc_size),
      last_element_offset_in_buf_(buf_size_ / alloc_size * alloc_size - alloc_size),
      first_free_(0)
  {}

  void*
  SimpleFixedAllocator::alloc()
    noexcept
  {
    {
      SyncPolicy::WriteGuard lock(lock_);
      if(first_free_)
      {
        void* ret = first_free_;
        first_free_ = *static_cast<void**>(first_free_);
        return ret;
      }
    }

    // first_free_ is null inside previous lock
    Buf new_buf;
    alloc_new_buf_(new_buf);
    BufList ins_list;
    ins_list.emplace_back(std::move(new_buf));

    SyncPolicy::WriteGuard lock(lock_);
    buffers_.splice(buffers_.end(), ins_list);
    Buf& back_buf = buffers_.back();
    *reinterpret_cast<void**>(&(back_buf[
      last_element_offset_in_buf_])) = first_free_;
    first_free_ = &(back_buf[alloc_size_]);
    return &(back_buf[0]);
  }

  void
  SimpleFixedAllocator::dealloc(void* buf)
    noexcept
  {
    SyncPolicy::WriteGuard lock(lock_);
    *static_cast<void**>(buf) = first_free_;
    first_free_ = buf;
  }

  void
  SimpleFixedAllocator::alloc_new_buf_(Buf& new_buf)
    noexcept
  {
    //std::cerr << "alloc_new_buf_" << std::endl;
    new_buf.resize(buf_size_);
    void* next = 0;
    for(long i = last_element_offset_in_buf_; i >= 0; i -= alloc_size_)
    {
      *reinterpret_cast<void**>(&new_buf[i]) = next;
      next = &new_buf[i];
    }
  }

  // SimpleDistribAllocator
  SimpleDistribAllocator::SimpleDistribAllocator(
    unsigned long min_alloc_size,
    unsigned long max_alloc_size) noexcept
    : min_alloc_size_(min_alloc_size),
      max_alloc_size_(max_alloc_size)
  {
    for(unsigned long i = min_alloc_size; i < max_alloc_size; ++i)
    {
      allocators_.emplace_back(new SimpleFixedAllocator(i));
    }
  }

  void*
  SimpleDistribAllocator::alloc(unsigned long size) noexcept
  {
    if(size < min_alloc_size_ || size >= max_alloc_size_)
    {
      return ::malloc(size);
    }

    return allocators_[size - min_alloc_size_]->alloc();
  }

  void
  SimpleDistribAllocator::dealloc(void* buf, unsigned long size) noexcept
  {
    if(size < min_alloc_size_ || size >= max_alloc_size_)
    {
      return ::free(buf);
    }

    allocators_[size - min_alloc_size_]->dealloc(buf);
  }
}
}
