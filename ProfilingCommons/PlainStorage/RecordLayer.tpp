// @file ProfilingCommons/PlainStorage/RecordLayer.tpp

#ifndef PLAINSTORAGE_RECORDLAYER_TPP
#define PLAINSTORAGE_RECORDLAYER_TPP

#include <iostream>
#include <iomanip>
#include <vector>

#include <Generics/ArrayAutoPtr.hpp>
#include "RecordLayerUtils.hpp"

/**
 * file contains implementation of
 *
 * ReadRecordLayer
 * ReadRecordLayer::ReadRecordImpl
 *
 * WriteRecordLayer
 * WriteRecordLayer::WriteRecordImpl
 */
namespace PlainStorage
{
  namespace
  {
    /// Debug switch on if true
    const bool TRACE_WRITE_RECORD_BLOCK = false;
  }

  template<typename BlockIndexType, typename BlockIndexSerializerType>
  struct ReadRecordSubBlockHead
  {
    /**
     * HEAD of Record
     * HEAD format, fields are described as: [type: description, ...]
     *
     *  [ u_int32_t: full record size (defined only in first block from sequence )
     *    u_int32_t: index_size
     *    key:       byte[index_size]
     *    reserved:  byte[max_index_size - index_size]
     *  ]
     * Read block head of Record
     * @param block The block object that will contain the HEAD of Record
     * @param offset The offset from the beginning of Data block to begin of HEAD
     * @param next_index The location of Data block
     */
    static void
    read_next_index(
      ReadBlock* block,
      SizeType offset,
      BlockIndexType& next_index,
      u_int32_t* next_index_size = 0)
      /*throw (BaseException, CorruptedRecord)*/
    {
      static const char* FUN = "ReadRecordSubBlockHead::read_next_index()";

      if (offset != 0 && offset != sizeof(u_int32_t))
      {
        Stream::Error ostr;
        ostr << FUN << ": Requested incorrect offset=" << offset <<
          " from the beginning of the block ";
        throw CorruptedRecord(ostr);
      }

      u_int32_t index_size;

      block->read(offset, &index_size, sizeof(u_int32_t));

      if(next_index_size)
      {
        *next_index_size = index_size;
      }

      if(index_size != 0)
      {
        Generics::ArrayAutoPtr<char> buf(index_size);

        try
        {
          block->read(offset + sizeof(u_int32_t), buf.get(), index_size);
        }
        catch(const BaseException& ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": can't read index buffer with size = " << index_size <<
            " from pos = " << (offset + sizeof(u_int32_t)) << ": " <<
            ex.what();
          throw CorruptedRecord(ostr);
        }

        try
        {
          BlockIndexSerializerType index_serializer;
          index_serializer.load(buf.get(), index_size, next_index);
        }
        catch(const eh::Exception& ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": can't unserialize index: " << ex.what();
          throw CorruptedRecord(ostr);
        }
      }
      else
      {
        next_index = 0;
      }
    }

    /**
     * @return Calculated size of HEAD of Record.
     *  max_index_size + sizeof(index_size)
     */
    static int size() noexcept
    {
      return sizeof(u_int32_t) + BlockIndexSerializerType::max_size();
    }
  };

  template<typename BlockIndexType, typename BlockIndexSerializerType>
  struct WriteRecordSubBlockHead:
    public ReadRecordSubBlockHead<BlockIndexType, BlockIndexSerializerType>
  {
    /**
     * Index of zero length can be written simpler than the full index
     * @param block The write Block where will be saved nul next index
     * @param offset The location of place where should save nul next index
     */
    static void
    write_null_next_index(WriteBlock* block, SizeType offset)
      /*throw (CorruptedRecord)*/
    {
      if (offset != 0 && offset != sizeof(u_int32_t))
      {
        Stream::Error ostr;
        ostr << "WriteRecordSubBlockHead::write_null_next_index(): "
          "Try write next index with incorrect offset=" << offset <<
          " from the beginning of the block ";
        throw CorruptedRecord(ostr);
      }

      u_int32_t null_index_size = 0;
      block->write(offset, &null_index_size, sizeof(u_int32_t));
    }

    /**
     * At the beginning of the block is stored size of index, next placed the body
     * of the index.
     * Method write index to block
     */
    static void write_next_index(
      const BlockIndexType& next_index, WriteBlock* block, SizeType offset)
      /*throw (CorruptedRecord)*/
    {
      if (offset != 0 && offset != sizeof(u_int32_t))
      {
        Stream::Error ostr;
        ostr << "WriteRecordSubBlockHead::write_next_index(): "
          "Try write next index with incorrect offset=" << offset <<
          " from the beginning of the block ";
        throw CorruptedRecord(ostr);
      }

      BlockIndexSerializerType index_serializer;
      u_int32_t index_size = index_serializer.size(next_index);
      Generics::ArrayAutoPtr<char> buf(sizeof(u_int32_t) + index_size);
      *(u_int32_t*)buf.get() = index_size;
      index_serializer.save(next_index, buf.get() + sizeof(u_int32_t), index_size);
      block->write(offset, buf.get(), sizeof(u_int32_t) + index_size);
    }
  };

  /**
   * ReadRecordLayer::ReadRecordImpl
   * Read memory buffer from storage. Record store in set of blocks.
   */
  template<
    typename NextIndexType,
    typename NextIndexSerializerType,
    typename SyncPolicyType>
  class ReadRecordLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  ReadRecordImpl:
    public virtual ReadBlock,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    /**
     * Read Record head and find out the record size,
     * Record block head (next index), iterate through Record blocks sequence and
     * fill read_blocks container
     */
    ReadRecordImpl(
      ReadRecordLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>*
        read_file_layer,
      ReadBlock* first_block_in_seq) /*throw(BaseException)*/;

    /**
     * virtual empty destructor
     */
    virtual ~ReadRecordImpl() noexcept;

    virtual void lock_read_() noexcept { lock_.lock_read(); };

    virtual void unlock_() noexcept { lock_.unlock(); };

    /**
     * @return The size of the Record stored in full_size_
     */
    virtual SizeType size() /*throw(BaseException, CorruptedRecord)*/;

    /**
     * Read memory buffer from storage
     * @param buffer The pointer to allocated memory where the read record
     * @param size The size of buffer
     */
    virtual SizeType
    read(void* buffer, SizeType size) /*throw(BaseException, CorruptedRecord)*/;

    /**
     * Read memory buffer from storage starting from position.
     * @param pos Start position of Record to read buffer
     * @param buffer The pointer to allocated memory where the read record
     * @param size The size of buffer
     */
    virtual void
    read(SizeType pos, void* buffer, SizeType size)
      /*throw(BaseException, CorruptedRecord)*/;

    virtual SizeType
    read_index_(void* buffer, SizeType buffer_size)
      /*throw(BaseException)*/;

  protected:
    typedef
      ReadRecordSubBlockHead<NextIndexType, NextIndexSerializerType>
      SubBlockHead;

    /**
     * Structure need for work with sequence of blocks that store Record
     */
    struct ReadBlockWithPos
    {
      typedef ReadBlock BlockType;

      ReadBlockWithPos(
        SizeType pos_val, SizeType data_offset_val, ReadBlock* block_val)
        : pos(pos_val),
          data_offset(data_offset_val),
          block(ReferenceCounting::add_ref(block_val))
      {}

      /// Position of the block in the original piece of memory,
      /// number of first byte of the block put in a continuous source buffer
      SizeType pos;
      /// Each block have service HEAD, here offset to begin of data
      SizeType data_offset;
      /// Just data block
      ReadBlock_var block;
    };

    typedef std::vector<ReadBlockWithPos> ReadBlockWithPosList;
    typedef BlockWithPosLess<ReadBlockWithPos> ReadBlockWithPosLess;

    /// Allow access to Data Blocks
    ReadRecordLayer* read_record_layer_;

    typename SyncPolicyType::Mutex lock_;

    /// Initializing from Record header and contain size of record.
    /// The size of all user data in the record
    SizeType full_size_;

    /// Be filled with the sorted positions by ascending
    /// References to blocks + position in whole buffer + offset to user data
    /// Record placed in these blocks
    ReadBlockWithPosList read_blocks_;
  };

  /**
   * WriteRecordLayer::WriteRecordImpl
   */
  template<typename NextIndexType,
    typename NextIndexSerializerType, typename SyncPolicyType>
  class WriteRecordLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  WriteRecordImpl:
    public virtual WriteBlock,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    /**
     * Check structures consistency, read full size of Record, iterate through
     * blocks sequence and fill write_blocks_
     * @param write_file_layer The layer that allow write/read data blocks
     * @param first_block_in_seq Can be ordinary data block or fragmented data block, allocated
     *   through allocator gave to RecordLayer
     * @param block_index The index of first block in sequence, need to close data block
     */
    WriteRecordImpl(
      WriteRecordLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>*
        write_file_layer,
      WriteBlock* first_block_in_seq,
      const NextIndexType& block_index)
      /*throw(BaseException)*/;

    /**
     * When lifetime of Record is over we should remove Record from RecordLayer cache.
     * @return true if object is over, false if exist references to *this
     */
    virtual bool remove_ref_no_delete_() const noexcept;

    virtual void lock_read_() noexcept { lock_.lock_read(); };

    virtual void lock_write_() noexcept { lock_.lock_write(); };

    virtual void unlock_() noexcept { lock_.unlock(); };

    // ReadBlock interface
    /**
     * Check that write_blocks_ is not empty (Record still allocated)
     * @return The size of the Record stored in full_size_
     */
    virtual SizeType size() /*throw(BaseException, CorruptedRecord)*/;

    virtual SizeType
    read(void* buffer, SizeType size) /*throw(BaseException, CorruptedRecord)*/;

    virtual void
    read(SizeType pos, void* buffer, SizeType size)
      /*throw(BaseException, CorruptedRecord)*/;

    // WriteBlock interface
    /**
     * @return The size of memory available for user data in Record - unlimited
     */
    virtual SizeType available_size() /*throw(BaseException)*/;

    virtual void
    write(const void* buffer, SizeType size) /*throw(BaseException, CorruptedRecord)*/;

    virtual void
    write(SizeType pos, const void* buffer, SizeType size)
      /*throw(BaseException, CorruptedRecord)*/;

    /**
     * Change size of memory available for user, current content stored
     * @param size The new size of memory for user data
     */
    virtual void
    resize(SizeType size) /*throw(BaseException, CorruptedRecord)*/;

    /**
     * Set Record size to zero, deallocate all blocks from sequence,
     * clear blocks container
     */
    virtual void
    deallocate() /*throw(BaseException)*/;

    /**
     * block_index_ accessor
     */
    const NextIndexType&
    block_index() const noexcept;

    virtual SizeType
    read_index_(void* buffer, SizeType buffer_size)
      /*throw(BaseException)*/;

  protected:
    typedef
      WriteRecordSubBlockHead<NextIndexType, NextIndexSerializerType>
      SubBlockHead;

    typedef
      std::list<typename BlockSizeAllocator<BlockIndex>::AllocatedBlock>
      AllocatedBlockSeq;

  protected:
    /** virtual empty destructor */
    virtual ~WriteRecordImpl() noexcept;

    void
    resize_(SizeType new_size, bool keep_content)
      /*throw (BaseException, CorruptedRecord)*/;

    /**
     * We are looking for a position with which you want to remove blocks
     * and properly remove unwanted tail sequence of blocks.
     * @param new_size The size of memory allocated for user data,
     * Record can be smaller
     */
    void
    erase_excess_blocks_(SizeType new_size)
      /*throw (BaseException, CorruptedRecord)*/;

    /**
     * Adding blocks to the sequence of blocks
     */
    void
    attach_blocks_(SizeType add_size) /*throw (BaseException, CorruptedRecord)*/;

    /**
     * Write full_size_ variable readed from header of record, back
     * to header (update file)
     */
    void
    sync_full_size_() /*throw (BaseException, CorruptedRecord)*/;

    std::ostream& print_(
      std::ostream& ostr, const char* offset = "") const noexcept;

  protected:
    struct WriteBlockWithPos
    {
      typedef WriteBlock BlockType;

      WriteBlockWithPos(
        SizeType pos_val, SizeType data_offset_val, WriteBlock* block_val)
        : pos(pos_val),
          data_offset(data_offset_val),
          block(ReferenceCounting::add_ref(block_val))
      {}

      SizeType pos;
      SizeType data_offset;
      WriteBlock_var block;
    };

    typedef std::vector<WriteBlockWithPos> WriteBlockWithPosList;
    typedef BlockWithPosLess<WriteBlockWithPos> WriteBlockWithPosLess;

  protected:
    /// Reference to Record layer that cache opened Records, now we can remove
    /// Records from its cache
    WriteRecordLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>*
      write_record_layer_;

    typename SyncPolicyType::Mutex lock_;

    /// The size of all user data in the record
    SizeType full_size_;

    NextIndexType block_index_;

    /// Be filled with the sorted positions by ascending
    /// References to blocks + position in whole buffer + offset to user data
    /// Record placed in these blocks
    WriteBlockWithPosList write_blocks_;
  };

  /**
   * ReadRecordLayer::ReadRecordImpl impl
   */
  template<typename NextIndexType,
    typename NextIndexSerializerType, typename SyncPolicyType>
  ReadRecordLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  ReadRecordImpl::ReadRecordImpl(
    ReadRecordLayer<NextIndexType,
      NextIndexSerializerType, SyncPolicyType>* read_record_layer,
    ReadBlock* first_block_in_seq)
    /*throw(BaseException)*/
    : read_record_layer_(read_record_layer)
  {
    /* init block seq & full */
    SizeType first_block_size = first_block_in_seq->size();
    if(first_block_size != sizeof(u_int32_t))
    {
      Stream::Error ostr;
      ostr << "ReadRecordImpl::ReadRecordImpl(): Incorrect first block size = " <<
        first_block_size;
      throw BaseException(ostr);
    }

    // read record head
    u_int32_t fs_read;
    first_block_in_seq->read(0, &fs_read, sizeof(u_int32_t));
    full_size_ = fs_read;

    // read record block head
    NextBlockIndex next_index;
    SubBlockHead::read_next_index(first_block_in_seq, sizeof(u_int32_t), next_index);

    SizeType data_offset = sizeof(u_int32_t) + SubBlockHead::size();
    SizeType data_size = first_block_in_seq->size() - data_offset;

    read_blocks_.push_back(
      ReadBlockWithPos(0, data_offset, first_block_in_seq));

    SizeType sz_cur = data_size;

    while(sz_cur < full_size_)
    {
      ReadBlock_var cur_block =
        read_record_layer_->next_layer()->get_read_block(next_index);

      SubBlockHead::read_next_index(cur_block, 0, next_index);
      SizeType data_offset = SubBlockHead::size();
      SizeType data_size = first_block_in_seq->size() - data_offset;

      read_blocks_.push_back(
        ReadBlockWithPos(sz_cur, data_offset, cur_block));

      sz_cur += data_size;
    }
  }

  template<typename NextIndexType,
    typename NextIndexSerializerType, typename SyncPolicyType>
  ReadRecordLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  ReadRecordImpl::~ReadRecordImpl() noexcept
  {}

  template<typename NextIndexType,
    typename NextIndexSerializerType, typename SyncPolicyType>
  SizeType
  ReadRecordLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  ReadRecordImpl::size() /*throw(BaseException, CorruptedRecord)*/
  {
    return full_size_;
  }

  template<typename NextIndexType,
    typename NextIndexSerializerType, typename SyncPolicyType>
  SizeType
  ReadRecordLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  ReadRecordImpl::read(void* buffer, SizeType buffer_size)
    /*throw(BaseException, CorruptedRecord)*/
  {
    if(buffer_size < full_size_)
    {
      return 0;
    }

    read_block_seq<ReadBlockWithPosList>(
      read_blocks_,
      buffer,
      full_size_);

    return full_size_;
  }

  template<typename NextIndexType,
    typename NextIndexSerializerType, typename SyncPolicyType>
  SizeType
  ReadRecordLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  ReadRecordImpl::read_index_(void* buffer, SizeType buffer_size)
    /*throw(BaseException)*/
  {
    return read_blocks_.begin()->block->read_index_(buffer, buffer_size);
  }

  template<typename NextIndexType,
    typename NextIndexSerializerType, typename SyncPolicyType>
  void
  ReadRecordLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  ReadRecordImpl::read(
    SizeType pos, void* buffer, SizeType read_size)
    /*throw(BaseException, CorruptedRecord)*/
  {
    static const char* FUN = "ReadRecordImpl::read(pos, ...)";

    if(pos + read_size >= full_size_)
    {
      Stream::Error ostr;
      ostr << FUN << ": incorrect pos or read size, record size = " <<
        full_size_ << ", read pos = " << pos << ", read size = " << read_size;
      throw BaseException(ostr);
    }

    read_block_seq_part<ReadBlockWithPosList, ReadBlockWithPosLess>(
      read_blocks_,
      pos,
      buffer,
      read_size);
  }

  /**
   * WriteRecordLayer::WriteRecordImpl implementation
   */
  template<typename NextIndexType,
    typename NextIndexSerializerType, typename SyncPolicyType>
  WriteRecordLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  WriteRecordImpl::WriteRecordImpl(
    WriteRecordLayer<NextIndexType,
      NextIndexSerializerType, SyncPolicyType>* write_record_layer,
    WriteBlock* first_block_in_seq,
    const NextIndexType& block_index)
    /*throw(BaseException)*/
    : write_record_layer_(write_record_layer),
      block_index_(block_index)
  {
    static const char* FUN = "WriteRecordImpl::WriteRecordImpl()";

    try
    {
      /* init block seq & full */
      SizeType first_block_size = first_block_in_seq->size();

      if(first_block_size < sizeof(u_int32_t))
      {
        throw BaseException("record head block size < 4");
      }

      u_int32_t full_size_read;

      try
      {
        first_block_in_seq->read(0, &full_size_read, sizeof(u_int32_t));
      }
      catch(const BaseException& ex)
      {
        Stream::Error ostr;
        ostr << "can't read record full size: " << ex.what();
        throw BaseException(ostr);
      }

      full_size_ = full_size_read;

      NextBlockIndex next_index;

      try
      {
        SubBlockHead::read_next_index(first_block_in_seq, sizeof(u_int32_t), next_index);
      }
      catch(const BaseException& ex)
      {
        Stream::Error ostr;
        ostr << "can't read first record index: " << ex.what();
        throw BaseException(ostr);
      }

      SizeType data_offset = sizeof(u_int32_t) + SubBlockHead::size();
      SizeType data_size = first_block_size - data_offset;

      write_blocks_.push_back(
        WriteBlockWithPos(0, data_offset, first_block_in_seq));

      SizeType sz_cur = data_size;

      while((sz_cur < full_size_) && (next_index != 0))
      {
        WriteBlock_var cur_block;

        try
        {
          cur_block =
            write_record_layer_->next_layer()->get_write_block(next_index);
        }
        catch(const BaseException& ex)
        {
          Stream::Error ostr;
          ostr << "can't resolve record block with index = ("
            << next_index << "):" << ex.what();
          throw BaseException(ostr);
        }

        SizeType data_offset = SubBlockHead::size();
        SizeType data_size = cur_block->size() - data_offset;

        write_blocks_.push_back(
          WriteBlockWithPos(sz_cur, data_offset, cur_block));

        sz_cur += data_size;

        NextIndexType cur_index(next_index);

        try
        {
          SubBlockHead::read_next_index(cur_block, 0, next_index);
        }
        catch(const BaseException& ex)
        {
          Stream::Error ostr;
          ostr << "can't read record next index in block ("
            << cur_index << "): " << ex.what();
          throw BaseException(ostr);
        }
      }
    }
    catch(const BaseException& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": can't init record by index (" <<
        block_index << "): " << ex.what() << std::endl <<
        "Record trace (" << this << "): " << std::endl;
      print_(ostr);
      throw BaseException(ostr);
    }

    if(TRACE_WRITE_RECORD_BLOCK)
    {
      std::cout << FUN << ": this = " << this <<
        ", full_size_ = " << full_size_ <<
        ", block_index_ = (" << block_index_ << ")" << std::endl;
    }
  }

  template<typename NextIndexType,
    typename NextIndexSerializerType, typename SyncPolicyType>
  bool
  WriteRecordLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  WriteRecordImpl::remove_ref_no_delete_() const noexcept
  {
    if(write_record_layer_)
    {
      typename SyncPolicyType::WriteGuard lock(write_record_layer_->opened_records_lock_);
      if(AtomicImpl::remove_ref_no_delete_())
      {
        write_record_layer_->close_record_i_(this);
        return true;
      }
    }
    else if(AtomicImpl::remove_ref_no_delete_())
    {
      return true;
    }

    return false;
  }

  template<typename NextIndexType,
    typename NextIndexSerializerType, typename SyncPolicyType>
  WriteRecordLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  WriteRecordImpl::~WriteRecordImpl() noexcept
  {
    if(TRACE_WRITE_RECORD_BLOCK)
    {
      std::cout << "WriteRecordImpl::~WriteRecordImpl(): this = " << this <<
        ", full_size_ = " << full_size_ <<
        ", block_index_ = (" << block_index_ << ")" << std::endl;
    }
  }

  template<typename NextIndexType,
    typename NextIndexSerializerType, typename SyncPolicyType>
  SizeType
  WriteRecordLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  WriteRecordImpl::size() /*throw(BaseException, CorruptedRecord)*/
  {
    static const char* FUN = "WriteRecordImpl::size()";

    if(TRACE_WRITE_RECORD_BLOCK)
    {
      std::cout << "WriteRecordImpl::size(): this = " << this <<
        ", full_size_ = " << full_size_ <<
        ", block_index_ = (" << block_index_ << ")" << std::endl;
    }

    if(write_blocks_.empty())
    {
      Stream::Error ostr;
      ostr << FUN << ": record already deallocated";
      throw BaseException(ostr);
    }

    return full_size_;
  }

  template<typename NextIndexType,
    typename NextIndexSerializerType, typename SyncPolicyType>
  SizeType
  WriteRecordLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  WriteRecordImpl::read(
    void* buffer, SizeType buffer_size) /*throw(BaseException, CorruptedRecord)*/
  {
    static const char* FUN = "WriteRecordImpl::read()";

    if(TRACE_WRITE_RECORD_BLOCK)
    {
      std::cout << FUN << ": this = " << this <<
        ", full_size_ = " << full_size_ <<
        ", buffer_size = " << buffer_size <<
        ", block_index_ = (" << block_index_ << ")" << std::endl;
    }

    if(write_blocks_.empty())
    {
      Stream::Error ostr;
      ostr << FUN << ": record already deallocated";
      throw BaseException(ostr);
    }

    if(buffer_size < full_size_)
    {
      return 0;
    }

    try
    {
      read_block_seq<WriteBlockWithPosList>(
        write_blocks_,
        buffer,
        full_size_);
    }
    catch(const BaseException& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": " << ex.what() << std::endl <<
        "Record trace (" << this << "): " << std::endl;
      print_(ostr);
      throw BaseException(ostr);
    }

    return full_size_;
  }

  template<typename NextIndexType,
    typename NextIndexSerializerType, typename SyncPolicyType>
  void
  WriteRecordLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  WriteRecordImpl::read(
    SizeType pos, void* buffer, SizeType read_size)
    /*throw(BaseException, CorruptedRecord)*/
  {
    static const char* FUN = "WriteRecordImpl::read(pos, ...)";

    if(TRACE_WRITE_RECORD_BLOCK)
    {
      std::cout << FUN << ": this = " << this <<
        ", full_size_ = " << full_size_ <<
        ", pos = " << pos <<
        ", read_size = " << read_size <<
        ", block_index_ = (" << block_index_ << ")" << std::endl;
    }

    if(write_blocks_.empty())
    {
      Stream::Error ostr;
      ostr << FUN << ": record already deallocated";
      throw BaseException(ostr);
    }

    if(pos + read_size > full_size_)
    {
      Stream::Error ostr;
      ostr << FUN << ": incorrect posed reading: pos = " << pos <<
        ", size to read = " << read_size <<
        ", full size of record = " << full_size_ <<
        ", index = (" << block_index_ <<
        ", this = " << this;

      throw BaseException(ostr);
    }

    try
    {
      read_block_seq_part<WriteBlockWithPosList, WriteBlockWithPosLess>(
        write_blocks_,
        pos,
        buffer,
        read_size);
    }
    catch(const BaseException& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": " << ex.what() << std::endl <<
        "Read params: pos = " << pos <<
        ", read_size = " << read_size << std::endl <<
        "Record trace (" << this << "): " << std::endl;
      print_(ostr);
      throw BaseException(ostr);
    }
  }

  template<typename NextIndexType,
    typename NextIndexSerializerType, typename SyncPolicyType>
  void
  WriteRecordLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  WriteRecordImpl::resize(SizeType new_size)
    /*throw(BaseException, CorruptedRecord)*/
  {
    if(TRACE_WRITE_RECORD_BLOCK)
    {
      std::cout << "WriteRecordImpl::resize(): this = " << this <<
        ", full_size_ = " << full_size_ <<
        ", new_size = " << new_size <<
        ", block_index_ = (" << block_index_ << ")" << std::endl;
    }

    resize_(new_size, true);
  }

  template<typename NextIndexType,
    typename NextIndexSerializerType, typename SyncPolicyType>
  SizeType
  WriteRecordLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  WriteRecordImpl::available_size()
    /*throw(BaseException)*/
  {
    return 0xFFFFFFFF; // unlimited
  }

  template<typename NextIndexType,
    typename NextIndexSerializerType, typename SyncPolicyType>
  void
  WriteRecordLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  WriteRecordImpl::write(
    const void* buffer, SizeType write_size)
    /*throw(BaseException, CorruptedRecord)*/
  {
    static const char* FUN = "WriteRecordImpl::write()";

    if(TRACE_WRITE_RECORD_BLOCK)
    {
      std::cout << FUN << ": this = " << this <<
        ", full_size_ = " << full_size_ <<
        ", write_size = " << write_size <<
        ", block_index_ = (" << block_index_ << ")" << std::endl;
    }

    if(write_blocks_.empty())
    {
      Stream::Error ostr;
      ostr << FUN << ": record already deallocated";
      throw BaseException(ostr);
    }

    try
    {
      if(full_size_ != write_size)
      {
        resize_(write_size, false);
      }

      write_block_seq<WriteBlockWithPosList>(
        write_blocks_,
        buffer,
        write_size);
    }
    catch(const BaseException& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": " << ex.what() << std::endl <<
        "Write params: write_size = " << write_size << std::endl <<
        "Record trace (" << this << "): " << std::endl;
      print_(ostr);
      throw BaseException(ostr);
    }
  }

  template<typename NextIndexType,
    typename NextIndexSerializerType, typename SyncPolicyType>
  void
  WriteRecordLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  WriteRecordImpl::write(
    SizeType pos, const void* buffer, SizeType write_size)
    /*throw(BaseException, CorruptedRecord)*/
  {
    static const char* FUN = "WriteRecordImpl::write(pos, ...)";

    if(write_blocks_.empty())
    {
      Stream::Error ostr;
      ostr << FUN << ": record already deallocated";
      throw BaseException(ostr);
    }

    if(pos + write_size > full_size_)
    {
      Stream::Error ostr;
      ostr << FUN << ": bounds error: pos = " << pos <<
        ", write size = " << write_size <<
        ", full size = " << full_size_;
      throw BaseException(ostr);
    }

    write_block_seq_part<WriteBlockWithPosList, WriteBlockWithPosLess>(
      write_blocks_,
      pos,
      buffer,
      write_size);
  }

  template<typename NextIndexType,
    typename NextIndexSerializerType, typename SyncPolicyType>
  void WriteRecordLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  WriteRecordImpl::deallocate()
    /*throw(BaseException)*/
  {
    static const char* FUN = "WriteRecordImpl::deallocate()";

    if(!write_record_layer_)
    {
      Stream::Error ostr;
      ostr << FUN << ": record already deallocated";
      throw BaseException(ostr);
    }

    {
      // unlink from write layer, see remove_ref_no_delete_()
      typename SyncPolicyType::WriteGuard lock(
        write_record_layer_->opened_records_lock_);

      write_record_layer_->close_record_i_(this);

      write_record_layer_ = 0;
    }

    full_size_ = 0;

    for(typename WriteBlockWithPosList::iterator it = write_blocks_.begin();
        it != write_blocks_.end(); ++it)
    {
      it->block->deallocate();
    }

    write_blocks_.clear();
  }

  template<typename NextIndexType,
    typename NextIndexSerializerType, typename SyncPolicyType>
  const NextIndexType&
  WriteRecordLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  WriteRecordImpl::block_index() const noexcept
  {
    return block_index_;
  }

  template<typename NextIndexType,
    typename NextIndexSerializerType, typename SyncPolicyType>
  SizeType
  WriteRecordLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  WriteRecordImpl::read_index_(void* buffer, SizeType buffer_size)
    /*throw(BaseException)*/
  {
    return write_blocks_.begin()->block->read_index_(buffer, buffer_size);
  }

  template<typename NextIndexType,
    typename NextIndexSerializerType, typename SyncPolicyType>
  void
  WriteRecordLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  WriteRecordImpl::sync_full_size_()
    /*throw (BaseException, CorruptedRecord)*/
  {
    static const char* FUN = "WriteRecordLayer::WriteRecordImpl::sync_full_size_()";

    if(write_blocks_.empty())
    {
      Stream::Error ostr;
      ostr << FUN << ": Empty index of write blocks";
      throw CorruptedRecord(ostr);
    }

    if(full_size_ > available_size())
    {
      Stream::Error ostr;
      ostr << FUN << ": The size of user data=" <<
        full_size_ << " excess available size=" << available_size();
      throw CorruptedRecord(ostr);
    }

    u_int32_t fs_write = full_size_;
    write_blocks_.begin()->block->write(0, &fs_write, sizeof(u_int32_t));
  }

  template<typename NextIndexType,
    typename NextIndexSerializerType, typename SyncPolicyType>
  void
  WriteRecordLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  WriteRecordImpl::erase_excess_blocks_(SizeType new_size)
    /*throw (BaseException, CorruptedRecord)*/
  {
    static const char* FUN =
      "WriteRecordLayer::WriteRecordImpl::erase_excess_blocks_()";

    if (write_blocks_.empty())
    {
      Stream::Error ostr;
      ostr << FUN << ": Empty index of write blocks";
      throw CorruptedRecord(ostr);
    }
    
    SizeType cur_size = full_size_;

    typename WriteBlockWithPosList::iterator erase_it = write_blocks_.end();

    for(typename WriteBlockWithPosList::iterator it = --write_blocks_.end();
        it != write_blocks_.begin(); --it)
    {
      SizeType data_size = it->block->size() - it->data_offset;
      cur_size -= data_size;

      if(new_size >= cur_size)
      {
        erase_it = it;
        break;
      }
    }

    if(erase_it != write_blocks_.end()) // not one block sequence
    {
      if(cur_size + erase_it->block->available_size() -
           erase_it->data_offset < new_size)
      {
        full_size_ = cur_size;
      }
      else
      {
        // simple resize last block
        erase_it->block->resize(new_size - cur_size + erase_it->data_offset);
        ++erase_it;
        full_size_ = new_size;
      }

      {
        // link last block reference into null
        typename WriteBlockWithPosList::iterator prev_it = erase_it;
        --prev_it;
        SizeType last_head_offset = (
          prev_it == write_blocks_.begin() ? sizeof(u_int32_t) : 0);
        SubBlockHead::write_null_next_index(prev_it->block, last_head_offset);
      }

      write_blocks_.erase(erase_it, write_blocks_.end());
    }
  }

  template<typename NextIndexType,
    typename NextIndexSerializerType, typename SyncPolicyType>
  void
  WriteRecordLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  WriteRecordImpl::attach_blocks_(SizeType add_size)
    /*throw (BaseException, CorruptedRecord)*/
  {
    static const char* FUN = "WriteRecordLayer::WriteRecordImpl::attach_blocks_()";

    if(add_size)
    {
      if (write_blocks_.empty())
      {
        Stream::Error ostr;
        ostr << FUN << ": Empty index of write blocks";
        throw CorruptedRecord(ostr);
      }

      BlockSizeAllocator<BlockIndex>* allocator =
        write_record_layer_->allocator();

      SizeType attach_size = add_size;
      SizeType safe_block_size = allocator->max_block_size();

      WriteBlock_var last_alloc_block;

      while(attach_size > 0)
      {
        SizeType alloc_size;
        if(attach_size + SubBlockHead::size() > safe_block_size)
        {
          alloc_size = safe_block_size;
          attach_size -= safe_block_size - SubBlockHead::size();
        }
        else
        {
          alloc_size = attach_size + SubBlockHead::size();
          attach_size = 0;
        }

        typename BlockSizeAllocator<BlockIndex>::AllocatedBlock alloc;
        allocator->allocate_size(alloc_size, alloc);

        typename WriteBlockWithPosList::iterator last_it = --write_blocks_.end();
        SizeType last_head_offset = (
          last_it == write_blocks_.begin() ? sizeof(u_int32_t) : 0);
        SubBlockHead::write_next_index(
          alloc.block_index, last_it->block, last_head_offset);
        write_blocks_.push_back(
          WriteBlockWithPos(
            last_it->pos + last_it->block->size() - last_it->data_offset,
            SubBlockHead::size(),
            alloc.block));
        last_alloc_block = alloc.block;
      }

      if(last_alloc_block.in())
      {
        SubBlockHead::write_null_next_index(last_alloc_block, 0);
      }

      full_size_ += add_size;
    }
  }

  template<typename NextIndexType,
    typename NextIndexSerializerType, typename SyncPolicyType>
  void
  WriteRecordLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  WriteRecordImpl::resize_(SizeType new_size, bool keep_content)
    /*throw (BaseException, CorruptedRecord)*/
  {
    static const char* FUN = "WriteRecordLayer::WriteRecordImpl::resize_()";

    if(TRACE_WRITE_RECORD_BLOCK)
    {
      std::cout << FUN << ": full_size_ = " << full_size_ <<
        ", new_size = " << new_size <<
        ", keep_content = " << keep_content << std::endl;
    }

    if(!keep_content)
    {
      erase_excess_blocks_(new_size);
      if (new_size < full_size_)
      {
        Stream::Error ostr;
        ostr << FUN << ": lost user data while resizing";
        throw CorruptedRecord(ostr);
      }
      attach_blocks_(new_size - full_size_);
    }
    else
    {
      /* no fragmentation control */
      attach_blocks_(new_size - full_size_);
    }

    sync_full_size_();
  }

  template<typename NextIndexType,
    typename NextIndexSerializerType, typename SyncPolicyType>
  std::ostream&
  WriteRecordLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  WriteRecordImpl::print_(std::ostream& ostr, const char* offset)
    const noexcept
  {
    ostr << offset << "this = " << this << std::endl <<
      offset << "full_size = " << full_size_ << std::endl <<
      offset << "block sequence: " << std::endl;

    unsigned long block_i = 0;
    for(typename WriteBlockWithPosList::const_iterator it = write_blocks_.begin();
        it != write_blocks_.end(); ++it, ++block_i)
    {
      ostr << offset << "  #" << block_i <<
        ":  pos = " << it->pos << ", data-offset = " << it->data_offset <<
        ", avail-size = ";

      try
      {
        ostr << it->block->available_size();
      }
      catch(...)
      {
        ostr << "undefined";
      }

      ostr << ", size = ";

      try
      {
        ostr << it->block->size();
      }
      catch(...)
      {
        ostr << "undefined";
      }

      ostr << ", next-index = (";

      uint32_t next_index_size = 0;
      SizeType next_index_offset = block_i == 0 ? sizeof(u_int32_t) : 0;

      try
      {
        NextBlockIndex next_index;

        SubBlockHead::read_next_index(
          it->block,
          next_index_offset,
          next_index,
          &next_index_size);
        ostr << next_index << "[" << next_index_size << "]";
      }
      catch(...)
      {
        ostr << "nonreadable[" << next_index_size << "]{";
        uint32_t to_read_index_size = std::min(
          next_index_offset <= it->block->size() ?
            it->block->size() - next_index_offset : 0,
          static_cast<SizeType>(next_index_size));
        Generics::ArrayAutoPtr<unsigned char> index_buf(to_read_index_size);
        it->block->read(next_index_offset, index_buf.get(), to_read_index_size);

        ostr << std::hex << std::setfill('0');
        for(unsigned long i = 0; i < to_read_index_size; ++i)
        {
          ostr << (i != 0 ? " " : "") << "0x" << std::setw(2) <<
            static_cast<unsigned int>(index_buf[i]);
        }
        ostr << "}" << std::dec << std::setw(0);
      }

      ostr << "): " << std::endl;
      it->block->print_(ostr, (std::string(offset) + "    ").c_str());
    }

    ostr << std::endl;
    return ostr;
  }

  /**
   * ReadRecordLayer implementation
   */
  template<typename NextIndexType,
    typename NextIndexSerializerType, typename SyncPolicyType>
  ReadRecordLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  ReadRecordLayer(
    ReadBaseLayer<NextIndexType>* next_layer) /*throw(Exception)*/
    : TransientPropertyReadLayerImpl<NextIndexType, NextIndexType>(next_layer)
  {}

  template<typename NextIndexType,
    typename NextIndexSerializerType, typename SyncPolicyType>
  ReadRecordLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  ~ReadRecordLayer() noexcept
  {}

  template<typename NextIndexType,
    typename NextIndexSerializerType, typename SyncPolicyType>
  unsigned long
  ReadRecordLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  area_size() const noexcept
  {
    return this->next_layer()->area_size();
  }

  template<typename NextIndexType,
    typename NextIndexSerializerType, typename SyncPolicyType>
  ReadBlock_var
  ReadRecordLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  get_read_block(const BlockIndex& block_index)
    /*throw(BaseException, CorruptedRecord)*/
  {
    return new ReadRecordImpl(
      this, this->next_layer()->get_read_block(block_index));
  }

  /** WriteRecordLayer */
  template<typename NextIndexType,
    typename NextIndexSerializerType, typename SyncPolicyType>
  WriteRecordLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  WriteRecordLayer(
    WriteBaseLayer<NextIndexType>* next_layer,
    BlockSizeAllocator<NextIndexType>* size_allocator)
    /*throw(Exception)*/
    : TransientPropertyReadLayerImpl<NextIndexType, NextIndexType>(next_layer),
      TransientPropertyWriteLayerImpl<NextIndexType, NextIndexType>(next_layer),
      ReadRecordLayer<NextIndexType,
        NextIndexSerializerType, SyncPolicyType>(next_layer),
      size_allocator_(ReferenceCounting::add_ref(size_allocator))
  {}

  template<typename NextIndexType,
    typename NextIndexSerializerType, typename SyncPolicyType>
  WriteRecordLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  ~WriteRecordLayer() noexcept
  {}

  template<typename NextIndexType,
    typename NextIndexSerializerType, typename SyncPolicyType>
  unsigned long
  WriteRecordLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  area_size() const noexcept
  {
    return this->next_layer()->area_size();
  }

  template<typename NextIndexType,
    typename NextIndexSerializerType, typename SyncPolicyType>
  void
  WriteRecordLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  allocate(AllocatedBlock& alloc_result)
    /*throw(BaseException)*/
  {
    typedef
      WriteRecordSubBlockHead<NextIndexType, NextIndexSerializerType>
      SubBlockHead;

    typename BlockSizeAllocator<BlockIndex>::AllocatedBlock alloc_block;
    size_allocator_->allocate_size(
      SubBlockHead::size() + sizeof(u_int32_t), alloc_block);

    /* init record head */
    u_int32_t null_full_size = 0;
    alloc_block.block->write(0, &null_full_size, sizeof(u_int32_t));

    /* init record block head */
    SubBlockHead::write_null_next_index(alloc_block.block, sizeof(u_int32_t));

    alloc_result.block_index = alloc_block.block_index;
    WriteRecordImpl* new_record = new WriteRecordImpl(
      this, alloc_block.block, alloc_block.block_index);
    alloc_result.block = new_record;

    /* fill opened records: need lock only opened_records_lock_,
     * because record isn't used now */
    typename SyncPolicyType::WriteGuard lock(opened_records_lock_);
    if(!opened_records_.insert(std::make_pair(
         alloc_result.block_index, new_record)).second)
    {
      assert(0);
    }
  }

  template<typename NextIndexType,
    typename NextIndexSerializerType, typename SyncPolicyType>
  ReadBlock_var
  WriteRecordLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  get_read_block(
    const BlockIndex& block_index)
    /*throw(BaseException, CorruptedRecord)*/
  {
    return get_write_block(block_index);
  }

  template<typename NextIndexType,
    typename NextIndexSerializerType, typename SyncPolicyType>
  WriteBlock_var
  WriteRecordLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  get_write_block(
    const BlockIndex& block_index)
    /*throw(BaseException, CorruptedRecord)*/
  {
    {
      typename SyncPolicyType::WriteGuard lock(opened_records_lock_);
      typename OpenedMap::iterator it = opened_records_.find(block_index);
      if(it != opened_records_.end())
      {
        return ReferenceCounting::add_ref(it->second);
      }
    }

    ReferenceCounting::SmartPtr<WriteRecordImpl> new_record =
      new WriteRecordImpl(
        this, this->next_layer()->get_write_block(block_index), block_index);

    {
      typename SyncPolicyType::WriteGuard lock(opened_records_lock_);
      typename OpenedMap::iterator it = opened_records_.find(block_index);
      if(it != opened_records_.end())
      {
        return ReferenceCounting::add_ref(it->second);
      }
      opened_records_.insert(std::make_pair(block_index, new_record.in()));
    }

    return new_record;
  }

  template<typename NextIndexType,
    typename NextIndexSerializerType, typename SyncPolicyType>
  BlockSizeAllocator<NextIndexType>*
  WriteRecordLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  allocator()
    noexcept
  {
    return size_allocator_;
  }

  template<typename NextIndexType,
    typename NextIndexSerializerType, typename SyncPolicyType>
  void
  WriteRecordLayer<NextIndexType, NextIndexSerializerType, SyncPolicyType>::
  close_record_i_(
    const WriteRecordImpl* record)
    noexcept
  {
    opened_records_.erase(record->block_index());
  }

} // namespace PlainStorage

#endif // PLAINSTORAGE_RECORDLAYER_TPP
