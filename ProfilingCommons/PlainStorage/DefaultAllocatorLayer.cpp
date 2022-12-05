#include <sstream>
#include <iostream>
#include <Stream/MemoryStream.hpp>
#include <Generics/Time.hpp>
#include "DefaultAllocatorLayer.hpp"

namespace PlainStorage
{
  namespace
  {
    // number of blocks that will be allocated if all blocks is used >= 2
    const unsigned long ALLOCATE_PORTION = 100;
  }
  
  namespace
  {
    const char DEFAULT_ALLOCATOR_CONTROL_INDEX_PROP_NAME[] =
      "DefaultAllocatorLayer.ControlBlockIndex";
  }

  /**
   * DefaultAllocatorLayer::WriteBlockImpl
   * delegate all calls to WriteBlock from next layer
   * except deallocate call
   */
  class DefaultAllocatorLayer::WriteBlockImpl:
    public virtual DelegateWriteBlockImpl,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    WriteBlockImpl(
      DefaultAllocatorLayer* allocator_layer,
      WriteBlock* write_block,
      FileBlockIndex index)
      /*throw(BaseException)*/;

    virtual
    ~WriteBlockImpl() noexcept;

    virtual void
    deallocate() /*throw(BaseException)*/;

  protected:
    FileBlockIndex index_;
    DefaultAllocatorLayer* allocator_layer_;
  };


  DefaultAllocatorLayer::WriteBlockImpl::WriteBlockImpl(
    DefaultAllocatorLayer* allocator_layer,
    WriteBlock* control_block,
    FileBlockIndex index)
    /*throw(BaseException)*/
    : DelegateWriteBlockImpl(control_block),
      index_(index),
      allocator_layer_(allocator_layer)
  {}

  DefaultAllocatorLayer::WriteBlockImpl::~WriteBlockImpl() noexcept
  {}

  void
  DefaultAllocatorLayer::WriteBlockImpl::deallocate()
    /*throw(BaseException)*/
  {
    allocator_layer_->deallocate_(write_block(), index_);
  }

  /**
   * DefaultAllocatorLayer
   */
  DefaultAllocatorLayer::DefaultAllocatorLayer(
    ExWriteBaseLayer<FileBlockIndex>* next_layer)
    /*throw(BaseException)*/
    : next_layer_(ReferenceCounting::add_ref(next_layer)),
      first_free_block_index_(0),
      allocate_blocks_size_(0)
  {
    PropertyValue allocator_name;

    if(!next_layer_->get_property("AllocatorType", allocator_name))
    {
      const char NAME[] = "Default";
      allocator_name.value(NAME, sizeof(NAME));

      /* init allocator for new file */
      next_layer_->set_property("AllocatorType", allocator_name);

      FileBlockIndex max_index = next_layer_->max_block_index();
      control_block_ = next_layer_->get_write_block(max_index);

      {
        std::ostringstream ostr;
        ostr << max_index;
        const std::string& str = ostr.str();

        PropertyValue prop(str.c_str(), str.length() + 1);
        next_layer_->set_property(
          DEFAULT_ALLOCATOR_CONTROL_INDEX_PROP_NAME, prop);
      }

      write_next_index_(control_block_, 0);
    }
    else
    {
      if(::strcmp((const char*)allocator_name.value(), "Default") == 0)
      {
        /* init control block */
        {
          PropertyValue prop;

          if(!next_layer_->get_property(
               DEFAULT_ALLOCATOR_CONTROL_INDEX_PROP_NAME, prop))
          {
            Stream::Error ostr;
            ostr << "File use 'Default' allocator, but don't contain '"
                 << DEFAULT_ALLOCATOR_CONTROL_INDEX_PROP_NAME << "'.";
            throw BaseException(ostr);
          }

          Stream::Parser istr(static_cast<const char*>(prop.value()));
          FileBlockIndex control_block_index;
          istr >> control_block_index;
          control_block_ = next_layer_->get_write_block(control_block_index);
          first_free_block_index_ = read_next_index_(control_block_);
        }
      }
      else
      {
        Stream::Error ostr;
        ostr << "File already use other allocator with name '"
             << (const char*)allocator_name.value()
             << "'.";
        throw BaseException(ostr);
      }
    }
  }

  DefaultAllocatorLayer::~DefaultAllocatorLayer() noexcept
  {
    static const char* FUN = "DefaultAllocatorLayer::~DefaultAllocatorLayer()";

    try
    {
      // dump control blocks
      if(!allocate_blocks_.empty())
      {
        for(AllocatedBlockList::iterator it = allocate_blocks_.begin();
          it != allocate_blocks_.end(); ++it)
        {
          write_next_index_(it->block, first_free_block_index_);
        }
        first_free_block_index_ = allocate_blocks_.rbegin()->block_index;
      }

      write_next_index_(control_block_, first_free_block_index_);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << ex.what();
      std::cerr << ostr.str() << std::endl;
    }
  }

  ReadBlock_var
  DefaultAllocatorLayer::get_read_block(const FileBlockIndex& index)
    /*throw(BaseException, CorruptedRecord)*/
  {
    return next_layer_->get_read_block(index);
  }

  WriteBlock_var
  DefaultAllocatorLayer::get_write_block(const FileBlockIndex& index)
    /*throw(BaseException, CorruptedRecord)*/
  {
    return new WriteBlockImpl(this, next_layer_->get_write_block(index), index);
  }

  bool
  DefaultAllocatorLayer::get_property(
    const char* name, PropertyValue& property)
    /*throw(BaseException)*/
  {
    return next_layer_->get_property(name, property);
  }

  void
  DefaultAllocatorLayer::set_property(
    const char* name, const PropertyValue& property)
    /*throw(BaseException)*/
  {
    return next_layer_->set_property(name, property);
  }

  unsigned long
  DefaultAllocatorLayer::area_size() const
    noexcept
  {
    return next_layer_->area_size();
  }

  /* allocator specific methods */
  void
  DefaultAllocatorLayer::allocate(AllocatedBlock& alloc_result)
    /*throw(BaseException)*/
  {
    static const char* FUN = "DefaultAllocatorLayer::allocate()";

    try
    {
      FileBlockIndex result_index;
      WriteBlock_var result_block;

      {
        SyncPolicy::WriteGuard lock(lock_);

        if(!allocate_blocks_.empty())
        {
          result_block = allocate_blocks_.begin()->block;
          result_index = allocate_blocks_.begin()->block_index;
          allocate_blocks_.pop_front();
          --allocate_blocks_size_;
        }
        else if(first_free_block_index_ != 0)
        {
          // allocate by first_free_block_index_
          result_block = next_layer_->get_write_block(
            first_free_block_index_);
          result_index = first_free_block_index_;
          first_free_block_index_ = read_next_index_(result_block);
        }
        else // first_free_block_index_ == 0
        {
          // extend file & alloc last block
          FileBlockIndex max_index = next_layer_->max_block_index();
          FileBlockIndex ind = max_index + ALLOCATE_PORTION;

          result_block = next_layer_->get_write_block(ind);
          result_index = ind;

          // init sequence of free blocks
          for(--ind; ind > max_index; --ind)
          {
            AllocatedBlock alloc_block;
            alloc_block.block = next_layer_->get_write_block(ind);
            alloc_block.block_index = ind;
            allocate_blocks_.push_back(alloc_block);
            ++allocate_blocks_size_;
          }
        }
      }

      alloc_result.block_index = result_index;
      result_block->resize(0);
      alloc_result.block = new WriteBlockImpl(this, result_block, result_index);
    }
    catch(const BaseException& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't allocate block(first free block index = " <<
        first_free_block_index_ << "): " << ex.what();

      first_free_block_index_ = 0; // reset allocator

      throw BaseException(ostr);
    }
  }

  void
  DefaultAllocatorLayer::free_indexes_(
    std::set<FileBlockIndex>& indexes)
    /*throw(BaseException)*/
  {
    static const char* FUN = "DefaultAllocatorLayer::free_indexes_()";

    try
    {
      SyncPolicy::WriteGuard lock(lock_);

      FileBlockIndex cur_free_index = read_next_index_(control_block_);

      while(cur_free_index == 0)
      {
        indexes.insert(cur_free_index);

        ReadBlock_var block = next_layer_->get_write_block(cur_free_index);
        cur_free_index = read_next_index_(block);
      }
    }
    catch(const BaseException& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught BaseException: " << ex.what();
      throw BaseException(ostr);
    }
  }

  FileBlockIndex
  DefaultAllocatorLayer::read_next_index_(
    ReadBlock* read_block) /*throw(BaseException)*/
  {
    static const char* FUN = "DefaultAllocatorLayer::read_next_index_()";

    SizeType sz = read_block->size();

    if(sz != sizeof(FileBlockIndex))
    {
      Stream::Error ostr;
      ostr << FUN << ": Incorrect block size for index block: " << sz <<
        ", expected: " << sizeof(FileBlockIndex);
      throw BaseException(ostr);
    }

    FileBlockIndex read_index;
    if(!read_block->read(&read_index, sz))
    {
      throw BaseException("Can't read index in block.");
    }

    return read_index;
  }

  void
  DefaultAllocatorLayer::write_next_index_(
    WriteBlock* write_block, FileBlockIndex next_index)
    /*throw(BaseException)*/
  {
    write_block->write(&next_index, sizeof(next_index));
  }

  void
  DefaultAllocatorLayer::deallocate_(
    WriteBlock* write_block, FileBlockIndex index)
    /*throw(BaseException)*/
  {
    /* insert this block between control block
     * and first_free_block */
    SyncPolicy::WriteGuard lock(lock_);
    if(allocate_blocks_size_ < ALLOCATE_PORTION)
    {
      AllocatedBlock alloc_block;
      alloc_block.block_index = index;
      alloc_block.block = ReferenceCounting::add_ref(write_block);
      allocate_blocks_.push_back(alloc_block);
      ++allocate_blocks_size_;
    }
    else
    {
      write_next_index_(write_block, first_free_block_index_);
      first_free_block_index_ = index;
    }
  }

  void
  DefaultAllocatorLayer::print_(std::ostream& out) noexcept
  {
    FileBlockIndex first_free_index = 0;
    SizeType control_block_sz = 0;

    try
    {
      first_free_index = read_next_index_(control_block_);
    }
    catch(...)
    {}

    try
    {
      control_block_sz = control_block_->size();
    }
    catch(...)
    {}

    out << "first_free_index = " << first_free_index << std::endl <<
      "control-block-size = " << control_block_sz << std::endl;
  }
} /*PlainStorage*/


