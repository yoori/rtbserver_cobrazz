#include <Generics/Rand.hpp>
#include "LoadingProgressCallback.hpp"

namespace AdServer
{
namespace ProfilingCommons
{
  namespace
  {
    const unsigned long INDEX_HEAD_SIZE = 256;
    const unsigned long INDEX_RESERVED_HEAD_SIZE = INDEX_HEAD_SIZE - 8;
    const unsigned long BODY_HEAD_SIZE = 256;
    const unsigned long BODY_RESERVED_HEAD_SIZE = BODY_HEAD_SIZE - 8;
    const uint32_t RESERVED_KEY_SIZE = 0xFFFFFFFF;

    unsigned long body_header_size(unsigned long key_size)
    {
      return sizeof(uint32_t) + key_size + // key
        sizeof(uint32_t) + // operation
        sizeof(uint64_t) + // access time
        sizeof(uint32_t); // body size
    }
  }

  // ReadFileLevel<>::KeyIteratorImpl
  template<typename KeyType, typename KeySerializerType>
  ReadFileLevel<KeyType, KeySerializerType>::
  KeyIteratorImpl::KeyIteratorImpl(
    const ReadFileLevel<KeyType, KeySerializerType>* read_file_level)
    noexcept
    : read_file_level_(ReferenceCounting::add_ref(read_file_level)),
      profiles_it_(read_file_level->profiles_.end())
  {}

  template<typename KeyType, typename KeySerializerType>
  bool
  ReadFileLevel<KeyType, KeySerializerType>::
  KeyIteratorImpl::get_next(
    KeyType& key,
    ProfileOperation& operation,
    Generics::Time& access_time)
    noexcept
  {
    if(profiles_it_ == read_file_level_->profiles_.end())
    {
      profiles_it_ = read_file_level_->profiles_.begin();
    }
    else
    {
      ++profiles_it_;
    }

    if(profiles_it_ != read_file_level_->profiles_.end())
    {
      key = profiles_it_->key;
      operation = static_cast<ProfileOperation>(profiles_it_->operation);
      access_time = Generics::Time(profiles_it_->access_time);
      return true;
    }

    return false;
  }

  // ReadFileLevel<>::IteratorImpl
  template<typename KeyType, typename KeySerializerType>
  ReadFileLevel<KeyType, KeySerializerType>::
  IteratorImpl::IteratorImpl(
    const ReadFileLevel<KeyType, KeySerializerType>* read_file_level,
    unsigned long read_buffer_size,
    bool disable_caching,
    FileController* file_controller)
    /*throw(typename ReadBaseLevel<KeyType>::Exception)*/
  try
    : read_file_level_(ReferenceCounting::add_ref(read_file_level)),
      file_reader_(
        read_file_level_->body_file_name_.c_str(),
        read_buffer_size,
        disable_caching,
        file_controller
        ),
      inited_(false),
      profile_inited_(false),
      end_reached_(false)
  {}
  catch(const FileReader::Exception& ex)
  {
    Stream::Error ostr;
    ostr << "ReadFileLevel::IteratorImpl::IteratorImpl()"
      ": caught eh::Exception: " << ex.what();
    throw typename ReadBaseLevel<KeyType>::Exception(ostr);
  }

  template<typename KeyType, typename KeySerializerType>
  bool
  ReadFileLevel<KeyType, KeySerializerType>::
  IteratorImpl::get_next(
    KeyType& key,
    ProfileOperation& operation,
    Generics::Time& access_time)
    /*throw(typename ReadBaseLevel<KeyType>::Exception)*/
  {
    static const char* FUN = "ReadFileLevel<>::IteratorImpl::get_next()";
    //std::cout << "get_next: file_reader_.pos() = " << file_reader_.pos() << std::endl;

    assert(!end_reached_);

    try
    {
      if(!inited_)
      {
        assert(!profile_inited_); // don't do get_next after false get_next
        read_file_level_->skip_body_head_(file_reader_);
        inited_ = true;
      }
      else if(!profile_inited_)
      {
        read_file_level_->skip_body_profile_(file_reader_);
      }
  
      //std::cout << "get_next(2): file_reader_.pos() = " << file_reader_.pos() << std::endl;
  
      profile_inited_ = false;
      uint32_t access_time_int;
      const bool res = read_file_level_->read_body_key_(
        file_reader_,
        key_buf_,
        key,
        operation,
        access_time_int);
  
      if(!res)
      {
        end_reached_ = true;
      }
      else
      {
        access_time = Generics::Time(access_time_int);
      }

      return res;
    }
    catch (eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << ex.what();
      throw typename ReadBaseLevel<KeyType>::Exception(ostr);
    }
  }

  template<typename KeyType, typename KeySerializerType>
  Generics::ConstSmartMemBuf_var
  ReadFileLevel<KeyType, KeySerializerType>::
  IteratorImpl::get_profile()
    /*throw (typename ReadBaseLevel<KeyType>::Exception)*/
  {
    static const char* FUN = "ReadFileLevel<>::IteratorImpl::get_profile()";

    try
    {
      if(!profile_inited_)
      {
        profile_ = read_file_level_->read_body_profile_(
          file_reader_);
        profile_inited_ = true;
      }
  
      return profile_;
    }
    catch (eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception: " << ex.what();
      throw typename ReadBaseLevel<KeyType>::Exception(ostr);
    }
  }

  // ReadFileLevel<>
  template<typename KeyType, typename KeySerializerType>
  ReadFileLevel<KeyType, KeySerializerType>::ReadFileLevel(
    const char* index_file_name,
    const char* body_file_name,
    unsigned long read_buf_size,
    bool disable_caching_on_fetch,
    FileController* file_controller,
    LoadingProgressCallbackBase_var progress_checker_parent)
    /*throw(typename ReadBaseLevel<KeyType>::Exception)*/
    : key_serializer_(KeySerializerType()),
      disable_caching_on_fetch_(disable_caching_on_fetch),
      index_file_name_(index_file_name),
      body_file_name_(body_file_name),
      file_controller_(
        file_controller ? ReferenceCounting::add_ref(file_controller) :
        new PosixFileController()),
      size_(0),
      area_size_(0),
      merge_free_size_(0)
  {
    static const char* FUN = "ReadFileLevel<>::ReadFileLevel()";

    try
    {
      file_.reset(new RandomAccessFile(body_file_name, file_controller_));
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught eh::Exception on body file opening '" <<
        body_file_name << "': " << ex.what();
      throw typename ReadBaseLevel<KeyType>::Exception(ostr);
    }

    // read file
    unsigned long profiles_file_size = file_->size();

    uint32_t index_version;
    uint32_t index_uniq_id;
    unsigned long rec_index = 0;
    uint64_t last_profile_end = 0;

    try
    {
      // read index
      FileReader reader(
        index_file_name,
        read_buf_size,
        true, // don't cache index file
        file_controller_);

      LoadingProgressCallbackBase_var progress_checker;
      if (progress_checker_parent.in())
      {
        progress_checker =
          new LoadingProgressCallback(progress_checker_parent, reader.file_size());
      }
      else
      {
        progress_checker = new LoadingProgressCallbackBase();
      }

      // read head
      reader.read(&index_version, sizeof(index_version));
      reader.read(&index_uniq_id, sizeof(index_uniq_id));
      Generics::MemBuf reserved_head(INDEX_RESERVED_HEAD_SIZE);
      reader.read(reserved_head.data(), INDEX_RESERVED_HEAD_SIZE);

      Generics::MemBuf key_buf;
      unsigned long key_i = 0;
      unsigned long cur_pos = 0;
      ActualProfileRef prev_profile_ref;
      prev_profile_ref.pos = 0;

      while(true)
      {
        ActualProfileRef profile_ref;
        const uint32_t key_size = read_key_(reader, key_buf, profile_ref.key);

        if(!key_size)
        {
          break;
        }

        read_index_profile_ref_(profile_ref, reader);

        // check consistency
        const unsigned long size_hurdle = 
          body_header_size(key_size) + std::numeric_limits<uint32_t>::max();

        if (key_i && (profile_ref.pos - prev_profile_ref.pos) > size_hurdle)
        {
          Stream::Error ostr;
          ostr << FUN << ": corrupted index file: '" << index_file_name_ << 
            "' size_hurdle = " << size_hurdle << " profile_ref.pos = " <<
            profile_ref.pos << " prev_profile_ref.pos = " << prev_profile_ref.pos;
          throw typename ReadBaseLevel<KeyType>::Exception(ostr);
        }

        ++key_i;

        if(profile_ref.pos + profile_ref.size > profiles_file_size)
        {
          Stream::Error ostr;
          ostr << FUN << ": invalid offset in index file:"
            "pos(" << profile_ref.pos <<
            ") + size(" << profile_ref.size <<
            ") > file size = " <<
            profiles_file_size;
          throw typename ReadBaseLevel<KeyType>::Exception(ostr);
        }

        if(profile_ref.pos + profile_ref.size > last_profile_end)
        {
          last_profile_end = profile_ref.pos + profile_ref.size;
        }

        area_size_ += profile_ref.area_size();
        profiles_.push_back(profile_ref);
        ++rec_index;
        progress_checker->post_progress(reader.pos() - cur_pos);
        cur_pos = reader.pos();

        prev_profile_ref = profile_ref;
      }

      // read number of records
      uint64_t control_rec_num;
      reader.read(&control_rec_num, sizeof(control_rec_num));
      if(rec_index != control_rec_num)
      {
        Stream::Error ostr;
        ostr << FUN << ": invalid 'number of records' in index file: " <<
          control_rec_num << " instead " << rec_index;
        throw typename ReadBaseLevel<KeyType>::Exception(ostr);
      }

      if(!reader.eof())
      {
        Stream::Error ostr;
        ostr << FUN << ": index file '" <<
          index_file_name << "' incorrectly closed, pos = " <<
          reader.pos() << ", found keys = " << key_i;
        throw typename ReadBaseLevel<KeyType>::Exception(ostr);
      }

      progress_checker->loading_is_finished();

      // read body file head
      uint32_t body_version;
      uint32_t body_uniq_id;
      file_->pread(&body_uniq_id, sizeof(body_uniq_id), sizeof(body_version));
      if(body_uniq_id != index_uniq_id)
      {
        Stream::Error ostr;
        ostr << FUN << ": index file identifier = " <<
          index_uniq_id <<
          " ins't equal to body file identifier = " <<
          body_uniq_id;
        throw typename ReadBaseLevel<KeyType>::Exception(ostr);
      }
  
      if(profiles_file_size != last_profile_end +
           sizeof(uint32_t) + // zero key
           sizeof(uint64_t) // number of records
         )
      {
        Stream::Error ostr;
        ostr << FUN << ": incorrect size of body file = " <<
          profiles_file_size <<
          ", expected = " <<
          (last_profile_end + sizeof(uint32_t) + sizeof(uint64_t));
        throw typename ReadBaseLevel<KeyType>::Exception(ostr);
      }
  
      uint64_t body_rec_num;
      file_->pread(
        &body_rec_num,
        sizeof(body_rec_num),
        last_profile_end + sizeof(uint32_t));
      if(body_rec_num != rec_index)
      {
        Stream::Error ostr;
        ostr << FUN << ": number of records in index file = " <<
          rec_index <<
          " ins't equal to number of records in body file = " <<
          body_rec_num;
        throw typename ReadBaseLevel<KeyType>::Exception(ostr);
      }
    }
    catch (typename ReadBaseLevel<KeyType>::Exception& ex)
    {
      throw;
    }
    catch (eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": index file '" <<
        index_file_name << "' caught eh::Exception: " <<
        ex.what();
      throw typename ReadBaseLevel<KeyType>::Exception(ostr);
    }
  }

  template<typename KeyType, typename KeySerializerType>
  ReadFileLevel<KeyType, KeySerializerType>::ReadFileLevel(
    const char* index_file_name,
    const char* body_file_name,
    typename ReadBaseLevel<KeyType>::Iterator* it,
    unsigned long write_buf_size,
    bool disable_caching_on_fetch,
    volatile sig_atomic_t* interrupter,
    FileController* file_controller)
    /*throw(Interrupted, typename ReadBaseLevel<KeyType>::Exception)*/
    : key_serializer_(KeySerializerType()),
      disable_caching_on_fetch_(disable_caching_on_fetch),
      index_file_name_(index_file_name),
      body_file_name_(body_file_name),
      file_controller_(
        file_controller ? ReferenceCounting::add_ref(file_controller) :
        new PosixFileController()),
      size_(0),
      area_size_(0),
      merge_free_size_(0)
  {
    static const char* FUN = "ReadFileLevel<>::ReadFileLevel()";

    assert(it);

    uint32_t version = 1;
    uint32_t uniq_id = Generics::safe_rand();
    Generics::MemBuf key_buf;
    bool min_access_time_inited = false;

    try
    {
      FileWriter index_writer(
        index_file_name,
        write_buf_size,
        false, // rewrite file
        true, // never cache index file it used only at storage opening
        file_controller_);
      FileWriter body_writer(
        body_file_name,
        write_buf_size,
        false, // rewrite file
        disable_caching_on_fetch_,
        file_controller_);

      try
      {
        // write index file head
        if(index_writer.size() != 0)
        {
          Stream::Error ostr;
          ostr << FUN << ": index file '" << index_file_name <<
            "' already exists and have non zero size";
          throw typename ReadBaseLevel<KeyType>::Exception(ostr);
        }

        index_writer.write(&version, sizeof(version));
        index_writer.write(&uniq_id, sizeof(uniq_id));

        {
          Generics::MemBuf reserved_head(INDEX_RESERVED_HEAD_SIZE);
          ::memset(reserved_head.data(), 0, INDEX_RESERVED_HEAD_SIZE);
          index_writer.write(reserved_head.data(), INDEX_RESERVED_HEAD_SIZE);
        }

        // write body file head
        if(body_writer.size() != 0)
        {
          Stream::Error ostr;
          ostr << FUN << ": data file '" << body_file_name <<
            "' already exists and have non zero size";
          throw typename ReadBaseLevel<KeyType>::Exception(ostr);
        }

        body_writer.write(&version, sizeof(version));
        body_writer.write(&uniq_id, sizeof(uniq_id));

        {
          Generics::MemBuf reserved_head(BODY_RESERVED_HEAD_SIZE);
          ::memset(reserved_head.data(), 0, BODY_RESERVED_HEAD_SIZE);
          body_writer.write(reserved_head.data(), BODY_RESERVED_HEAD_SIZE);
        }

        // fill index, body files
        unsigned long body_pos = BODY_HEAD_SIZE;

        ActualProfileRef profile_ref;
        uint64_t rec_num = 0;

        Generics::Time access_time;

        ProfileOperation profile_ref_operation;

        while(it->get_next(
          profile_ref.key, profile_ref_operation, access_time))
        {
          profile_ref.operation = profile_ref_operation;

          if(interrupter && *interrupter)
          {
            throw Interrupted("");
          }

          if(!min_access_time_inited)
          {
            min_access_time_ = access_time;
          }
          else
          {
            min_access_time_ = std::min(min_access_time_, access_time);
          }

          ++rec_num;

          Generics::ConstSmartMemBuf_var mem_buf = it->get_profile();
          assert(mem_buf.in() || profile_ref.operation == PO_ERASE);

          const uint32_t body_size = profile_ref.operation == PO_ERASE ?
            0 : mem_buf->membuf().size();

          {
            // write into index file
            const unsigned long key_size =
              write_key_(index_writer, key_buf, profile_ref.key);

            body_pos += body_header_size(key_size);

            //std::cerr << "body_pos = " << body_pos << std::endl;

            profile_ref.pos = body_pos;
            profile_ref.size = body_size;
            write_index_profile_ref_(index_writer, profile_ref);

            body_pos += (profile_ref.operation != PO_ERASE ?
              mem_buf->membuf().size() :
              0);
          }

          {
            // write to body file
            write_key_(body_writer, key_buf, profile_ref.key);
            const uint32_t operation = profile_ref.operation;
            body_writer.write(&operation, sizeof(operation));
            const uint64_t save_access_time = access_time.tv_sec;
            body_writer.write(&save_access_time, sizeof(save_access_time));

            body_writer.write(&body_size, sizeof(body_size));

            if(profile_ref.operation != PO_ERASE)
            {
              profile_ref.pos = body_writer.size();
              profile_ref.size = mem_buf->membuf().size();
              body_writer.write(
                mem_buf->membuf().data(),
                mem_buf->membuf().size());
            }
            else
            {
              profile_ref.pos = 0;
              profile_ref.size = 0;
            }

            area_size_ += profile_ref.area_size();
            profiles_.push_back(profile_ref);
          }
        }

        if(interrupter && *interrupter)
        {
          throw Interrupted("");
        }

        // write fin key size
        // close index file
        write_null_key_(index_writer);
        index_writer.write(&rec_num, sizeof(rec_num));
        index_writer.close();

        // close body file
        write_null_key_(body_writer);
        body_writer.write(&rec_num, sizeof(rec_num));
        body_writer.close();

        try
        {
          file_.reset(new RandomAccessFile(body_file_name, file_controller_));
        }
        catch(const eh::Exception& ex)
        {
          Stream::Error ostr;
          ostr << FUN << ": caught eh::Exception on body file opening '" <<
            body_file_name << "' after creation: " << ex.what();
          throw typename ReadBaseLevel<KeyType>::Exception(ostr);
        }
      }
      catch(...)
      {
        index_writer.close();
        body_writer.close();
        ::unlink(index_file_name);
        ::unlink(body_file_name);
        throw;
      }
    }
    catch(const FileWriter::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught FileWriter::Exception: " << ex.what();
      throw typename ReadBaseLevel<KeyType>::Exception(ostr);
    }
  }

  template<typename KeyType, typename KeySerializerType>
  CheckProfileResult
  ReadFileLevel<KeyType, KeySerializerType>::check_profile(
    const KeyType& key)
    const /*throw(typename ReadBaseLevel<KeyType>::Exception)*/
  {
    CheckProfileResult result;

    const typename ProfileRefs::const_iterator it = 
      std::lower_bound(profiles_.begin(), profiles_.end(), 
        key, ProfileRefsComparator());

    if(it != profiles_.end() && it->key == key)
    {
      result.operation = static_cast<ProfileOperation>(it->operation);
      result.size = it->size;
    }
    else
    {
      result.operation = PO_NOT_FOUND;
      result.size = 0;
    }

    return result;
  }

  template<typename KeyType, typename KeySerializerType>
  GetProfileResult
  ReadFileLevel<KeyType, KeySerializerType>::get_profile(
    const KeyType& key)
    const /*throw(typename ReadBaseLevel<KeyType>::Exception)*/
  {
    GetProfileResult result;
    const typename ProfileRefs::const_iterator it = 
      std::lower_bound(profiles_.begin(), profiles_.end(), 
        key, ProfileRefsComparator());

    if(it != profiles_.end() && it->key == key)
    {
      result.operation = static_cast<ProfileOperation>(it->operation);

      if(it->operation != PO_ERASE)
      {
        // read buffer        
        Generics::SmartMemBuf_var mem_buf(
          new Generics::SmartMemBuf(it->size));
        file_->pread(
          mem_buf->membuf().data(),
          it->size,
          it->pos);
        result.mem_buf = Generics::transfer_membuf(mem_buf);
        result.access_time = Generics::Time(it->access_time);
      }
    }

    return result;
  }

  template<typename KeyType, typename KeySerializerType>
  typename ReadBaseLevel<KeyType>::KeyIterator_var
  ReadFileLevel<KeyType, KeySerializerType>::get_key_iterator()
    const noexcept
  {
    return new KeyIteratorImpl(this);
  }

  template<typename KeyType, typename KeySerializerType>
  typename ReadBaseLevel<KeyType>::Iterator_var
  ReadFileLevel<KeyType, KeySerializerType>::get_iterator(
    unsigned long read_buffer_size) const
    /*throw(typename ReadBaseLevel<KeyType>::Exception)*/
  {
    return new IteratorImpl(
      this,
      read_buffer_size,
      disable_caching_on_fetch_,
      file_controller_);
  }

  template<typename KeyType, typename KeySerializerType>
  unsigned long
  ReadFileLevel<KeyType, KeySerializerType>::size() const noexcept
  {
    return size_;
  }

  template<typename KeyType, typename KeySerializerType>
  unsigned long
  ReadFileLevel<KeyType, KeySerializerType>::area_size()
    const noexcept
  {
    return area_size_;
  }

  template<typename KeyType, typename KeySerializerType>
  unsigned long
  ReadFileLevel<KeyType, KeySerializerType>::merge_free_size()
    const noexcept
  {
    return merge_free_size_;
  }

  template<typename KeyType, typename KeySerializerType>
  Generics::Time
  ReadFileLevel<KeyType, KeySerializerType>::min_access_time()
    const noexcept
  {
    return min_access_time_;
  }

  template<typename KeyType, typename KeySerializerType>
  void
  ReadFileLevel<KeyType, KeySerializerType>::rename_files(
    const char* new_index_file_name,
    const char* new_body_file_name)
    /*throw(typename ReadBaseLevel<KeyType>::Exception)*/
  {
    static const char* FUN = "ReadFileLevel<>::rename_files()";

    file_.reset(0);
    if(::rename(index_file_name_.c_str(), new_index_file_name) < 0)
    {
      Stream::Error ostr;
      ostr << FUN << ": can't rename '" << index_file_name_ <<
        "' to '" << new_index_file_name;
      eh::throw_errno_exception<
        typename ReadBaseLevel<KeyType>::Exception>(ostr.str());
    }

    if(::rename(body_file_name_.c_str(), new_body_file_name) < 0)
    {
      Stream::Error ostr;
      ostr << FUN << ": can't rename '" << body_file_name_ <<
        "' to '" << new_body_file_name;
      eh::throw_errno_exception<
        typename ReadBaseLevel<KeyType>::Exception>(ostr.str());
    }

    file_.reset(new RandomAccessFile(new_body_file_name, file_controller_));

    index_file_name_ = new_index_file_name;
    body_file_name_ = new_body_file_name;
  }

  template<typename KeyType, typename KeySerializerType>
  void
  ReadFileLevel<KeyType, KeySerializerType>::read_index_profile_ref_(
    ActualProfileRef& profile_ref,
    FileReader& reader) const 
    /*throw (FileReader::Exception)*/
  {
    uint32_t operation;
    uint64_t pos;
    uint32_t size;
    uint64_t access_time;
    reader.read(&operation, sizeof(operation));
    reader.read(&pos, sizeof(pos));
    reader.read(&size, sizeof(size));
    reader.read(&access_time, sizeof(access_time));
    profile_ref.operation = static_cast<ProfileOperation>(operation);
    profile_ref.pos = pos;
    profile_ref.size = size;
    profile_ref.access_time = access_time;
  }

  template<typename KeyType, typename KeySerializerType>
  void
  ReadFileLevel<KeyType, KeySerializerType>::write_index_profile_ref_(
    FileWriter& writer,
    const ActualProfileRef& profile_ref)
    const /*throw(FileWriter::Exception)*/
  {
    uint32_t operation = profile_ref.operation;
    uint64_t pos = profile_ref.pos;
    uint32_t size = profile_ref.size;
    uint64_t access_time = profile_ref.access_time;
    writer.write(&operation, sizeof(operation));
    writer.write(&pos, sizeof(pos));
    writer.write(&size, sizeof(size));
    writer.write(&access_time, sizeof(access_time));
  }

  template<typename KeyType, typename KeySerializerType>
  unsigned long
  ReadFileLevel<KeyType, KeySerializerType>::write_key_(
    FileWriter& writer,
    Generics::MemBuf& key_buf,
    const KeyType& key)
    const /*throw(
      eh::Exception,
      FileWriter::Exception,
      typename ReadBaseLevel<KeyType>::Exception)*/
  {
    static const char* FUN = "ReadFileLevel<>::write_key_()";

    unsigned long key_size = key_serializer_.size(key);
    if(key_size >= RESERVED_KEY_SIZE)
    {
      Stream::Error ostr;
      ostr << FUN << ": very big key size: " << key_size;
      throw typename ReadBaseLevel<KeyType>::Exception(ostr);
    }

    if(key_size > key_buf.capacity())
    {
      key_buf.alloc(2*key_size);
    }
    else if(key_size > key_buf.size())
    {
      key_buf.resize(key_size);
    }
    uint32_t key_size_for_save = key_size;
    key_serializer_.write(key_buf.data(), key_size, key);
    writer.write(&key_size_for_save, sizeof(key_size_for_save));
    writer.write(key_buf.data(), key_size);
    return key_size;
  }

  template<typename KeyType, typename KeySerializerType>
  void
  ReadFileLevel<KeyType, KeySerializerType>::write_null_key_(
    FileWriter& writer)
    const /*throw(FileWriter::Exception)*/
  {
    writer.write(&RESERVED_KEY_SIZE, sizeof(RESERVED_KEY_SIZE));
  }

  template<typename KeyType, typename KeySerializerType>
  uint32_t
  ReadFileLevel<KeyType, KeySerializerType>::read_key_(
    FileReader& reader,
    Generics::MemBuf& key_buf,
    KeyType& key)
    const /*throw(
      eh::Exception,
      FileReader::Exception,
      typename ReadBaseLevel<KeyType>::Exception)*/
  {
    static const char* FUN = "ReadFileLevel<>::read_key_()";

    uint32_t key_size;
    reader.read(&key_size, sizeof(uint32_t));
    //std::cout << "read_key_: key_size = " << key_size << std::endl;

    if(key_size == RESERVED_KEY_SIZE)
    {
      return 0;
    }

    if(key_size > key_buf.capacity())
    {
      key_buf.alloc(2*key_size);
    }
    else if(key_size > key_buf.size())
    {
      key_buf.resize(key_size);
    }

    reader.read(key_buf.data(), key_size);

    try
    {
      key_serializer_.read(key, key_buf.data(), key_size);
    }
    catch(const eh::Exception& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": can't read key from buffer with size = " <<
        key_size << ": " << ex.what();
      throw typename ReadBaseLevel<KeyType>::Exception(ostr);
    }

    return key_size;
  }

  template<typename KeyType, typename KeySerializerType>
  void
  ReadFileLevel<KeyType, KeySerializerType>::read_body_head_(
    FileReader& reader,
    unsigned long& version,
    unsigned long& id)
    /*throw(FileReader::Exception)*/
  {
    uint32_t res_version;
    uint32_t res_id;
    reader.read(&res_version, sizeof(res_version));
    reader.read(&res_id, sizeof(res_id));
    version = res_version;
    id = res_id;
    reader.skip(BODY_RESERVED_HEAD_SIZE);
  }

  template<typename KeyType, typename KeySerializerType>
  void
  ReadFileLevel<KeyType, KeySerializerType>::skip_body_head_(
    FileReader& reader)
    /*throw(FileReader::Exception)*/
  {
    reader.skip(
      sizeof(uint32_t) + // version
      sizeof(uint32_t) + // id
      BODY_RESERVED_HEAD_SIZE);
  }

  template<typename KeyType, typename KeySerializerType>
  bool
  ReadFileLevel<KeyType, KeySerializerType>::read_body_key_(
    FileReader& reader,
    Generics::MemBuf& key_buf,
    KeyType& key,
    ProfileOperation& operation,
    uint32_t& access_time)
    const /*throw(
      eh::Exception,
      FileReader::Exception,
      typename ReadBaseLevel<KeyType>::Exception)*/
  {
    //std::cout << "read_body_key_: key pos = " << reader.pos() << std::endl;

    if(!read_key_(reader, key_buf, key))
    {
      return false;
    }

    uint32_t res_operation;
    reader.read(&res_operation, sizeof(res_operation));
    operation = static_cast<ProfileOperation>(res_operation);
    uint64_t res_access_time;
    reader.read(&res_access_time, sizeof(res_access_time));
    access_time = res_access_time;

    //std::cout << "read_body_key_: key = '" << key << "'" << std::endl;
    return true;
  }

  template<typename KeyType, typename KeySerializerType>
  Generics::ConstSmartMemBuf_var
  ReadFileLevel<KeyType, KeySerializerType>::read_body_profile_(
    FileReader& reader)
    const /*throw(FileReader::Exception, eh::Exception)*/
  {
    uint32_t size;
    reader.read(&size, sizeof(size));
    Generics::SmartMemBuf_var mem_buf(new Generics::SmartMemBuf(size));
    reader.read(mem_buf->membuf().data(), size);
    return Generics::transfer_membuf(mem_buf);
  }

  template<typename KeyType, typename KeySerializerType>
  void
  ReadFileLevel<KeyType, KeySerializerType>::skip_body_profile_(
    FileReader& reader)
    const /*throw(FileReader::Exception)*/
  {
    uint32_t size;
    reader.read(&size, sizeof(size));
    reader.skip(size);
  }
}
}
