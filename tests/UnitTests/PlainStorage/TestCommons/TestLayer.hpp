// @file PlainStorage/TestCommons/TestLayer.hpp

#ifndef COMMONS_TESTLAYER_HPP
#define COMMONS_TESTLAYER_HPP

#include <cassert>

#include <Generics/ArrayAutoPtr.hpp>
#include <ProfilingCommons/PlainStorage/BaseLayer.hpp>

namespace PlainStorage
{
  typedef u_int64_t TestIndex;

  struct TestIndexSerializer
  {
    static SizeType
    max_size() noexcept
    {
      return sizeof(TestIndex);
    };

    static SizeType
    size(const TestIndex& /*val*/) noexcept
    {
      return sizeof(TestIndex);
    }

    void
    save(const TestIndex& val, void* buf, SizeType /*sz*/) const noexcept
    {
      *(TestIndex*)buf = val;
    }

    SizeType
    load(const void* buf, SizeType sz, TestIndex& val) const
      /*throw(BaseException)*/
    {
      assert(sz == sizeof(TestIndex));
      val = *(const TestIndex*)buf;
      return sz;
    }
  };

  class TestWriteBlock :
    public virtual WriteBlock,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    TestWriteBlock(
      const TestIndex& index,
      SizeType size,
      SizeType avail_size,
      std::ostream* ostr,
      bool ref_count_print = false)
      noexcept
      : index_(index),
        size_(size),
        avail_size_(avail_size),
        buf_(avail_size),
        ostr_(ostr),
        ref_count_print_(ref_count_print)
    {}

    virtual
    ~TestWriteBlock() noexcept
    {
      if (ostr_ && ref_count_print_)
      {
        *ostr_ << "destroy #" << index_ << std::endl;
      }
    };

    virtual SizeType
    size() /*throw(BaseException)*/
    {
      return size_;
    }

    virtual SizeType
    read(void* buffer, SizeType buffer_size) /*throw(BaseException)*/
    {
      if (ostr_)
      {
        *ostr_ << "read from #" << index_ << " with size=" << buffer_size
               << std::endl;
      }

      ::memcpy(buffer, buf_.get(), buffer_size);

      return size_;
    }

    virtual void
    read(SizeType pos, void* buffer, SizeType size) /*throw(BaseException)*/
    {
      if (ostr_)
      {
        *ostr_
          << "read from #" << index_
          << " with pos=" << pos
          << ", size=" << size
          << std::endl;
      }

      ::memcpy(buffer, buf_.get() + pos, size);
    }

    virtual SizeType
    available_size() /*throw(BaseException)*/
    {
      return avail_size_;
    }

    virtual void
    write(const void* buffer, SizeType size) /*throw(BaseException)*/
    {
      assert(size <= avail_size_);

      if (ostr_)
      {
        *ostr_
          << "write into #" << index_
          << " with size=" << size
          << std::endl;
      }

      ::memcpy(buf_.get(), buffer, size);
    }

    virtual void
    write(SizeType pos, const void* buffer, SizeType size) /*throw(BaseException)*/
    {
      assert(pos + size <= avail_size_);

      if (ostr_)
      {
        *ostr_
          << "write into #" << index_
          << " with pos=" << pos
          << ", size=" << size
          << std::endl;
      }

      ::memcpy(buf_.get() + pos, buffer, size);
    }

    virtual void
    resize(SizeType size) /*throw(BaseException)*/
    {
      if (ostr_)
      {
        *ostr_
          << "resize #" << index_
          << " with size=" << size
          << std::endl;
      }

      size_ = size;
    }

    virtual void
    deallocate() /*throw(BaseException)*/
    {
      if (ostr_)
      {
        *ostr_
          << "deallocate #" << index_
          << std::endl;
      }
    }

    virtual void
    add_ref() const noexcept
    {
      if (ostr_ && ref_count_print_)
      {
        *ostr_ << "add_ref #" << index_ << std::endl;
      }

      ReferenceCounting::AtomicImpl::add_ref();
    }

    virtual void
    remove_ref() const noexcept
    {
      if (ostr_ && ref_count_print_)
      {
        *ostr_ << "remove_ref #" << index_ << std::endl;
      }

      ReferenceCounting::AtomicImpl::remove_ref();
    }

    TestIndex index_;
    SizeType size_;
    SizeType avail_size_;
    Generics::ArrayAutoPtr<char> buf_;
    std::ostream* ostr_;
    bool ref_count_print_;
  };

  typedef
    ReferenceCounting::SmartPtr<TestWriteBlock>
    TestWriteBlock_var;

  class TestWriteLayer: public ExWriteBaseLayer<TestIndex>
  {
    typedef std::map<TestIndex, WriteBlock_var> BlocksMap;
  public:
    typedef TestIndex BlockIndex;
    typedef std::map<std::string, PropertyValue> PropertyMap;

    TestWriteLayer(
      bool auto_resize = false,
      std::ostream* ostr = 0)
      noexcept
      : auto_resize_(auto_resize),
        ostr_(ostr)
    {}

    virtual
    ReadBlock_var
    get_read_block(const BlockIndex& index) /*throw(BaseException)*/
    {
      if (auto_resize_)
      {
        if (blocks_.find(index) == blocks_.end())
        {
          blocks_[index] =
            new TestWriteBlock(index, 0, max_block_size(), ostr_);
        }
      }

      return ReferenceCounting::add_ref(blocks_[index]);
    }

    virtual
    WriteBlock_var
    get_write_block(const BlockIndex& index) /*throw(BaseException)*/
    {
      if (auto_resize_)
      {
        if (blocks_.find(index) == blocks_.end())
        {
          blocks_[index] =
            new TestWriteBlock(index, 0, max_block_size(), ostr_);
        }
      }

      return ReferenceCounting::add_ref(blocks_[index]);
    }

    virtual
    unsigned long area_size() const noexcept
    {
      return 0;
    }

    virtual
    bool
    get_property(const char* name, PropertyValue& property)
      /*throw(BaseException)*/
    {
      PropertyMap::iterator it = props_.find(name);
      if (it == props_.end())
      {
        return false;
      }

      property = it->second;
      return true;
    }

    virtual
    void
    set_property(const char* name, const PropertyValue& property)
      /*throw(BaseException)*/
    {
      props_[name] = property;
    }

    virtual SizeType
    max_block_size() const noexcept
    {
      return 4 * 1024 - 4 * 2;
    }

    virtual TestIndex
    max_block_index() const noexcept
    {
      return blocks_.empty() ? 0 : blocks_.rbegin()->first;
    }

    void
    insert_block(const BlocksMap::value_type& value) /*throw(eh::Exception)*/
    {
      blocks_.insert(value);
    }

  private:
    virtual
    ~TestWriteLayer() noexcept
    {}

    BlocksMap blocks_;
    PropertyMap props_;
    bool auto_resize_;
    std::ostream* ostr_;
  };
  typedef ReferenceCounting::SmartPtr<TestWriteLayer> TestWriteLayer_var;

  struct TestBlockAllocator : public BlockAllocator<TestIndex>
  {
    TestBlockAllocator(
      TestWriteLayer* test_layer,
      std::ostream* ostr = 0)
      : test_layer_(test_layer),
        current_index_(0),
        ostr_(ostr)
    {}

    virtual SizeType
    max_block_size() const noexcept
    {
      return 16 * 1024;
    }

    virtual void
    allocate(AllocatedBlock& allocated_block) /*throw(BaseException)*/
    {
      allocated_block.block_index = ++current_index_;
      allocated_block.block =
        new TestWriteBlock(
          allocated_block.block_index,
          0,
          16 * 1024,
          ostr_);

      test_layer_->insert_block(
        std::make_pair(
          allocated_block.block_index,
          PlainStorage::WriteBlock_var(allocated_block.block)));

      if (ostr_)
      {
        *ostr_ << "allocated #" << allocated_block.block_index
          << std::endl;
      }
    }

    unsigned long
    allocated_size() const
    {
      return 16 * 1024 * current_index_;
    }

  private:
    virtual
    ~TestBlockAllocator() noexcept
    {}

    TestWriteLayer* test_layer_;
    TestIndex current_index_;
    std::ostream* ostr_;
  };
  typedef ReferenceCounting::SmartPtr<TestBlockAllocator>
    TestBlockAllocator_var;

  struct TestBlockSizeAllocator : public BlockSizeAllocator<TestIndex>
  {
    TestBlockSizeAllocator(
      TestWriteLayer* test_layer,
      std::ostream* ostr = 0)
      : test_layer_(test_layer),
        current_index_(0),
        ostr_(ostr)
    {}

    virtual SizeType
    max_block_size() const noexcept
    {
      return 4 * 1024;
    }

    virtual void
    allocate_size(
      SizeType size,
      AllocatedBlock& allocated_block) /*throw(BaseException)*/
    {
      if (ostr_)
      {
        *ostr_ << "allocate_size(): size = " << size << std::endl;
      }

      SizeType real_size = 2 * 4 * 1024 / 16;

      for (unsigned int i = 2; i < 16; ++i)
      {
        if (size > 4 * 1024 / i && size <= 2 * 4 * 1024 / i)
        {
          real_size = 2 * 4 * 1024 / i;
        }
      }

      allocated_block.block_index = ++current_index_;
      allocated_block.block =
        new TestWriteBlock(
          allocated_block.block_index,
          size,
          real_size,
          ostr_);

      test_layer_->insert_block(
        std::make_pair(
          allocated_block.block_index,
          PlainStorage::WriteBlock_var(allocated_block.block)));
    }

  private:
    virtual
    ~TestBlockSizeAllocator() noexcept
    {}

    TestWriteLayer* test_layer_;
    TestIndex current_index_;
    std::ostream* ostr_;
  };
  typedef ReferenceCounting::SmartPtr<TestBlockSizeAllocator>
    TestBlockSizeAllocator_var;
}

#endif // COMMONS_TESTLAYER_HPP
