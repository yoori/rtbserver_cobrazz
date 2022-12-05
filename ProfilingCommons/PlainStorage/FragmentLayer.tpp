// @file PlainStorage/FragmentLayer.tpp

#include <iostream>
#include <iterator>

#include <Generics/ArrayAutoPtr.hpp>
#include <Generics/BitAlgs.hpp>

#include "FragmentLayer.hpp"

namespace
{
  const uint32_t MAX_FRAGMENT_COUNT = 2113;
  const unsigned int MAX_FRAGMENT_SIZE_INDEX = 11;

  const char FREE_BLOCK_FLAG = 1;
  const char USED_BLOCK_FLAG = 0;
  /// Criteria that all fragments in this block are used
  const u_int32_t NO_FREE_FRAGMENT = 0xFFFFFFFF;

  /// Debug messages flag
  const bool PLST_TRACE_FRAGMENT_LAYER = false;
}

namespace PlainStorage
{
namespace
{
  template<typename NextIndexType, typename NextIndexSerializerType>
  struct FragmentBlockHead
  {
    /**
     * fragmented block head:
     *   next index size : u_int32_t
     *   next index : char[NextIndexSerializerType::max_size]
     *   fragment size : u_int32_t
     *   fragment count : u_int32_t
     *   first free sub index : u_int32_t (0xFFFFFFFF if not)
     * @return The size of fragmented block header
     */
    static SizeType
    size()
    {
      return sizeof(u_int32_t)*4 + NextIndexSerializerType::max_size();
    }

    /**
     * @return next index of block
     */
    static bool
    read_next_index(ReadBlock* block, NextIndexType& index)
    {
      u_int32_t index_size;
      NextIndexSerializerType index_serializer;
      block->read(0, &index_size, sizeof(u_int32_t));
      if(index_size > 0)
      {
        Generics::ArrayAutoPtr<char> buf(index_size);
        block->read(sizeof(u_int32_t), buf.get(), index_size);
        index_serializer.load(buf.get(), index_size, index);
        return true;
      }
      return false;
    }

    /**
     * Write next index val in block
     * @param block The block to update the index
     * @param val New next index value
     */
    static void
    write_next_index(WriteBlock* block, const NextIndexType& val)
    {
      NextIndexSerializerType index_serializer;
      u_int32_t index_size = index_serializer.size(val);
      block->write(0, &index_size, sizeof(u_int32_t));
      if(index_size > 0)
      {
        Generics::ArrayAutoPtr<char> buf(index_size);
        index_serializer.save(val, buf.get(), index_size);
        block->write(sizeof(u_int32_t), buf.get(), index_size);
      }
    }

    /**
     * Write null next index (end of blocks chain or unlink block from
     * chain of blocks)
     * @param block The block to be updated
     */
    static void
    write_null_next_index(WriteBlock* block)
    {
      u_int32_t index_size = 0;
      block->write(0, &index_size, sizeof(u_int32_t));
    }

    /**
     * @return Size of fragment in this fragmented block
     */
    static u_int32_t
    read_fragment_size(ReadBlock* block)
    {
      u_int32_t ret;
      block->read(
        sizeof(u_int32_t) + NextIndexSerializerType::max_size(),
        &ret,
        sizeof(u_int32_t));
      return ret;
    }

    /**
     * Save fragment size
     * @param block The block to update the size of fragment
     * @param val New value of fragment size
     */
    static void
    write_fragment_size(WriteBlock* block, u_int32_t val)
    {
      block->write(
        sizeof(u_int32_t) + NextIndexSerializerType::max_size(),
        &val,
        sizeof(u_int32_t));
    }

    /**
     * @return The number of fragments in the block
     */
    static u_int32_t
    read_fragment_count(ReadBlock* block)
    {
      u_int32_t ret;
      block->read(
        2*sizeof(u_int32_t) + NextIndexSerializerType::max_size(),
        &ret,
        sizeof(u_int32_t));
      return ret;
    }

    /**
     * Save number of fragments
     * @param block The block to update the size of fragment
     * @param val New value of fragment size
     */
    static void
    write_fragment_count(WriteBlock* block, u_int32_t val)
    {
      block->write(
        2*sizeof(u_int32_t) + NextIndexSerializerType::max_size(),
        &val,
        sizeof(u_int32_t));
    }

    /**
     * Read first free fragment index
     * @param block The block contain information
     * @return First free fragment subindex, NO_FREE_FRAGMENT=0xFFFFFFFF
     *   if it is not
     */
    static u_int32_t
    read_first_free_fragment(ReadBlock* block)
    {
      u_int32_t ret;
      block->read(
        3*sizeof(u_int32_t) + NextIndexSerializerType::max_size(),
        &ret,
        sizeof(u_int32_t));
      return ret;
    }

    /**
     * Write first free fragment index
     * @param block The block to be updated
     * @param val The index of first free fragment in the block
     */
    static void
    write_first_free_fragment(WriteBlock* block, u_int32_t val)
    {
      block->write(
        3*sizeof(u_int32_t) + NextIndexSerializerType::max_size(),
        &val,
        sizeof(u_int32_t));
    }
  }; // FragmentBlockHead struct
}
}

namespace PlainStorage
{
  /**
   * WriteFragmentLayer::ReadFragmentImpl class can function on a ordinary
   * data block and fragment of data from the block uniformly
   */
  template<
    typename NextIndexType,
    typename NextIndexSerializerType,
    typename SyncPolicyType>
  class WriteFragmentLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  ReadFragmentImpl :
    public virtual ReadBlock,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    /**
     * Save input parameters and calculate position of
     * fragment body, if the input parameters determine the fragment
     */
    ReadFragmentImpl(
      WriteFragmentLayer* allocator_layer,
      ReadBlock* write_block,
      const BlockExIndex<NextIndexType>& index)
      /*throw(BaseException)*/;

    /**
     * Empty virtual destructor
     */
    virtual ~ReadFragmentImpl() noexcept;

    /* ReadBlock interface */
    /**
     * Get user data size of block adjust fragmented or not
     * If block fragmented return used size value from fragment body
     */
    virtual SizeType size() /*throw(BaseException, CorruptedRecord)*/;

    /**
     * Read block content adjust fragmented or not
     */
    virtual SizeType
    read(void* buffer, SizeType size)
      /*throw(BaseException, CorruptedRecord)*/;

    virtual void
    read(SizeType pos, void* buffer, SizeType size)
      /*throw(BaseException, CorruptedRecord)*/;

    SizeType
    read_index_(void* buffer, SizeType buffer_size)
      /*throw(BaseException)*/;

  protected:
    BlockExIndex<NextIndexType> index_;
    ReadBlock_var read_block_;
    /// Locate fragment body, if block not fragmented = 0
    SizeType fragment_block_offset_;
  };

  /**
   * WriteFragmentLayer::WriteFragmentImpl can function on a ordinary
   * data block and fragment of data from the block uniformly
   * except deallocate call
   */
  template<
    typename NextIndexType,
    typename NextIndexSerializerType,
    typename SyncPolicyType>
  class WriteFragmentLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  WriteFragmentImpl:
    public virtual WriteBlock,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    /**
     * Save input parameters and calculate position of
     * fragment body, if the input parameters determine the fragment
     */
    WriteFragmentImpl(
      WriteFragmentLayer* allocator_layer,
      WriteBlock* write_block,
      const BlockExIndex<NextIndexType>& index)
      /*throw(BaseException)*/;

    /* ReadBlock interface */
    virtual SizeType size() /*throw(BaseException, CorruptedRecord)*/;

    virtual SizeType
    read(void* buffer, SizeType size)
      /*throw(BaseException, CorruptedRecord)*/;

    virtual void
    read(SizeType pos, void* buffer, SizeType size)
      /*throw(BaseException, CorruptedRecord)*/;

    /* WriteBlock interface */
    /**
     * @return The size of allocated memory in fragment or block
     * if block not fragmented. The presence of user data ignored
     */
    virtual SizeType available_size() /*throw(BaseException)*/;

    virtual void
    write(const void* buffer, SizeType size)
      /*throw(BaseException, CorruptedRecord)*/;

    virtual void
    write(SizeType pos, const void* buffer, SizeType size)
      /*throw(BaseException, CorruptedRecord)*/;

    /**
     * Check available size in fragment (or block) and change
     * used size of fragment or block
     * @param size The new size value for used space by user data.
     */
    virtual void
    resize(SizeType size) /*throw(BaseException, CorruptedRecord)*/;

    virtual void
    deallocate() /*throw(BaseException)*/;

    SizeType
    read_index_(void* buffer, SizeType buffer_size)
      /*throw(BaseException)*/;

    std::ostream&
    print_(std::ostream& ostr,
      const char* offset = "")
      const noexcept;

  protected:
    /**
     * Empty virtual destructor
     */
    virtual ~WriteFragmentImpl() noexcept;

  protected:
    BlockExIndex<NextIndexType> index_;
    /// BlockSizeAllocator implemented in FragmentLayer class,
    /// allocator that manage fragments
    WriteFragmentLayer* allocator_layer_;
    /// Reference to a data block that can be fragmented and if fragmented
    /// contains the indexed fragment
    WriteBlock_var write_block_;
    /// Locate fragment body, if block not fragmented = 0
    SizeType fragment_block_offset_;
  };

  //
  // WriteFragmentLayer<>::AllocatePool
  //
  template<
    typename NextIndexType,
    typename NextIndexSerializerType,
    typename SyncPolicyType>
  WriteFragmentLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  AllocatePool::AllocatePool(WriteBaseLayer<NextBlockIndex>* next_layer)
    noexcept
    : next_layer_(ReferenceCounting::add_ref(next_layer))
  {}

  template<
    typename NextIndexType,
    typename NextIndexSerializerType,
    typename SyncPolicyType>
  typename WriteFragmentLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::ControlBlock
  WriteFragmentLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  AllocatePool::get()
    /*throw(BaseException)*/
  {
    ControlBlockList control_block_holder;

    {
      typename ControlBlockSyncPolicy::WriteGuard lock(
        control_blocks_lock_);

      if(!control_blocks_.empty())
      {
        control_block_holder.splice(
          control_block_holder.begin(),
          control_blocks_,
          control_blocks_.begin());
      }
    }

    if(!control_block_holder.empty())
    {
      return *control_block_holder.begin();
    }

    typename NextControlBlockSyncPolicy::WriteGuard lock(
      next_control_block_lock_);

    if(next_control_block_.block)
    {
      // read next free
      typedef FragmentBlockHead<NextIndexType, NextIndexSerializerType> ActualFBH;

      ControlBlock res_control_block = next_control_block_;
      ControlBlock new_next_control_block;

      bool defined_next_block = ActualFBH::read_next_index(
        next_control_block_.block, new_next_control_block.index);

      if(defined_next_block)
      {
        new_next_control_block.block = next_layer_->get_write_block(
          new_next_control_block.index);
      }

      next_control_block_ = new_next_control_block;

      return res_control_block;
    }

    return ControlBlock();
  }

  template<
    typename NextIndexType,
    typename NextIndexSerializerType,
    typename SyncPolicyType>
  void
  WriteFragmentLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  AllocatePool::release(const ControlBlock& control_block)
    noexcept
  {
    ControlBlockList control_block_holder;
    control_block_holder.push_back(control_block);

    {
      typename ControlBlockSyncPolicy::WriteGuard control_blocks_lock(
        control_blocks_lock_);

      control_blocks_.splice(
        control_blocks_.begin(),
        control_block_holder);
    }
  }

  template<
    typename NextIndexType,
    typename NextIndexSerializerType,
    typename SyncPolicyType>
  void
  WriteFragmentLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  AllocatePool::add_free(const NextIndexType& index, WriteBlock* write_block)
    /*throw(BaseException)*/
  {
    typedef FragmentBlockHead<NextIndexType, NextIndexSerializerType> ActualFBH;

    NextControlBlockSyncPolicy::WriteGuard lock(next_control_block_lock_);
    ControlBlock new_control_block(index, write_block);

    if(next_control_block_.block)
    {
      ActualFBH::write_next_index(
        write_block,
        next_control_block_.index);
    }

    next_control_block_ = new_control_block;
  }

  //
  // WriteFragmentLayer<>::ReadFragmentImpl
  //
  template<
    typename NextIndexType,
    typename NextIndexSerializerType,
    typename SyncPolicyType>
  WriteFragmentLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  ReadFragmentImpl::ReadFragmentImpl(
    WriteFragmentLayer* /*allocator_layer*/,
    ReadBlock* control_block,
    const BlockExIndex<NextIndexType>& index)
    /*throw(BaseException)*/
    : index_(index),
      read_block_(ReferenceCounting::add_ref(control_block))
  {
    typedef FragmentBlockHead<NextIndexType, NextIndexSerializerType> ActualFBH;

    if(index_.has_sub_index())
    {
      fragment_block_offset_ =
        ActualFBH::size() +
        ActualFBH::read_fragment_count(control_block) +
        ActualFBH::read_fragment_size(control_block)*index.sub_index;
    }
    else
    {
      fragment_block_offset_ = 0;
    }
  }

  template<
    typename NextIndexType,
    typename NextIndexSerializerType,
    typename SyncPolicyType>
  WriteFragmentLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  ReadFragmentImpl::~ReadFragmentImpl() noexcept
  {}

  template<
    typename NextIndexType,
    typename NextIndexSerializerType,
    typename SyncPolicyType>
  SizeType
  WriteFragmentLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  ReadFragmentImpl::read(
    void* buffer, SizeType buf_size) /*throw(BaseException, CorruptedRecord)*/
  {
    SizeType sz = size();
    if(buf_size < sz)
    {
      return 0;
    }

    if(index_.has_sub_index())
    {
      read_block_->read(fragment_block_offset_ + sizeof(u_int32_t), buffer, sz);
    }
    else
    {
      sz = read_block_->read(buffer, sz);
    }

    return sz;
  }

  template<
    typename NextIndexType,
    typename NextIndexSerializerType,
    typename SyncPolicyType>
  SizeType
  WriteFragmentLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  ReadFragmentImpl::read_index_(
    void* buffer, SizeType buf_size) /*throw(BaseException)*/
  {
    IndexSerializer index_serializer;
    u_int32_t index_size = index_serializer.size(index_);
    if(index_size > buf_size)
    {
      return 0;
    }
    index_serializer.save(index_, buffer, index_size);
    return index_size;
  }

  template<
    typename NextIndexType,
    typename NextIndexSerializerType,
    typename SyncPolicyType>
  void
  WriteFragmentLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  ReadFragmentImpl::read(
    SizeType pos, void* buffer, SizeType buf_size)
    /*throw(BaseException, CorruptedRecord)*/
  {
    if(pos + buf_size > size())
    {
      Stream::Error ostr;
      ostr << "WriteFragmentLayer::ReadFragmentImpl::read(pos, ...): "
        "Can't read size=" << buf_size <<
        " from pos=" << pos <<
        " from block with full size=" << size() <<
        ", fragment offset=" << fragment_block_offset_;
      throw BaseException(ostr);
    }

    if(index_.has_sub_index())
    {
      read_block_->read(
        fragment_block_offset_ + sizeof(u_int32_t) + pos, buffer, buf_size);
    }
    else
    {
      read_block_->read(pos, buffer, buf_size);
    }
  }

  //
  // WriteFragmentLayer<>::WriteFragmentImpl
  //
  template<
    typename NextIndexType,
    typename NextIndexSerializerType,
    typename SyncPolicyType>
  WriteFragmentLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  WriteFragmentImpl::WriteFragmentImpl(
    WriteFragmentLayer* allocator_layer,
    WriteBlock* control_block,
    const BlockExIndex<NextIndexType>& index)
    /*throw(BaseException)*/
    : index_(index),
      allocator_layer_(allocator_layer),
      write_block_(ReferenceCounting::add_ref(control_block))
  {
    typedef FragmentBlockHead<NextIndexType, NextIndexSerializerType> ActualFBH;

    if(index_.has_sub_index())
    {
      fragment_block_offset_ =
        ActualFBH::size() +
        ActualFBH::read_fragment_count(control_block) +
        ActualFBH::read_fragment_size(control_block)*index.sub_index;
    }
    else
    {
      fragment_block_offset_ = 0;
    }
  }

  template<
    typename NextIndexType,
    typename NextIndexSerializerType,
    typename SyncPolicyType>
  WriteFragmentLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  WriteFragmentImpl::~WriteFragmentImpl()
    noexcept
  {}

  template<
    typename NextIndexType,
    typename NextIndexSerializerType,
    typename SyncPolicyType>
  SizeType
  WriteFragmentLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  ReadFragmentImpl::size() /*throw(BaseException, CorruptedRecord)*/
  {
    if(index_.has_sub_index())
    {
      u_int32_t sub_size;
      read_block_->read(fragment_block_offset_, &sub_size, sizeof(u_int32_t));
      return sub_size;
    }

    return read_block_->size();
  }

  template<
    typename NextIndexType,
    typename NextIndexSerializerType,
    typename SyncPolicyType>
  SizeType
  WriteFragmentLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  WriteFragmentImpl::size() /*throw(BaseException, CorruptedRecord)*/
  {
    if(index_.has_sub_index())
    {
      u_int32_t sub_size;
      write_block_->read(fragment_block_offset_, &sub_size, sizeof(u_int32_t));
      return sub_size;
    }

    return write_block_->size();
  }

  template<
    typename NextIndexType,
    typename NextIndexSerializerType,
    typename SyncPolicyType>
  SizeType
  WriteFragmentLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  WriteFragmentImpl::read(
    void* buffer, SizeType buf_size) /*throw(BaseException, CorruptedRecord)*/
  {
    SizeType sz = size();
    if(buf_size < sz)
    {
      return 0;
    }

    if(index_.has_sub_index())
    {
      if(buf_size < size())
      {
        Stream::Error ostr;
        ostr << "WriteFragmentLayer::WriteFragmentImpl::read(...): "
          "Can't into buffer with size=" << buf_size <<
          " from block with full size=" << size() <<
          ", fragment offset=" << fragment_block_offset_;
        throw BaseException(ostr);
      }

      write_block_->read(fragment_block_offset_ + sizeof(u_int32_t), buffer, sz);
    }
    else
    {
      sz = write_block_->read(buffer, sz);
    }

    return sz;
  }

  template<
    typename NextIndexType,
    typename NextIndexSerializerType,
    typename SyncPolicyType>
  void
  WriteFragmentLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  WriteFragmentImpl::read(
    SizeType pos, void* buffer, SizeType buf_size)
    /*throw(BaseException, CorruptedRecord)*/
  {
    if(pos + buf_size > size())
    {
      Stream::Error ostr;
      ostr << "WriteFragmentLayer::WriteFragmentImpl::read(pos, ...): "
        "Can't read size=" << buf_size <<
        " from pos=" << pos <<
        " from block with full size=" << size() <<
        ", fragment offset=" << fragment_block_offset_;
      throw BaseException(ostr);
    }

    if(index_.has_sub_index())
    {
      write_block_->read(
        fragment_block_offset_ + sizeof(u_int32_t) + pos, buffer, buf_size);
    }
    else
    {
      write_block_->read(pos, buffer, buf_size);
    }
  }

  template<
    typename NextIndexType,
    typename NextIndexSerializerType,
    typename SyncPolicyType>
  SizeType
  WriteFragmentLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  WriteFragmentImpl::available_size()
    /*throw(BaseException)*/
  {
    typedef FragmentBlockHead<NextIndexType, NextIndexSerializerType> ActualFBH;

    if(index_.has_sub_index())
    {
      return ActualFBH::read_fragment_size(write_block_) - sizeof(u_int32_t);
    }

    return write_block_->available_size();
  }

  template<
    typename NextIndexType,
    typename NextIndexSerializerType,
    typename SyncPolicyType>
  void
  WriteFragmentLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  WriteFragmentImpl::write(
    const void* buffer, SizeType buf_size)
    /*throw(BaseException, CorruptedRecord)*/
  {
    if(buf_size > size())
    {
      Stream::Error ostr;
      ostr << "WriteFragmentLayer<>::WriteFragmentImpl::write(): "
        "Can't write buffer with size=" << buf_size <<
        " great then size=" << size() << ".";
      throw BaseException(ostr);
    }

    u_int32_t sz = buf_size;

    if(index_.has_sub_index())
    {
      write_block_->write(fragment_block_offset_, &sz, sizeof(u_int32_t));
      write_block_->write(
        fragment_block_offset_ + sizeof(u_int32_t), buffer, buf_size);
    }
    else
    {
      write_block_->write(buffer, buf_size);
    }
  }

  template<
    typename NextIndexType,
    typename NextIndexSerializerType,
    typename SyncPolicyType>
  void
  WriteFragmentLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  WriteFragmentImpl::write(
    SizeType pos, const void* buffer, SizeType buf_size)
    /*throw(BaseException, CorruptedRecord)*/
  {
    static const char* FUN = "WriteFragmentLayer::WriteFragmentImpl::write()";

    if(pos + buf_size > size())
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't write to offset=" << pos <<
        " buffer with size=" << buf_size <<
        ", it intersect right bound=" << size() << ".";
      throw BaseException(ostr);
    }

    if(index_.has_sub_index())
    {
      write_block_->write(
        fragment_block_offset_ + sizeof(u_int32_t) + pos, buffer, buf_size);
    }
    else
    {
      write_block_->write(pos, buffer, buf_size);
    }
  }

  template<
    typename NextIndexType,
    typename NextIndexSerializerType,
    typename SyncPolicyType>
  void
  WriteFragmentLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  WriteFragmentImpl::resize(SizeType new_size)
    /*throw(BaseException, CorruptedRecord)*/
  {
    if(new_size > available_size())
    {
      Stream::Error ostr;
      ostr << "WriteFragmentImpl::resize(): Can't resize fragment, new size=" <<
        new_size << " great then available size=" << available_size() << ".";
      throw BaseException(ostr);
    }

    if(index_.has_sub_index())
    {
      u_int32_t sz = new_size;
      write_block_->write(fragment_block_offset_, &sz, sizeof(u_int32_t));
    }
    else
    {
      write_block_->resize(new_size);
    }
  }

  template<
    typename NextIndexType,
    typename NextIndexSerializerType,
    typename SyncPolicyType>
  void
  WriteFragmentLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  WriteFragmentImpl::deallocate()
    /*throw(BaseException)*/
  {
    allocator_layer_->deallocate_(write_block_, index_);
  }

  template<
    typename NextIndexType,
    typename NextIndexSerializerType,
    typename SyncPolicyType>
  SizeType
  WriteFragmentLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  WriteFragmentImpl::read_index_(
    void* buffer, SizeType buf_size) /*throw(BaseException)*/
  {
    IndexSerializer index_serializer;
    u_int32_t index_size = index_serializer.size(index_);
    if(index_size > buf_size)
    {
      return 0;
    }
    index_serializer.save(index_, buffer, index_size);
    return index_size;
  }

  template<
    typename NextIndexType,
    typename NextIndexSerializerType,
    typename SyncPolicyType>
  std::ostream&
  WriteFragmentLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  WriteFragmentImpl::print_(
    std::ostream& ostr,
    const char* offset)
    const noexcept
  {
    typedef FragmentBlockHead<NextIndexType, NextIndexSerializerType> ActualFBH;

    ostr << offset << "#(" << index_.base_index << ", ";
    if(index_.sub_index == SUB_INDEX_RESERVED)
    {
      ostr << "none";
    }
    else
    {
      u_int32_t first_free_fragment =
        ActualFBH::read_first_free_fragment(write_block_);

      ostr << index_.sub_index << "): " <<
       "fragment_block_offset = " << fragment_block_offset_ <<
       ", first free fragment = " << first_free_fragment <<
       ", mask =";

      char fragment_flag_buf[MAX_FRAGMENT_COUNT];
      SizeType fragment_count = ActualFBH::read_fragment_count(write_block_);
      write_block_->read(ActualFBH::size(), fragment_flag_buf, fragment_count);

      for(typename BlockIndex::SubIndex i = 0; i < fragment_count; ++i)
      {
        ostr << " " << i << "-" <<
          (fragment_flag_buf[i] == FREE_BLOCK_FLAG ? "free" : "used");
      }
    }
    ostr << std::endl;
    return ostr;
  }

  //
  // WriteFragmentLayer
  //
  template<
    typename NextIndexType,
    typename NextIndexSerializerType,
    typename SyncPolicyType>
  WriteFragmentLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  WriteFragmentLayer(
    WriteBaseLayer<NextIndexType>* next_layer,
    BlockAllocator<NextIndexType>* allocator)
    /*throw(BaseException)*/
    : TransientPropertyReadLayerImpl<
        BlockExIndex<NextIndexType>, NextIndexType>(next_layer),
      TransientPropertyWriteLayerImpl<
        BlockExIndex<NextIndexType>, NextIndexType>(next_layer),
      next_layer_(ReferenceCounting::add_ref(next_layer)),
      allocator_(ReferenceCounting::add_ref(allocator))
  {
    static const char* FUN = "WriteFragmentLayer<>::WriteFragmentLayer()";
    static const char* MAIN_CONTROL_BLOCK_PROP = "WriteFragmentLayer::MainControlBlock";
    static const char* ALLOCATOR_PROP_NAME = "RecordAllocatorType2";
    static const char* ALLOCATOR_NAME = "Default";

    PropertyValue allocator_name;

    if(!next_layer_->get_property(ALLOCATOR_PROP_NAME, allocator_name))
    {
      allocator_name.value(ALLOCATOR_NAME, sizeof(ALLOCATOR_NAME));

      /* init allocator for new file */
      next_layer_->set_property(ALLOCATOR_PROP_NAME, allocator_name);

      typename BlockAllocator<NextIndexType>::AllocatedBlock alloc_mcb;
      allocator_->allocate(alloc_mcb);

      PropertyValue mcb_index_val;
      SizeType ind_size = NextIndexSerializer::size(alloc_mcb.block_index);
      Generics::ArrayAutoPtr<char> buf(ind_size);
      next_index_serializer_.save(alloc_mcb.block_index, buf.get(), ind_size);
      mcb_index_val.value(buf.get(), ind_size);
      next_layer_->set_property(MAIN_CONTROL_BLOCK_PROP, mcb_index_val);

      main_control_block_.block = alloc_mcb.block;
      main_control_block_.index = alloc_mcb.block_index;
    }
    else
    {
      if(::strcmp((const char*)allocator_name.value(), ALLOCATOR_NAME) == 0)
      {
        /* init impl from properties - main control block */
        PropertyValue mcb_index_val;

        if(next_layer_->get_property(MAIN_CONTROL_BLOCK_PROP, mcb_index_val))
        {
          NextBlockIndex mcb_index;
          next_index_serializer_.load(
            mcb_index_val.value(), mcb_index_val.size(), mcb_index);
          main_control_block_.block = next_layer_->get_write_block(mcb_index);
          main_control_block_.index = mcb_index;
        }
        else
        {
          Stream::Error ostr;
          ostr << FUN << ": '" << MAIN_CONTROL_BLOCK_PROP <<
            "' property didn't init.";
          throw BaseException(ostr);
        }
      }
      else
      {
        Stream::Error ostr;
        ostr << FUN <<
          ": File already use other record allocator with name '" <<
          (const char*)allocator_name.value() <<
          "'.";
        throw BaseException(ostr);
      }
    }

    allocate_pools_.resize(MAX_FRAGMENT_SIZE_INDEX + 1);
    for(typename AllocatePoolArray::iterator pool_it =
          allocate_pools_.begin();
        pool_it != allocate_pools_.end(); ++pool_it)
    {
      *pool_it = new AllocatePool(next_layer_);
    }

    // read main control block
    const unsigned long rec_key_size =
      sizeof(u_int64_t) + NextIndexSerializerType::max_size();
    const unsigned long rec_size = sizeof(u_int64_t) + rec_key_size;

    SizeType buf_size = main_control_block_.block->size();
    if(buf_size)
    {
      Generics::ArrayAutoPtr<unsigned char> buf(buf_size);
      main_control_block_.block->read(buf.get(), buf_size);

      for(unsigned char* buf_pos = buf.get();
          buf_pos < buf.get() + buf_size;
          buf_pos += rec_size)
      {
        SizeType fragment_size = *reinterpret_cast<u_int64_t*>(buf_pos);
        SizeType index_size = *(reinterpret_cast<u_int64_t*>(buf_pos) + 1);
        NextIndexType index;
        next_index_serializer_.load(
          buf_pos + 2 * sizeof(u_int64_t),
          index_size,
          index);

        SizeType safe_block_size = max_block_size();
        SizeType new_fragment_size;
        unsigned int fragment_size_index = eval_fragment_size_(
          new_fragment_size,
          fragment_size,
          safe_block_size);

        WriteBlock_var write_block = next_layer_->get_write_block(index);
        allocate_pools_[fragment_size_index]->add_free(index, write_block);
      }
    }

    // clear control block - it will be filled only after correct destruct
    main_control_block_.block->resize(0);
  }

  template<
    typename NextIndexType,
    typename NextIndexSerializerType,
    typename SyncPolicyType>
  WriteFragmentLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  ~WriteFragmentLayer()
    noexcept
  {
    try
    {
      sync_control_block_();
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << "WriteFragmentImpl::~WriteFragmentImpl(): caught eh::Exception: " <<
        ex.what();
      std::cerr << ostr.str() << std::endl;
    }
  }

  template<
    typename NextIndexType,
    typename NextIndexSerializerType,
    typename SyncPolicyType>
  ReadBlock_var
  WriteFragmentLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  get_read_block(
    const BlockExIndex<NextIndexType>& index)
    /*throw(BaseException, CorruptedRecord)*/
  {
    return new ReadFragmentImpl(
      this, next_layer_->get_read_block(index.base_index), index);
  }

  template<
    typename NextIndexType,
    typename NextIndexSerializerType,
    typename SyncPolicyType>
  WriteBlock_var
  WriteFragmentLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  get_write_block(
    const BlockExIndex<NextIndexType>& index)
    /*throw(BaseException, CorruptedRecord)*/
  {
    return new WriteFragmentImpl(
      this, next_layer_->get_write_block(index.base_index), index);
  }

  template<
    typename NextIndexType,
    typename NextIndexSerializerType,
    typename SyncPolicyType>
  unsigned long
  WriteFragmentLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  area_size() const noexcept
  {
    return next_layer_->area_size();
  }

  template<
    typename NextIndexType,
    typename NextIndexSerializerType,
    typename SyncPolicyType>
  SizeType
  WriteFragmentLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  max_block_size() const noexcept
  {
    return main_control_block_.block->available_size();
  }

  template<
    typename NextIndexType,
    typename NextIndexSerializerType,
    typename SyncPolicyType>
  void
  WriteFragmentLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  allocate_size(
    SizeType size, AllocatedBlock& allocated_block) /*throw(BaseException)*/
  {
    static const char* FUN = "WriteFragmentLayer<>::allocate_size()";

    // available memory in ordinary data block, example MCB
    SizeType safe_block_size = max_block_size();

    if (size > safe_block_size)
    {
      Stream::Error ostr;
      ostr << FUN << ": Requested fragment size more "
        "than size of ordinary data block";
      throw BaseException(ostr);
    }

    try
    {
      if(safe_block_size <= 2*size)
        // if requested size more than half available memory in
        // ordinary block, we must allocate not fragmented blocks
      {
        typename BlockAllocator<NextBlockIndex>::AllocatedBlock a_block;
        allocator_->allocate(a_block);
        a_block.block->resize(size);
        allocated_block.block_index =
          BlockExIndex<NextIndexType>(a_block.block_index);
        allocated_block.block = a_block.block;
      }
      else
      { // possible to allocate only part of block - the fragment
        SizeType fragment_size;
        unsigned int fragment_size_index = eval_fragment_size_(
          fragment_size,
          size + sizeof(u_int32_t),
          safe_block_size);
        
        {
          SizeType check_fragment_size;
          assert(
            eval_fragment_size_(check_fragment_size, size + sizeof(u_int32_t), safe_block_size) ==
              fragment_size_index &&
            fragment_size == check_fragment_size);
        }

        AllocatedBlock block_with_rest;
        allocate_fragment_(
          fragment_size_index,
          fragment_size,
          block_with_rest);

        allocated_block.block_index = block_with_rest.block_index;
        allocated_block.block =
          new WriteFragmentImpl(
            this, block_with_rest.block, block_with_rest.block_index);
      }

      allocated_block.block->resize(size);

      if(PLST_TRACE_FRAGMENT_LAYER)
      {
        Stream::Error trace_ostr;
        trace_ostr << FUN << ": allocated block (#" <<
          allocated_block.block_index << ") requested size = " << size <<
          ", size = " << allocated_block.block->size() <<
          ", available_size = " << allocated_block.block->available_size();
        std::cout << trace_ostr.str() << std::endl;
      }
    }
    catch (const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw BaseException(ostr);
    }
  }

  template<
    typename NextIndexType,
    typename NextIndexSerializerType,
    typename SyncPolicyType>
  unsigned int
  WriteFragmentLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  eval_fragment_size_(
    SizeType& fragment_size,
    SizeType size_rest,
    SizeType safe_block_size)
    /*throw(BaseException)*/
  {
    typedef FragmentBlockHead<NextIndexType, NextIndexSerializerType> ActualFBH;
    static const char* FUN = "WriteFragmentLayer::eval_fragment_size_()";

    if(safe_block_size < size_rest || size_rest == 0)
    {
      Stream::Error ostr;
      ostr << FUN << ": rest size great then safe block size (" <<
        size_rest << " > " << safe_block_size << ").";
      throw BaseException(ostr);
    }

    unsigned int highest_bit = Generics::BitAlgs::highest_bit_32(
      (safe_block_size - ActualFBH::size()) / (size_rest + 1));

    highest_bit = std::min(highest_bit, MAX_FRAGMENT_SIZE_INDEX);

    SizeType divider = (static_cast<SizeType>(1) << highest_bit);
    fragment_size = (safe_block_size - ActualFBH::size()) / divider - 1;

    assert(fragment_size >= size_rest);

    return highest_bit;
  }

  template<
    typename NextIndexType,
    typename NextIndexSerializerType,
    typename SyncPolicyType>
  void
  WriteFragmentLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  allocate_fragment_(
    unsigned int fragment_size_index,
    SizeType fragment_size,
    AllocatedBlock& result_block)
    /*throw(BaseException)*/
  {
    static const char* FUN = "WriteFragmentLayer::allocate_fragment_()";

    try
    {
      AllocatePool& allocate_pool = *allocate_pools_[fragment_size_index];

      ControlBlock control_block = allocate_pool.get();

      if(!control_block.block)
      {
        control_block = alloc_control_block_i_(fragment_size);
      }

      /* allocate_fragment */
      bool last_fragment;
      NextIndexType fragment_base_index = control_block.index;
      typename BlockIndex::SubIndex fragment_index =
        allocate_fragment_i_(control_block, last_fragment);

      WriteBlock_var res_block = control_block.block;

      if(!last_fragment)
      {
        // return ControlBlock to pool
        allocate_pool.release(control_block);
      }

      result_block.block_index = BlockIndex(fragment_base_index, fragment_index);
      result_block.block = res_block;

#     ifdef DEBUG_ALLOC_CONFLICT_CHECK_

      {
        typename SyncPolicyType::WriteGuard lock(debug_alloc_control_lock_);
        if(!debug_allocated_indexes_.insert(result_block.block_index).second)
        {
          raise(SIGSEGV);
        }
      }
      
#     endif
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't allocate fragment with size index = " <<
        fragment_size_index << ". Caught eh::Exception: " <<
        ex.what();
      throw BaseException(ostr);
    }
  }

  template<
    typename NextIndexType,
    typename NextIndexSerializerType,
    typename SyncPolicyType>
  void
  WriteFragmentLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  deallocate_(
    WriteBlock* write_block, const BlockIndex& index)
    /*throw(BaseException)*/
  {
    static const char* FUN = "WriteFragmentLayer<>::deallocate_()";

    if(PLST_TRACE_FRAGMENT_LAYER)
    {
      Stream::Error trace_ostr;
      trace_ostr << FUN << ":> deallocate block (#" <<
        index << ")";
      std::cout << trace_ostr.str() << std::endl;
    }

    if(index.has_sub_index())
    {
      deallocate_fragment_(write_block, index);
    }
    else
    {
      write_block->deallocate();
    }

    if(PLST_TRACE_FRAGMENT_LAYER)
    {
      Stream::Error trace_ostr;
      trace_ostr << FUN << ":< deallocate block (#" <<
        index << ")";
      std::cout << trace_ostr.str() << std::endl;
    }
  }

  template<
    typename NextIndexType,
    typename NextIndexSerializerType,
    typename SyncPolicyType>
  typename WriteFragmentLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  ControlBlock
  WriteFragmentLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  alloc_control_block_i_(SizeType fragment_size)
    /*throw(BaseException)*/
  {
    typedef FragmentBlockHead<NextIndexType, NextIndexSerializerType> ActualFBH;

    /* create block with requested fragment size */
    typename BlockAllocator<NextIndexType>::AllocatedBlock allocated_block;
    allocator_->allocate(allocated_block);

    init_fragmentation_i_(allocated_block.block_index, allocated_block.block, fragment_size);

    ControlBlock alloc_control_block(
      allocated_block.block_index, allocated_block.block);

    ActualFBH::write_null_next_index(allocated_block.block);

    return alloc_control_block;
  }

  template<
    typename NextIndexType,
    typename NextIndexSerializerType,
    typename SyncPolicyType>
  void
  WriteFragmentLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  sync_control_block_()
    /*throw(BaseException)*/
  {
    //static const char* FUN = "WriteFragmentLayer::sync_control_block_()";

    typedef FragmentBlockHead<NextIndexType, NextIndexSerializerType> ActualFBH;

    /* update main control block :
     * save pair (size, index) to end of block
     * main control block can be empty (size = 0) */
    const unsigned long rec_key_size =
      sizeof(u_int64_t) + NextIndexSerializerType::max_size();
    const unsigned long rec_size = sizeof(u_int64_t) + rec_key_size;

    Generics::ArrayAutoPtr<unsigned char> buf(rec_size * allocate_pools_.size());
    unsigned char* buf_pos = buf.get();

    for(typename AllocatePoolArray::const_iterator pool_it =
          allocate_pools_.begin();
        pool_it != allocate_pools_.end(); ++pool_it)
    {
      const AllocatePool& pool = *(*pool_it);
      ControlBlock head_block = pool.next_control_block_i();
      const ControlBlockList& control_blocks = pool.control_blocks_i();
      for(typename ControlBlockList::const_iterator cb_it =
            control_blocks.begin();
          cb_it != control_blocks.end(); ++cb_it)
      {
        if(head_block.block)
        {
          ActualFBH::write_next_index(cb_it->block, head_block.index);
        }
        else
        {
          ActualFBH::write_null_next_index(cb_it->block);
        }

        head_block = *cb_it;
      }

      if(head_block.block)
      {
        // save head into control block (fragment size -> next index)
        u_int64_t index_size = next_index_serializer_.size(head_block.index);
        u_int32_t fragment_size = ActualFBH::read_fragment_size(head_block.block);
        *reinterpret_cast<u_int64_t*>(buf_pos) = fragment_size;
        *(reinterpret_cast<u_int64_t*>(buf_pos) + 1) = index_size;
        next_index_serializer_.save(
          head_block.index,
          buf_pos + 2*sizeof(u_int64_t),
          index_size);
        buf_pos += rec_size;
      }
    }

    unsigned int buf_size = buf_pos - buf.get();
    if(buf_size > 0)
    {
      main_control_block_.block->write(buf.get(), buf_size);
    }
    else
    {
      main_control_block_.block->resize(0);
    }
  }

  template<
    typename NextIndexType,
    typename NextIndexSerializerType,
    typename SyncPolicyType>
  void
  WriteFragmentLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  deallocate_fragment_(
    WriteBlock* write_block,
    const BlockIndex& index)
    /*throw (BaseException, CorruptedRecord)*/
  {
    static const char* FUN = "WriteFragmentLayer::deallocate_fragment_()";

    typedef FragmentBlockHead<NextIndexType, NextIndexSerializerType> ActualFBH;

    u_int32_t fragment_size = ActualFBH::read_fragment_size(write_block);
    u_int32_t fragment_count = ActualFBH::read_fragment_count(write_block);

    if (index.sub_index >= fragment_count)
    {
      Stream::Error ostr;
      ostr << FUN << ": sub index " <<
        index.sub_index <<
        " points to outside fragments count " << fragment_count;
      throw CorruptedRecord(ostr);
    }

    if((fragment_size + 1)*fragment_count + ActualFBH::size() !=
       write_block->size())
    {
      Stream::Error ostr;
      ostr << FUN << ": Non correct control block record:"
          " fragment_size = " << fragment_size <<
        " fragment_count = " << fragment_count <<
        " and block size = " <<
        write_block->size() <<
        ".";
      throw BaseException(ostr);
    }

    SizeType safe_block_size = max_block_size();
    SizeType unused_fragment_size;

    unsigned int fragment_size_index = eval_fragment_size_(
      unused_fragment_size,
      fragment_size,
      safe_block_size);

    SizeType flag_offset = ActualFBH::size() + index.sub_index;

    bool use_as_control = false;
    
    {
      typename FragmentAllocateLockMap::WriteGuard block_lock(
        fragment_alloc_locks_.write_lock(index.base_index));

      write_block->write(flag_offset, &FREE_BLOCK_FLAG, sizeof(FREE_BLOCK_FLAG));

      u_int32_t first_free_fragment =
        ActualFBH::read_first_free_fragment(write_block);

      if(first_free_fragment == NO_FREE_FRAGMENT)
      {
        ActualFBH::write_first_free_fragment(write_block, index.sub_index);
        use_as_control = true;
      }
      else if(index.sub_index < first_free_fragment)
      {
        ActualFBH::write_first_free_fragment(write_block, index.sub_index);
      }
    }

    if(use_as_control)
    {
      ActualFBH::write_null_next_index(write_block);
      allocate_pools_[fragment_size_index]->add_free(index.base_index, write_block);
    }

#   ifdef DEBUG_ALLOC_CONFLICT_CHECK_

    {
      typename SyncPolicyType::WriteGuard lock(debug_alloc_control_lock_);
      if(!debug_allocated_indexes_.erase(index))
      {
        raise(SIGSEGV);
      }
    }

#   endif
  }

  template<
    typename NextIndexType,
    typename NextIndexSerializerType,
    typename SyncPolicyType>
  void
  WriteFragmentLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  init_fragmentation_i_(
    const NextIndexType& /*block_index*/,
    WriteBlock* write_block,
    SizeType fragment_size)
    /*throw(BaseException)*/
  {
    typedef FragmentBlockHead<NextIndexType, NextIndexSerializerType> ActualFBH;

    SizeType avail_size = write_block->available_size();

    // + 1 it's space for Busy markers
    u_int32_t fragment_count =
      (avail_size - ActualFBH::size())/(1 + fragment_size);

    write_block->resize(ActualFBH::size() + fragment_count*(1 + fragment_size));
    ActualFBH::write_fragment_size(write_block, fragment_size);
    ActualFBH::write_fragment_count(write_block, fragment_count);
    ActualFBH::write_null_next_index(write_block);
    ActualFBH::write_first_free_fragment(write_block, 0);

    Generics::ArrayAutoPtr<char> markers_buf(fragment_count);
    ::memset(markers_buf.get(), FREE_BLOCK_FLAG, fragment_count);

    write_block->write(ActualFBH::size(), markers_buf.get(), fragment_count);
  }


  template<
    typename NextIndexType,
    typename NextIndexSerializerType,
    typename SyncPolicyType>
  typename WriteFragmentLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
    BlockIndex::SubIndex
  WriteFragmentLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  allocate_fragment_i_(
    ControlBlock& control_block, bool& last_fragment)
    /*throw(BaseException)*/
  {
    typedef FragmentBlockHead<NextIndexType, NextIndexSerializerType> ActualFBH;

    static const char* FUN = "WriteFragmentLayer::allocate_fragment_i_()";

    try
    {
      u_int32_t fragment_size =
        ActualFBH::read_fragment_size(control_block.block);
      u_int32_t fragment_count =
        ActualFBH::read_fragment_count(control_block.block);

      if (fragment_count > MAX_FRAGMENT_COUNT)
      {
        Stream::Error ostr;
        ostr << FUN << ": Exceeded maximum number of fragments: "
          "fragment_count = " << fragment_count <<
          ", max_fragment_count = " << MAX_FRAGMENT_COUNT;
        throw BaseException(ostr);
      }

      if((fragment_size + 1)*fragment_count + ActualFBH::size() !=
         control_block.block->size())
      {
        Stream::Error ostr;
        ostr << FUN << ": Non correct control block record:"
          " fragment_size = " << fragment_size <<
          " fragment_count = " << fragment_count <<
          " and block size = " << control_block.block->size() << ".";
        throw BaseException(ostr);
      }

      // TODO : REVIEW
      typename FragmentAllocateLockMap::WriteGuard block_lock(
        fragment_alloc_locks_.write_lock(control_block.index));

      /* search new first free fragment */
      char fragment_flag_buf[MAX_FRAGMENT_COUNT];
      control_block.block->read(ActualFBH::size(), fragment_flag_buf, fragment_count);

      typename BlockIndex::SubIndex result_index =
        ActualFBH::read_first_free_fragment(control_block.block);

      if(result_index == NO_FREE_FRAGMENT)
      {
        Stream::Error ostr;
        ostr << FUN << ": Cannot get free fragment";
        throw BaseException(ostr);
      }

      typename BlockIndex::SubIndex new_first_free_fragment = NO_FREE_FRAGMENT;

      if(fragment_flag_buf[result_index] != FREE_BLOCK_FLAG)
      {
        Stream::Error ostr;
        ostr << FUN << ": first free reference corrupted";
        throw BaseException(ostr);
      }
      
      for(typename BlockIndex::SubIndex i = result_index + 1;
          i < fragment_count; ++i)
      {
        if(fragment_flag_buf[i] == FREE_BLOCK_FLAG)
        {
          new_first_free_fragment = i;
          break;
        }
      }

      control_block.block->write(
        ActualFBH::size() + result_index, &USED_BLOCK_FLAG, sizeof(char));

      last_fragment = (new_first_free_fragment == NO_FREE_FRAGMENT);
      ActualFBH::write_first_free_fragment(
        control_block.block, new_first_free_fragment);

      return result_index;
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught eh::Exception: " << ex.what();
      throw BaseException(ostr);
    }
  }

  template<
    typename NextIndexType,
    typename NextIndexSerializerType,
    typename SyncPolicyType>
  void
  WriteFragmentLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  print_control_block_(std::ostream& out) noexcept
  {
    SizeType size = 0;

    try
    {
      size = main_control_block_->size();
    }
    catch(...)
    {}

    out << "size: " << size << std::endl;

    Generics::ArrayAutoPtr<u_int64_t> buf(size / sizeof(u_int64_t));
    main_control_block_->read(buf.get(), size);

    for(u_int64_t* c = buf.get(); c != buf.get() + size / sizeof(u_int64_t); c += 2)
    {
      out << *c << " : " << *(c + 1) << std::endl;
    }
  }
} // PlainStorage namespace
