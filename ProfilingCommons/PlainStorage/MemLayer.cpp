// @file PlainStorage/MemLayer.cpp
#include "MemLayer.hpp"

#include <iostream>
#include <map>

#include <Generics/ArrayAutoPtr.hpp>
#include <Generics/Time.hpp>

namespace PlainStorage
{
  /**
   * WriteBlockImpl
   */
  class WriteMemLayer::WriteBlockImpl:
    public virtual WriteBlock,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    WriteBlockImpl(
      const MemBlockIndex& block_index,
      unsigned long size)
      /*throw(BaseException)*/;

    virtual ~WriteBlockImpl() noexcept;

    /* ReadBlock interface */
    virtual SizeType size() /*throw(BaseException)*/;

    virtual SizeType
    read(void* buffer, SizeType size) /*throw(BaseException)*/;

    virtual void
    read(SizeType pos, void* buffer, SizeType size) /*throw(BaseException)*/;

    /* WriteBlock interface */
    virtual SizeType available_size() /*throw(BaseException)*/;

    virtual void
    write(const void* buffer, SizeType size)
      /*throw(BaseException, CorruptedRecord)*/;

    virtual void
    write(SizeType pos, const void* buffer, SizeType size)
      /*throw(BaseException, CorruptedRecord)*/;

    virtual void
    resize(SizeType size) /*throw(BaseException)*/;

    virtual void
    deallocate() /*throw(BaseException)*/;

  protected:
    /// The index of Data block
    MemBlockIndex block_index_;

    unsigned long block_size_;

    /// The pointer to begin of Data block
    void* content_;
  };

  /**
   * WriteMemLayer::WriteBlockImpl
   */
  WriteMemLayer::WriteBlockImpl::WriteBlockImpl(
    const MemBlockIndex& block_index,
    unsigned long block_size)
    /*throw(BaseException)*/
    : block_index_(block_index),
      block_size_(block_size)
  {
    content_ = ::malloc(block_size);
    *static_cast<u_int64_t*>(content_) = 0;
  }

  WriteMemLayer::WriteBlockImpl::~WriteBlockImpl() noexcept
  {
    ::free(content_);
  }

  SizeType
  WriteMemLayer::WriteBlockImpl::size() /*throw(BaseException)*/
  {
    return *static_cast<const u_int64_t*>(content_);
  }

  SizeType
  WriteMemLayer::WriteBlockImpl::read(
    void* buffer, SizeType buffer_size) /*throw(BaseException)*/
  {
    SizeType sz = size();

    if(sz > buffer_size)
    {
      return 0;
    }

    ::memcpy(buffer, (const u_int64_t*)content_ + 1, sz);

    return sz;
  }

  void
  WriteMemLayer::WriteBlockImpl::read(
    SizeType pos, void* buffer, SizeType read_size)
    /*throw(BaseException)*/
  {
    static const char* FUN = "WriteMemLayer::WriteBlockImpl::read()";

    if(pos + read_size > size())
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't read buffer with size = " <<
        read_size << " in pos = " << pos <<
        ". Full size of buffer is " << size();
      throw Exception(ostr);
    }

    ::memcpy(buffer,
      (char*)((const u_int64_t*)content_ + 1) + pos,
      read_size);
  }

  void
  WriteMemLayer::WriteBlockImpl::resize(SizeType new_size) /*throw(BaseException)*/
  {
    static const char* FUN = "WriteMemLayer::WriteBlockImpl::resize()";

    if(new_size > available_size())
    {
      Stream::Error ostr;
      ostr << FUN << ": requested size = " << new_size <<
        " is great then available size = " << available_size() << ".";
      throw Exception(ostr);
    }

    *(u_int64_t*)content_ = new_size;
  }

  SizeType
  WriteMemLayer::WriteBlockImpl::available_size() /*throw(BaseException)*/
  {
    return block_size_;
  }

  void
  WriteMemLayer::WriteBlockImpl::write(const void* buffer, SizeType size)
    /*throw(BaseException, CorruptedRecord)*/
  {
    static const char* FUN = "WriteMemLayer::WriteBlockImpl::write()";

    if (size > block_size_)
    {
      Stream::Error ostr;
      ostr << FUN << ": try write " << size <<
        " bytes to block with size=" << block_size_;
      throw CorruptedRecord(ostr);
    }

    *static_cast<u_int64_t*>(content_) = size;
    ::memcpy((u_int64_t*)content_ + 1, buffer, size);
  }

  void
  WriteMemLayer::WriteBlockImpl::write(
    SizeType pos, const void* buffer, SizeType write_size)
    /*throw(BaseException, CorruptedRecord)*/
  {
    static const char* FUN = "WriteMemLayer::WriteBlockImpl::write(pos, ...)";

    if (pos + write_size > block_size_)
    {
      Stream::Error ostr;
      ostr << FUN << ": Overflow while writing, position=" <<
        pos << ", write size=" << write_size <<
        ", block data size=" << block_size_;
      throw CorruptedRecord(ostr);
    }
    if(pos + write_size > size())
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't write buffer with size = " <<
        write_size << " in pos = " << pos <<
        ". Full size of buffer is " << size();
      throw Exception(ostr);
    }

    ::memcpy((char*)((u_int64_t*)content_ + 1) + pos, buffer, write_size);
  }

  void WriteMemLayer::WriteBlockImpl::deallocate() /*throw(BaseException)*/
  {
    throw Exception("Not Support");
  }

  /**
   * WriteMemLayer
   */
  WriteMemLayer::WriteMemLayer(unsigned long block_size)
    /*throw(Exception)*/
    : block_size_(block_size)
  {}

  WriteMemLayer::~WriteMemLayer() noexcept
  {}

  ReadBlock_var
  WriteMemLayer::get_read_block(const MemBlockIndex& block_index)
    /*throw(BaseException)*/
  {
    return get_write_block(block_index);
  }

  WriteBlock_var
  WriteMemLayer::get_write_block(const MemBlockIndex& block_index)
    /*throw(BaseException)*/
  {
    SyncPolicy::WriteGuard lock(blocks_lock_);
    BlockMap::const_iterator it = blocks_.find(block_index);
    if(it != blocks_.end())
    {
      return it->second;
    }

    return blocks_.insert(std::make_pair(
      block_index,
      WriteBlock_var(new WriteBlockImpl(block_index, block_size_)))).first->second;
  }

  bool
  WriteMemLayer::get_property(const char* name, PropertyValue& property)
    /*throw(BaseException)*/
  {
    SyncPolicy::ReadGuard lock(properties_lock_);
    PropertyMap::const_iterator it = properties_.find(name);
    if(it != properties_.end())
    {
      property = it->second;
      return true;
    }

    return false;
  }

  void
  WriteMemLayer::set_property(const char* name, const PropertyValue& property)
    /*throw(BaseException)*/
  {
    SyncPolicy::ReadGuard lock(properties_lock_);
    properties_[name] = property;
  }

  unsigned long
  WriteMemLayer::area_size() const noexcept
  {
    SyncPolicy::ReadGuard lock(blocks_lock_);
    return blocks_.size() * block_size_;
  }

  SizeType
  WriteMemLayer::block_data_size() const noexcept
  {
    return block_size_ - 2*sizeof(u_int32_t);
  }

  MemBlockIndex
  WriteMemLayer::max_block_index() const noexcept
  {
    SyncPolicy::ReadGuard lock(blocks_lock_);
    if(blocks_.empty())
    {
      return 0;
    }
    else
    {
      return blocks_.rbegin()->first + 1;
    }
  }
} // namespace PlainStorage
