/**
 * @file PlainStorage/BaseLayer.hpp
 *
 * Base interfaces for plain storage:
 *
 * ReadBlock
 * WriteBlock
 *
 * ReadBaseLayer<BLOCK_INDEX>
 * WriteBaseLayer<BLOCK_INDEX>
 *
 * DefaultBlockIndexAdapter<INPUT_BLOCK_INDEX, OUTPUT_BLOCK_INDEX>
 *
 * ReadLayerAdapter<BLOCK_INDEX_ADAPTER>
 * WriteLayerAdapter<BLOCK_INDEX_ADAPTER>
 *
 * BlockSequenceAllocator
 * BlockAllocator
 *
 */

#ifndef PLAINSTORAGELAYER_HPP
#define PLAINSTORAGELAYER_HPP

#include <list>
#include <cstring>

#include <Generics/ArrayAutoPtr.hpp>
#include <Stream/MemoryStream.hpp>
#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>

namespace PlainStorage
{
  DECLARE_EXCEPTION(BaseException, eh::DescriptiveException);
  DECLARE_EXCEPTION(CorruptedRecord, BaseException);

  typedef unsigned long SizeType;

  /**
   * Text value of property, store buffer and its size
   */
  struct PropertyValue
  {
    PropertyValue() : size_(0)
    {
    }

    PropertyValue(const PropertyValue& init)
    {
      init_value_(init.buffer_.get(), init.size_);
    }

    PropertyValue& operator=(const PropertyValue& right)
    {
      init_value_(right.buffer_.get() ,right.size_);
      return *this;
    }

    PropertyValue(const void* buf, SizeType sz)
    {
      init_value_(buf, sz);
    }

    SizeType
    size() const // noexcept
    {
      return size_;
    }

    const void*
    value() const // noexcept
    {
      return buffer_.get();
    }

    void
    value(const void* new_val, SizeType new_size) // noexcept
    {
      init_value_(new_val, new_size);
    }

  protected:
    void
    init_value_(const void* new_val, SizeType new_size) // noexcept
    {
      buffer_.reset(new_size);
      size_ = new_size;
      ::memcpy(buffer_.get(), new_val, size_);
    }

    Generics::ArrayChar buffer_;
    SizeType size_;
  };

  /**
   * ReadBlock interface
   * Block - a portion of memory, can read its contents and knows its own size.
   * Provides default methods for synchronization. (None of synch)
   * Read methods are needed because the data stored on disk in a chain of blocks
   * and may be fragmented, not solid
   */
  struct ReadBlock: public virtual ReferenceCounting::Interface
  {
    virtual void
    lock_read_() noexcept {}

    virtual void
    unlock_() noexcept {}

    /**
     * @return The size of these block
     */
    virtual SizeType
    size() /*throw(BaseException, CorruptedRecord)*/ = 0;

    /**
     * full read method
     * @return If the size of block less than buffer_size, return read size,
     * else method return 0
     */
    virtual SizeType
    read(void* buffer, SizeType buffer_size)
      /*throw(BaseException, CorruptedRecord)*/ = 0;

    /**
     * read part of block
     * throw exception if reading is outside of block bounds
     */
    virtual void
    read(SizeType pos, void* buffer, SizeType read_size)
      /*throw(BaseException, CorruptedRecord)*/ = 0;

    /** debug
     * get plained index of block
     * use it only for trace & debug plain storage
     */
    virtual SizeType
    read_index_(void* /*buffer*/, SizeType /*buffer_size*/)
      /*throw(BaseException)*/
    {
      return 0;
    }

    /** debug
     * print struct of block
     */
    virtual std::ostream&
    print_(std::ostream& out, const char* /*offset = ""*/) const
      noexcept
    {
      return out;
    }

    /** debug
     * print struct of block
     */
    virtual Stream::Error&
    print_(Stream::Error& out, const char* /*offset = ""*/) const
      noexcept
    {
      return out;
    }
  };

  typedef ReferenceCounting::SmartPtr<ReadBlock> ReadBlock_var;

  /**
   * WriteBlock interface
   * WriteBlock - a portion of memory, can read/write its contents and knows its own size.
   * Provides default methods for synchronization. (None of synch)
   */
  struct WriteBlock: public virtual ReadBlock
  {
    virtual void
    lock_write_() noexcept {}

    /**
     * @return The maximum size that may be used in resize and full write methods
     */
    virtual SizeType
    available_size() /*throw(BaseException)*/ = 0;

    /**
     * full write method (resize block if need)
     */
    virtual void
    write(const void* buffer, SizeType size)
      /*throw(BaseException, CorruptedRecord)*/ = 0;

    /**
     * resize block.
     * It need for using if your want fill block by parts
     */
    virtual void
    resize(SizeType size)
      /*throw(BaseException, CorruptedRecord)*/ = 0;

    /**
     * fill part of block.
     * It don't resize block.
     * throw exception if writing is outside of block bounds
     */
    virtual void
    write(SizeType pos, const void* buffer, SizeType size)
      /*throw(BaseException, CorruptedRecord)*/ = 0;

    /**
     * deallocate current block
     * if block is deallocated and program try call some method
     * method will throw exception
     */
    virtual void deallocate() /*throw(BaseException)*/ = 0;
  };

  typedef ReferenceCounting::SmartPtr<WriteBlock> WriteBlock_var;

  /**
   * ReadLayer - storage layer interface
   */
  template <typename BLOCK_INDEX>
  struct ReadBaseLayer : public virtual ReferenceCounting::DefaultImpl<>
  {
    typedef BLOCK_INDEX BlockIndex;

    virtual
    ReadBlock_var
    get_read_block(const BlockIndex& index)
      /*throw(BaseException, CorruptedRecord)*/ = 0;

    /**
     * @param name The name of properties to get value
     * @param property The value of property name
     * @return false if unknown properties requested
     */
    virtual bool
    get_property(const char* name, PropertyValue& property)
      /*throw(BaseException)*/ = 0;

    virtual
    unsigned long area_size() const noexcept = 0;
  };

  /**
   * WriteLayer - storage layer interface
   */
  template<typename BLOCK_INDEX>
  struct WriteBaseLayer : public virtual ReadBaseLayer<BLOCK_INDEX>
  {
    typedef BLOCK_INDEX BlockIndex;

    virtual
    WriteBlock_var
    get_write_block(const BlockIndex& index)
      /*throw(BaseException, CorruptedRecord)*/ = 0;

    virtual
    void
    set_property(const char* name, const PropertyValue& property)
      /*throw(BaseException)*/ = 0;
  };

  /**
   * ExWriteLayer - storage layer interface
   *   extension of WriteBaseLayer: + max_block_index() call
   * Notes: needed by Allocator implementations
   */
  template <typename BlockIndexType>
  struct ExWriteBaseLayer : public virtual WriteBaseLayer<BlockIndexType>
  {
    typedef BlockIndexType BlockIndex;

    /**
     * @return The maximum index of the block is available in the layer
     * Example for File layer: file size divided on block size.
     */
    virtual BlockIndex
    max_block_index() const noexcept = 0;
  };

  /**
   * Layer Adapters
   * util classes for linking layers with different block index structs
   */

  /**
   * DefaultBlockIndexAdapter
   *  default transformer of one IndexType to other
   *  by using type alignment
   */
  template<
    typename InputBlockIndexType,
    typename OutputBlockIndexType>
  struct DefaultBlockIndexAdapter
  {
    typedef InputBlockIndexType InputBlockIndex;
    typedef OutputBlockIndexType OutputBlockIndex;

    OutputBlockIndex
    operator ()(const InputBlockIndex& idx)
    {
      return static_cast<OutputBlockIndex>(idx);
    }
  };

  /**
   * ReadLayerAdapter
   */
  template<typename BlockIndexAdapterType>
  struct ReadLayerAdapter :
    public virtual ReadBaseLayer<typename BlockIndexAdapterType::InputBlockIndex>
  {
    typedef typename BlockIndexAdapterType::InputBlockIndex BlockIndex;
    typedef
      ReadBaseLayer<typename BlockIndexAdapterType::OutputBlockIndex>
      NextLayerType;

    ReadLayerAdapter(NextLayerType* next_layer) /*throw(BaseException)*/
      : next_layer_(next_layer)
    {}

    virtual ReadBlock*
    get_read_block(const BlockIndex& index)
      /*throw(BaseException, CorruptedRecord)*/
    {
      return next_layer_->get_read_layer(block_index_adapter_(index));
    }

  protected:
    BlockIndexAdapterType block_index_adapter_;
    NextLayerType* next_layer_;
  };

  /**
   * WriteLayerAdapter
   *   implementation of WriteBaseLayer for use some WriteLayer
   *   by other Index Type
   */
  template<typename BlockIndexAdapterType>
  struct WriteLayerAdapter :
    public virtual WriteBaseLayer<
      typename BlockIndexAdapterType::InputBlockIndex>
  {
    typedef typename BlockIndexAdapterType::InputBlockIndex BlockIndex;
    typedef
      WriteBaseLayer<typename BlockIndexAdapterType::OutputBlockIndex>
      NextLayerType;

    WriteLayerAdapter(NextLayerType* next_layer) /*throw(BaseException)*/
      : next_layer_(next_layer)
    {}

    virtual
    WriteBlock_var
    get_write_block(const BlockIndex& index)
      /*throw(BaseException, CorruptedRecord)*/
    {
      return next_layer_->get_write_block(block_index_adapter_(index));
    }

  protected:
    BlockIndexAdapterType block_index_adapter_;
    NextLayerType* next_layer_;
  };

  /**
   * BlockAllocator
   *  base allocate interface for allocating by one full block
   */
  template<typename BLOCK_INDEX>
  struct BlockAllocator : public virtual ReferenceCounting::DefaultImpl<>
  {
    struct AllocatedBlock
    {
      BLOCK_INDEX block_index;
      WriteBlock_var block;
    };

    virtual void
    allocate(AllocatedBlock& alloc_result) /*throw(BaseException)*/ = 0;
  };

  template <typename BlockIndexType>
  struct WriteAllocateBaseLayer:
    public virtual WriteBaseLayer<BlockIndexType>,
    public virtual BlockAllocator<BlockIndexType>
  {
  };

  typedef std::list<WriteBlock_var> WriteBlockSeq;

  /**
   * BlockSizeAllocator
   *  base allocate interface for allocating by concrete size
   */
  template<typename BlockIndexType>
  struct BlockSizeAllocator: public virtual ReferenceCounting::DefaultImpl<>
  {
    struct AllocatedBlock
    {
      BlockIndexType block_index;
      WriteBlock_var block;
    };

    virtual SizeType max_block_size() const noexcept = 0;

    virtual void allocate_size(
      SizeType size,
      AllocatedBlock& allocated_block) /*throw(BaseException)*/ = 0;
  };

  /**
   * Simple Serializer for POD types to/from memory buffers
   */
  template<typename PODType>
  struct PlainSerializer
  {
    static SizeType
    max_size() noexcept
    {
      return sizeof(PODType);
    };

    static SizeType
    size(const PODType& /*val*/) noexcept
    {
      return sizeof(PODType);
    }

    /**
     * Deserialize object from memory buffer
     * @param buf The source memory buffer
     * @param buf_size The size of memory buffer
     * @param val The object reference to be restored
     * @return sizeof of val
     */
    SizeType
    load(const void* buf, SizeType buf_size, PODType& val) const
      /*throw(BaseException)*/
    {
      if(buf_size < sizeof(PODType))
      {
        Stream::Error ostr;
        ostr << "PlainSerializer::read(): "
                "buffer for reading has incorrect size: " << buf_size
             << ", but must be great then " << sizeof(PODType) << ".";
        throw BaseException(ostr);
      }

      val = *(const PODType*)buf;
      return sizeof(PODType);
    }

    /**
     * Serialize object to memory buffer
     * @param val The object reference to be serialized to buf
     * @param buf The pointer to memory buffer that should store serialized
     * object
     * @param buf_size The size of memory buffer
     */
    void
    save(const PODType& val, void* buf, SizeType buf_size) const
      /*throw(BaseException)*/
    {
      if (buf_size < sizeof(PODType))
      {
        Stream::Error ostr;
        ostr << "PlainSerializer<>::save(): Not enough "
          "buffer size=" << buf_size << " to save object size="
          << sizeof(PODType);
        throw BaseException(ostr);
      }
      *(PODType*)buf = val;
    }
  };

  /**
   * DelegateReadBlockImpl
   * implementation of ReadBlock interface for delegate calls into
   * other ReadBlock object.
   */
  class DelegateReadBlockImpl: public virtual ReadBlock
  {
  public:
    DelegateReadBlockImpl(ReadBlock* read_block)
      : read_block_(ReferenceCounting::add_ref(read_block))
    {}

    virtual SizeType
    size() /*throw(BaseException, CorruptedRecord)*/
    {
      return read_block_->size();
    }

    virtual SizeType
    read(void* buffer, SizeType size)
      /*throw(BaseException, CorruptedRecord)*/
    {
      return read_block_->read(buffer, size);
    }

    virtual void
    read(SizeType pos, void* buffer, SizeType size)
      /*throw(BaseException, CorruptedRecord)*/
    {
      return read_block_->read(pos, buffer, size);
    }

    ReadBlock*
    read_block() noexcept
    {
      return read_block_;
    }

  private:
    ReadBlock_var read_block_;
  };

  /**
   * DelegateWriteBlockImpl class
   *  delegate implementation for WriteBlock.
   */
  class DelegateWriteBlockImpl: public virtual WriteBlock
  {
  public:
    DelegateWriteBlockImpl(WriteBlock* write_block)
      : write_block_(ReferenceCounting::add_ref(write_block))
    {}

    virtual SizeType
    size() /*throw(BaseException, CorruptedRecord)*/
    {
      return write_block_->size();
    }

    virtual SizeType
    read(void* buffer, SizeType size)
      /*throw(BaseException, CorruptedRecord)*/
    {
      return write_block_->read(buffer, size);
    }

    virtual void
    read(SizeType pos, void* buffer, SizeType size)
      /*throw(BaseException, CorruptedRecord)*/
    {
      write_block_->read(pos, buffer, size);
    }

    /* WriteBlock interface */
    virtual SizeType available_size() /*throw(BaseException)*/
    {
      return write_block_->available_size();
    }

    virtual void
    write(const void* buffer, SizeType size)
      /*throw(BaseException, CorruptedRecord)*/
    {
      write_block_->write(buffer, size);
    }

    virtual void
    write(SizeType pos, const void* buffer, SizeType size)
      /*throw(BaseException, CorruptedRecord)*/
    {
      write_block_->write(pos, buffer, size);
    }

    virtual void
    resize(SizeType size) /*throw(BaseException, CorruptedRecord)*/
    {
      write_block_->resize(size);
    }

    virtual void
    deallocate() /*throw(BaseException)*/
    {
      write_block_->deallocate();
    }

    WriteBlock* write_block() noexcept
    {
      return write_block_;
    }

  private:
    WriteBlock_var write_block_;
  };

  /**
   * Read block synchronization guard
   */
  class ReadBlockGuard
  {
  public:
    ReadBlockGuard(ReadBlock* read_block)
      : read_block_(ReferenceCounting::add_ref(read_block))
    {
      read_block_->lock_read_();
    }

    virtual ~ReadBlockGuard()
    {
      read_block_->unlock_();
    }

  protected:
    ReadBlock_var read_block_;
  };

  /**
   * Write block synchronization guard
   */
  class WriteBlockGuard
  {
  public:
    WriteBlockGuard(WriteBlock* write_block)
      : write_block_(ReferenceCounting::add_ref(write_block))
    {
      write_block_->lock_write_();
    }

    virtual ~WriteBlockGuard()
    {
      write_block_->unlock_();
    }

  protected:
    WriteBlock_var write_block_;
  };
}

#endif // PLAINSTORAGELAYER_HPP
