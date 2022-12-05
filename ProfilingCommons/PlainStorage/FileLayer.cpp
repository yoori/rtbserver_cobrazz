// @file PlainStorage/FileLayer.cpp
#include "FileLayer.hpp"

#include <fstream>
#include <iostream>
#include <map>

#include <Generics/ArrayAutoPtr.hpp>
#include <Generics/Time.hpp>

namespace
{
  const unsigned long PROPERTY_BLOCK_INDEX = 0;
  const bool TRACE_FILE_LAYER = false;
  const bool TRACE_FILE_LAYER_CONTENT = false;
  const char CREATION_TIME_PROPERTY[] = "FileLayer::CreateTime";
}

namespace
{
  void print_hex(std::ostream& out, unsigned char n)
  {
    {
      int ch = n / 16 % 16;
      if(ch < 10) out << (char)('0' + ch);
      else out << (char)('A' + ch - 10);
    }

    {
      int ch = n % 16;
      if(ch < 10) out << (char)('0' + ch);
      else out << (char)('A' + ch - 10);
    }
  }

  void print_hex(std::ostream& out, const void* buf, unsigned long n)
  {
    for(unsigned long i = 0; i < n; ++i)
    {
      out << " 0x";
      print_hex(out, *((const unsigned char*)buf + i));
    }
    out << " ";
  }
}

namespace PlainStorage
{
  /**
   * ReadFileLayer::PropertyReader
   */
  class ReadFileLayer::PropertyReader
  {
  public:
    typedef std::map<std::string, PropertyValue> PropertyMap;

  public:
    /**
     * Constructor cached properties in std::map from raw format
     * @param read_block The data block contains properties
     */
    PropertyReader(ReadBlock* read_block)
      : property_read_block_(ReferenceCounting::add_ref(read_block))
    {
      read_properties_();
    }

    /**
     * @return false if unknown properties requested
     */
    bool
    get_property(const char* name, PropertyValue& property)
    {
      PropertyMap::const_iterator it = property_cache_.find(name);
      if(it == property_cache_.end())
      {
        return false;
      }
      property = it->second;
      return true;
    }

  protected:
    /**
     * PROPERTIES block
     * unsigned long : size of block
     * uint32_t      : name_length
     * uint32_t      : value_length
     * name_length   : name text
     * value_length  : value text
     * ... repeat until run out of the block size ...
     */
    void read_properties_()
    {
      SizeType size = property_read_block_->size();
      Generics::ArrayAutoPtr<char> buffer(size);
      property_read_block_->read(buffer.get(), size);
      const char* cursor = buffer.get();

      while(cursor < buffer.get() + size)
      {
        std::string property_name;
        PropertyValue property_value;

        u_int32_t pn_size = *(u_int32_t*)cursor;
        u_int32_t pv_size = *((u_int32_t*)cursor + 1);

        cursor += 2*sizeof(u_int32_t);

        property_name.assign(cursor, pn_size);
        property_value.value(cursor + pn_size, pv_size);
        cursor += pn_size + pv_size;
        property_cache_.insert(std::make_pair(property_name, property_value));
      }
    }

  protected:
    ReadBlock_var property_read_block_;
    /// Cached properties. Mapping name -> value
    PropertyMap property_cache_;
  };

  /**
   * WriteFileLayer::PropertyWriter
   */
  class WriteFileLayer::PropertyWriter: public ReadFileLayer::PropertyReader
  {
  public:
    PropertyWriter(WriteBlock* head_block)
      : PropertyReader(head_block),
        property_write_block_(ReferenceCounting::add_ref(head_block))
    {}

    /**
     * Update cache and immediately put all properties to storage
     */
    void
    set_property(const char* name, const PropertyValue& property)
    {
      property_cache_[name] = property;
      write_properties_();
    }

  protected:
    /**
     * Full write all cached properties
     */
    void write_properties_()
    {
      SizeType size_count = 0;

      /* count need size */
      for(PropertyMap::const_iterator it = property_cache_.begin();
          it != property_cache_.end(); ++it)
      {
        size_count += 2*sizeof(u_int32_t);
        size_count += it->first.length();
        size_count += it->second.size();
      }

      Generics::ArrayAutoPtr<char> buf(size_count);
      char* cursor = buf.get();

      for(PropertyMap::const_iterator it = property_cache_.begin();
          it != property_cache_.end(); ++it)
      {
        *(u_int32_t*)cursor = it->first.length();
        cursor += sizeof(u_int32_t);
        *(u_int32_t*)cursor = it->second.size();
        cursor += sizeof(u_int32_t);

        unsigned long name_len = it->first.length();
        ::memcpy(cursor, it->first.c_str(), name_len);
        cursor += name_len;
        ::memcpy(cursor, it->second.value(), it->second.size());
        cursor += it->second.size();
      }

      property_write_block_->write(buf.get(), size_count);
    }

  protected:
    WriteBlock_var property_write_block_;
  };

  /**
   * ReadBlockImpl
   */
  class ReadFileLayer::ReadBlockImpl:
    public virtual ReadBlock,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    ReadBlockImpl(
      ReadFileLayer* read_file_layer,
      const FileBlockIndex& block_index,
      const void* content) /*throw(BaseException)*/;

    virtual ~ReadBlockImpl() noexcept;

    virtual SizeType size() /*throw(BaseException)*/;

    virtual SizeType
    read(void* buffer, SizeType size) /*throw(BaseException)*/;

    virtual void
    read(SizeType pos, void* buffer, SizeType size) /*throw(BaseException)*/;

  protected:
    ReadFileLayer* read_file_layer_;
    FileBlockIndex block_index_;
    const void* content_;
  };

  /**
   * WriteBlockImpl
   */
  class WriteFileLayer::WriteBlockImpl:
    public virtual WriteBlock,
    public virtual ReferenceCounting::AtomicImpl
  {
  public:
    WriteBlockImpl(
      WriteFileLayer* write_file_layer,
      const FileBlockIndex& block_index,
      void* content,
      bool init = false)
      /*throw(BaseException)*/;

    virtual ~WriteBlockImpl() noexcept;

    virtual bool remove_ref_no_delete_() const noexcept;

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

    void print_(std::ostream& out, SizeType byte_count) noexcept;

  protected:
    WriteFileLayer* write_file_layer_;
    /// The index of Data block
    FileBlockIndex block_index_;
    /// The pointer to begin of Data block
    void* content_;
  };

  /**
   * ReadBlockImpl implementation
   */
  ReadFileLayer::ReadBlockImpl::ReadBlockImpl(
    ReadFileLayer* read_file_layer,
    const FileBlockIndex& block_index,
    const void* content)
    /*throw(BaseException)*/
    : read_file_layer_(read_file_layer),
      block_index_(block_index),
      content_(content)
  {}

  ReadFileLayer::ReadBlockImpl::~ReadBlockImpl() noexcept
  {
    static const char* FUN = "ReadFileLayer::ReadBlockImpl::~ReadBlockImpl()";
    
    try
    {
      read_file_layer_->read_unresolve_block_(block_index_, content_);
    }
    catch (const eh::Exception& ex)
    {
      std::cerr << FUN << ": " << ex.what() << std::endl;
    }
  }

  SizeType
  ReadFileLayer::ReadBlockImpl::size() /*throw(BaseException)*/
  {
    return *(const u_int64_t*)content_;
  }

  SizeType
  ReadFileLayer::ReadBlockImpl::read(
    void* buffer, SizeType buffer_size) /*throw(BaseException)*/
  {
    static const char* FUN = "ReadFileLayer::ReadBlockImpl::read()";

    if(TRACE_FILE_LAYER)
    {
      std::cout << FUN << ": (#" << block_index_ <<
        ") size = " << buffer_size << std::endl;
    }

    SizeType sz = size();

    if(sz > buffer_size)
    {
      return 0;
    }

    ::memcpy(buffer, (const u_int64_t*)content_ + 1, sz);

    if(TRACE_FILE_LAYER_CONTENT)
    {
      std::cout << FUN << ": content (";
      print_hex(std::cout, buffer, sz);
      std::cout << std::endl;
    }

    return sz;
  }

  void
  ReadFileLayer::ReadBlockImpl::read(
    SizeType pos, void* buffer, SizeType read_size)
    /*throw(BaseException)*/
  {
    static const char* FUN = "ReadFileLayer::ReadBlockImpl::read(pos, ...)";

    if(TRACE_FILE_LAYER)
    {
      std::cout << FUN << ": (#" << block_index_ <<
        ") pos = " << pos << ", size = " <<
        read_size << std::endl;
    }

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

    if(TRACE_FILE_LAYER_CONTENT)
    {
      std::cout << FUN << ": content (";
      print_hex(std::cout, buffer, read_size);
      std::cout << ")" << std::endl;
    }
  }

  /**
   * WriteFileLayer::WriteBlockImpl
   */
  WriteFileLayer::WriteBlockImpl::WriteBlockImpl(
    WriteFileLayer* write_file_layer,
    const FileBlockIndex& block_index,
    void* content,
    bool init)
    /*throw(BaseException)*/
    : write_file_layer_(write_file_layer),
      block_index_(block_index),
      content_(content)
  {
    if(init)
    {
      resize(0);
    }
  }

  WriteFileLayer::WriteBlockImpl::~WriteBlockImpl() noexcept
  {}

  bool
  WriteFileLayer::WriteBlockImpl::remove_ref_no_delete_() const noexcept
  {
    static const char* FUN =
      "WriteFileLayer::WriteBlockImpl::remove_ref_no_delete_()";

    bool to_remove = false;

    {
      WriteFileLayer::ResolveSyncPolicy::WriteGuard lock(
        write_file_layer_->opened_blocks_lock_);

      if(AtomicImpl::remove_ref_no_delete_())
      {
        try
        {
          write_file_layer_->close_write_block_i_(block_index_);
          to_remove = true;
        }
        catch (const eh::Exception& ex)
        {
          std::cerr << FUN << ": " << ex.what() << std::endl;
        }
      }
    }

    if(to_remove)
    {
      try
      {
        write_file_layer_->unresolve_block_content_(content_);
      }
      catch (const eh::Exception& ex)
      {
        std::cerr << FUN << ": " << ex.what() << std::endl;
      }

      return true;
    }

    return false;
  }

  SizeType
  WriteFileLayer::WriteBlockImpl::size() /*throw(BaseException)*/
  {
    return *static_cast<const u_int64_t*>(content_);
  }

  SizeType
  WriteFileLayer::WriteBlockImpl::read(
    void* buffer, SizeType buffer_size) /*throw(BaseException)*/
  {
    static const char* FUN = "WriteFileLayer::WriteBlockImpl::read()";

    if(TRACE_FILE_LAYER)
    {
      std::cout << FUN << ": (#" << block_index_ <<
        ") size = " << buffer_size << std::endl;
    }

    SizeType sz = size();

    if(sz > buffer_size)
    {
      return 0;
    }

    ::memcpy(buffer, (const u_int64_t*)content_ + 1, sz);

    if(TRACE_FILE_LAYER_CONTENT)
    {
      std::cout << FUN << ": content (";
      print_hex(std::cout, buffer, sz);
      std::cout << ")" << std::endl;
    }

    return sz;
  }

  void
  WriteFileLayer::WriteBlockImpl::read(
    SizeType pos, void* buffer, SizeType read_size)
    /*throw(BaseException)*/
  {
    static const char* FUN = "WriteFileLayer::WriteBlockImpl::read()";

    if(TRACE_FILE_LAYER)
    {
      std::cout << FUN << ": (#" << block_index_ <<
        ") pos = " << pos << ", size = " <<
        read_size << std::endl;
    }

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

    if(TRACE_FILE_LAYER_CONTENT)
    {
      std::cout << FUN << ": content (";
      print_hex(std::cout, buffer, read_size);
      std::cout << ")" << std::endl;
    }
  }

  void
  WriteFileLayer::WriteBlockImpl::resize(SizeType new_size) /*throw(BaseException)*/
  {
    static const char* FUN = "WriteFileLayer::WriteBlockImpl::resize()";

    if(TRACE_FILE_LAYER)
    {
      std::cout << FUN << ": (#" << block_index_ <<
        ") new_size = " << new_size << std::endl;
    }

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
  WriteFileLayer::WriteBlockImpl::available_size() /*throw(BaseException)*/
  {
    return write_file_layer_->block_data_size();
  }

  void
  WriteFileLayer::WriteBlockImpl::write(const void* buffer, SizeType size)
    /*throw(BaseException, CorruptedRecord)*/
  {
    static const char* FUN = "WriteFileLayer::WriteBlockImpl::write()";

    if(TRACE_FILE_LAYER)
    {
      std::cout << FUN << ": (#" << block_index_ <<
        ") size = " << size << std::endl;
    }

    if (size > write_file_layer_->block_data_size())
    {
      Stream::Error ostr;
      ostr << FUN << ": try write " << size <<
        " bytes to block with size=" << write_file_layer_->block_data_size();
      throw CorruptedRecord(ostr);
    }

    *static_cast<u_int64_t*>(content_) = size;
    ::memcpy((u_int64_t*)content_ + 1, buffer, size);

    if(TRACE_FILE_LAYER_CONTENT)
    {
      std::cout << FUN << ": content (";
      print_hex(std::cout, buffer, size);
      std::cout << ")" << std::endl;
    }
  }

  void
  WriteFileLayer::WriteBlockImpl::write(
    SizeType pos, const void* buffer, SizeType write_size)
    /*throw(BaseException, CorruptedRecord)*/
  {
    static const char* FUN = "WriteFileLayer::WriteBlockImpl::write(pos, ...)";

    if(TRACE_FILE_LAYER)
    {
      std::cout << FUN << ": #" << block_index_ <<
        ": pos = " << pos << ", write-size = " <<
        write_size << std::endl;
    }

    if (pos + write_size > write_file_layer_->block_data_size())
    {
      Stream::Error ostr;
      ostr << FUN << ": Overflow while writing, position=" <<
        pos << ", write size=" << write_size <<
        ", block data size=" << write_file_layer_->block_data_size();
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

    if(TRACE_FILE_LAYER_CONTENT)
    {
      std::cout << FUN << ": content (";
      print_hex(std::cout, buffer, write_size);
      std::cout << ")" << std::endl;
    }
  }

  void WriteFileLayer::WriteBlockImpl::deallocate() /*throw(BaseException)*/
  {
    throw Exception("Not Support");
  }

  void WriteFileLayer::WriteBlockImpl::print_(
    std::ostream& out, SizeType byte_count)
    noexcept
  {
    for(unsigned char* cur = (unsigned char*)content_;
        cur < (unsigned char*)content_ + byte_count; ++cur)
    {
      out << " ";
      print_hex(out, *cur);
    }
  }

  /**
   * ReadFileLayer
   */
  ReadFileLayer::ReadFileLayer(
    const char* file_name,
    unsigned long block_size) /*throw(Exception)*/
    : map_page_size_(0),
      block_size_(block_size),
      file_in_blocks_size_(0)
  {
    open_file_(file_name);
    property_reader_.reset(new PropertyReader(get_read_block(PROPERTY_BLOCK_INDEX)));
  }

  ReadFileLayer::~ReadFileLayer() noexcept
  {
    file_.close();
  }

  void
  ReadFileLayer::open_file_(const char* file_name) /*throw(Exception)*/
  {
    unsigned int system_page_size = Sys::File::get_page_size();

    file_.open(file_name, Sys::FO_READ);

    file_in_blocks_size_ = file_.size() / block_size_;

    // Calculate size of file part that portions will be read
    // block_size_ cannot be zero, rounding up block_size_ to a multiply
    // of SYSTEM_PAGE_SIZE_
    // todo: can change to faster expr: block_size_ - 1 + system_page_size -
    // (block_size_ - 1) % system_page_size;
    map_page_size_ = system_page_size *
      (block_size_ / system_page_size +
       ((block_size_ % system_page_size) ? 1 : 0));
  }

  const void*
  ReadFileLayer::read_resolve_block_(const FileBlockIndex& block_index)
    /*throw(Exception)*/
  {
    static const char* FUN = "ReadFileLayer::read_resolve_block_()";
    
    try
    {
      return file_.mmap(
        Sys::OffsetType(block_index)*map_page_size_,
        map_page_size_,
        Sys::MM_READ);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Can't map to memory file block: pos = " <<
        block_index*map_page_size_ <<
        ", size = " << map_page_size_ << ": " << ex.what();
      throw Exception(ostr);
    }
  }

  void
  ReadFileLayer::read_unresolve_block_(
    const FileBlockIndex& block_index,
    const void* mem_ptr) /*throw(Exception)*/
  {
    {
      SyncPolicy::WriteGuard lock(opened_blocks_lock_);

      ReadBlockHashTable::iterator it = opened_blocks_.find(block_index);

      if(it == opened_blocks_.end())
      {
        throw Exception(
          "Implementation critical exception: unresolving of "
          "not opened block");
      }

      opened_blocks_.erase(it);
    }

    /* unresolve content */
    file_.munmap(const_cast<void*>(mem_ptr), map_page_size_);
  }

  unsigned long
  ReadFileLayer::area_size() const noexcept
  {
    return file_in_blocks_size_ * block_size_;
  }

  ReadBlock_var
  ReadFileLayer::get_read_block(const FileBlockIndex& block_index)
    /*throw(BaseException)*/
  {
    /* search for opened blocks */
    SyncPolicy::WriteGuard lock(opened_blocks_lock_);
    ReadBlockHashTable::iterator it = opened_blocks_.find(block_index);
    if(it != opened_blocks_.end())
    {
      return ReferenceCounting::add_ref(it->second);
    }

    /* create new block */
    ReadBlockImpl* resolved_block =
      new ReadBlockImpl(this, block_index, read_resolve_block_(block_index));

    /* exception accuracy section */
    opened_blocks_.insert(std::make_pair(block_index, resolved_block));

    return ReadBlock_var(resolved_block);
  }

  bool
  ReadFileLayer::get_property(const char* name, PropertyValue& property)
    /*throw(BaseException)*/
  {
    return property_reader_->get_property(name, property);
  }

  SizeType
  ReadFileLayer::block_data_size() const noexcept
  {
    return block_size_ - sizeof(u_int64_t);
  }

  FileBlockIndex
  ReadFileLayer::max_block_index() const
    noexcept
  {
    return file_in_blocks_size_ - 1;
  }

  /**
   * WriteFileLayer
   */
  WriteFileLayer::WriteFileLayer(
    const char* filename,
    unsigned long block_size,
    OpenType open_type)
    /*throw(Exception)*/
    : map_page_size_(0),
      block_size_(block_size),
      file_in_blocks_size_(0)
  {
    open_file_(filename, open_type);
    property_writer_.reset(new PropertyWriter(get_write_block(PROPERTY_BLOCK_INDEX)));
    PropertyValue ct_val;
    if(!property_writer_->get_property(CREATION_TIME_PROPERTY, ct_val))
    {
      std::string ct = Generics::Time::get_time_of_day().gm_ft();
      ct_val.value(ct.c_str(), ct.size());
      property_writer_->set_property(CREATION_TIME_PROPERTY, ct_val);
    }
  }

  WriteFileLayer::~WriteFileLayer() noexcept
  {
    file_.close();
  }

  ReadBlock_var
  WriteFileLayer::get_read_block(const FileBlockIndex& block_index)
    /*throw(BaseException)*/
  {
    return get_write_block(block_index);
  }

  WriteBlock_var
  WriteFileLayer::get_write_block(const FileBlockIndex& block_index)
    /*throw(BaseException)*/
  {
    /* search for opened blocks */
    BlockResolveLockMap::WriteGuard block_lock(
      block_resolve_locks_.write_lock(block_index));

    {
      ResolveSyncPolicy::ReadGuard lock(opened_blocks_lock_);

      WriteBlockHashTable::iterator it = opened_blocks_.find(block_index);
      if(it != opened_blocks_.end())
      {
        return ReferenceCounting::add_ref(it->second);
      }
    }

    /* create new block */
    bool need_to_init;
    void* mem_ptr = write_resolve_block_(block_index, need_to_init);
    WriteBlockImpl* resolved_block =
      new WriteBlockImpl(this, block_index, mem_ptr, need_to_init);

    /* exception accuracy section */
    ResolveSyncPolicy::WriteGuard lock(opened_blocks_lock_);
    opened_blocks_.insert(std::make_pair(block_index, resolved_block));

    return WriteBlock_var(resolved_block);
  }

  bool
  WriteFileLayer::get_property(const char* name, PropertyValue& property)
    /*throw(BaseException)*/
  {
    return property_writer_->get_property(name, property);
  }

  void
  WriteFileLayer::set_property(const char* name, const PropertyValue& property)
    /*throw(BaseException)*/
  {
    property_writer_->set_property(name, property);
  }

  unsigned long
  WriteFileLayer::area_size() const noexcept
  {
    return static_cast<unsigned long>(file_in_blocks_size_) * block_size_;
  }

  SizeType
  WriteFileLayer::block_data_size() const noexcept
  {
    return block_size_ - 2*sizeof(u_int32_t);
  }

  FileBlockIndex
  WriteFileLayer::max_block_index() const noexcept
  {
    return file_in_blocks_size_;
  }

  void*
  WriteFileLayer::write_resolve_block_(
    const FileBlockIndex& block_index,
    bool& need_to_init)
    /*throw(Exception)*/
  {
    static const char* FUN = "WriteFileLayer::write_resolve_block_()";

    need_to_init = false;

    if(block_index >= max_block_index())
    {
      ResizeSyncPolicy::WriteGuard lock(resize_lock_);

      // recheck resize predicate
      if(block_index >= max_block_index())
      {
        resize_file_(block_index + 1);
        need_to_init = true;
      }
    }

    try
    {
      void* mem_ptr = file_.mmap(
        off_t(block_index)*map_page_size_,
        map_page_size_,
        Sys::MM_RW);

      return mem_ptr;
    }
    catch(const Sys::File::PosixException& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught Sys::File::PosixException: " << ex.what();
      throw Exception(ostr);
    }
  }

  void
  WriteFileLayer::close_write_block_i_(
    const FileBlockIndex& block_index) /*throw(Exception)*/
  {
    static const char* FUN = "WriteFileLayer::close_write_block_i_()";

    WriteBlockHashTable::iterator it = opened_blocks_.find(block_index);

    if(it == opened_blocks_.end())
    {
      Stream::Error ostr;
      ostr << FUN <<
        ": Implementation critical exception: unresolving of "
        "not opened block";

      throw Exception(ostr);
    }

    opened_blocks_.erase(it);
  }

  void
  WriteFileLayer::unresolve_block_content_(void* content) /*throw(Exception)*/
  {
    static const char* FUN = "WriteFileLayer::unresolve_block_content_()";

    /* unresolve content */
    try
    {
      file_.munmap(content, map_page_size_);
    }
    catch(const Sys::File::PosixException& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught Sys::File::PosixException at munmap: " << ex.what();
      throw Exception(ostr);
    }
  }

  void
  WriteFileLayer::open_file_(const char* filename, OpenType open_type)
    /*throw(Exception)*/
  {
    static const char* FUN = "WriteFileLayer::open_file_()";

    try
    {
      unsigned int system_page_size = Sys::File::get_page_size();

      if(open_type == OT_OPEN)
      {
        file_.open(filename, Sys::FO_RW);
      }
      else if(open_type == OT_OPEN_OR_CREATE)
      {
        file_.open(filename, Sys::FO_RW_CREATE);
      }
      else
      {
        throw Exception("Not defined open mode");
      }

      file_in_blocks_size_ = file_.size() / block_size_;

      map_page_size_ = system_page_size *
        (block_size_ / system_page_size +
         ((block_size_ % system_page_size) ? 1 : 0));
    }
    catch(const Sys::File::PosixException& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught Sys::File::PosixException at open: " << ex.what();
      throw Exception(ostr);
    }
  }

  void
  WriteFileLayer::resize_file_(const FileBlockIndex& new_size_in_blocks)
    /*throw(Exception)*/
  {
    static const char* FUN = "WriteFileLayer::resize_file_()";

    Sys::OffsetType max_off = std::numeric_limits<Sys::OffsetType>::max();

    if(max_off &&
       new_size_in_blocks > static_cast<FileBlockIndex>(max_off / block_size_))
    {
      Stream::Error ostr;
      ostr << FUN <<
        ": Can't resize file. "
        "Requested size less then maximum possible offset: "
        "requested size in blocks is " <<
        new_size_in_blocks << " < " << max_off / block_size_ << ".";
      throw Exception(ostr);
    }

    Sys::OffsetType new_size = Sys::OffsetType(new_size_in_blocks)*block_size_;
    Sys::OffsetType prev_size = Sys::OffsetType(max_block_index())*block_size_;

    try
    {
      file_.resize(new_size);
      file_.alloc(prev_size, new_size - prev_size);
    }
    catch(const Sys::File::PosixException& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": Caught PosixException: " << ex.what();
      throw Exception(ostr);
    }

    file_in_blocks_size_ = new_size_in_blocks;
  }
} // namespace PlainStorage
