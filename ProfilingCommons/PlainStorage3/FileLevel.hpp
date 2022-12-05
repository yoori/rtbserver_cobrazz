#ifndef FILELEVEL_HPP
#define FILELEVEL_HPP

#include <deque>

#include "BaseLevel.hpp"
#include "ReadMemLevel.hpp"
#include "RandomAccessFile.hpp"
#include "FileReader.hpp"
#include "FileWriter.hpp"
#include "LoadingProgressCallbackBase.hpp"

namespace AdServer
{
namespace ProfilingCommons
{
  template<typename KeyType, typename KeySerializerType>
  class ReadFileLevel:
    public ReadBaseLevel<KeyType>,
    public ReferenceCounting::AtomicImpl
  {
  public:
    DECLARE_EXCEPTION(Interrupted, typename ReadBaseLevel<KeyType>::Exception);

    class KeyIteratorImpl:
      public ReadBaseLevel<KeyType>::KeyIterator,
      public ReferenceCounting::AtomicImpl
    {
    public:
      KeyIteratorImpl(
        const ReadFileLevel<KeyType, KeySerializerType>* read_mem_level)
        noexcept;

      virtual bool
      get_next(
        KeyType& key,
        ProfileOperation& operation,
        Generics::Time& access_time)
        noexcept;

    private:
      const ReferenceCounting::ConstPtr<
        ReadFileLevel<KeyType, KeySerializerType> > read_file_level_;
      typename ReadFileLevel<KeyType, KeySerializerType>::
        ProfileRefs::const_iterator profiles_it_;
    };

    class IteratorImpl:
      public ReadBaseLevel<KeyType>::Iterator,
      public ReferenceCounting::AtomicImpl
    {
    public:
      IteratorImpl(
        const ReadFileLevel<KeyType, KeySerializerType>* read_mem_level,
        unsigned long read_buffer_size,
        bool disable_caching,
        FileController* file_controller)
        /*throw(typename ReadBaseLevel<KeyType>::Exception)*/;

      virtual bool
      get_next(
        KeyType& key,
        ProfileOperation& operation,
        Generics::Time& access_time)
        /*throw(typename ReadBaseLevel<KeyType>::Exception)*/;

      virtual Generics::ConstSmartMemBuf_var
      get_profile()
        /*throw(typename ReadBaseLevel<KeyType>::Exception)*/;

    private:
      const ReferenceCounting::ConstPtr<
        ReadFileLevel<KeyType, KeySerializerType> > read_file_level_;
      FileReader file_reader_;
      Generics::MemBuf key_buf_;
      bool inited_;
      bool profile_inited_;
      bool end_reached_;
      Generics::ConstSmartMemBuf_var profile_;
    };

  public:
    // load index from file
    ReadFileLevel(
      const char* index_file_name,
      const char* file_name,
      unsigned long read_buf_size,
      bool disable_caching_on_fetch,
      FileController* file_controller = nullptr,
      LoadingProgressCallbackBase_var progress_checker_parent = nullptr)
      /*throw(typename ReadBaseLevel<KeyType>::Exception)*/;

    // save data into file
    ReadFileLevel(
      const char* index_file_name,
      const char* file_name,
      typename ReadBaseLevel<KeyType>::Iterator* src_it,
      unsigned long write_buf_size,
      bool disable_caching_on_fetch,
      volatile sig_atomic_t* interrupter,
      FileController* file_controller = nullptr)
      /*throw(Interrupted, typename ReadBaseLevel<KeyType>::Exception)*/;

    virtual CheckProfileResult
    check_profile(const KeyType& key) const
      /*throw(typename ReadBaseLevel<KeyType>::Exception)*/;

    virtual GetProfileResult
    get_profile(const KeyType& key) const
      /*throw(typename ReadBaseLevel<KeyType>::Exception)*/;

    virtual typename ReadBaseLevel<KeyType>::KeyIterator_var
    get_key_iterator() const
      noexcept;

    virtual typename ReadBaseLevel<KeyType>::Iterator_var
    get_iterator(unsigned long read_buffer_size) const
      /*throw(typename ReadBaseLevel<KeyType>::Exception)*/;

    virtual unsigned long
    size() const
      noexcept;

    virtual unsigned long
    area_size() const
      noexcept;

    virtual unsigned long
    merge_free_size() const
      noexcept;

    virtual Generics::Time
    min_access_time() const
      noexcept;

    void
    rename_files(
      const char* new_index_file_name,
      const char* new_body_file_name)
      /*throw(typename ReadBaseLevel<KeyType>::Exception)*/;

  protected:
    // pos, size eval in runtime by result file size
    template<typename PosType>
    struct __attribute__ ((__packed__)) ProfileRefHolder
    {
      unsigned char operation; // ProfileOperation
      PosType pos;
      uint32_t size;
      // TODO: eval max size and pos in runtime at saving and keep in file head (
      //  in much cases can be used uint16_t or uint32_t)
      uint32_t access_time; // keep with seconds precision (short variant time < 2106 year)
      // TODO: for exclude 2106 limitation - keep offset relative to minimum time in file
    };

    template<typename PosType>
    struct ProfileRef: public ProfileRefHolder<PosType>
    {
      unsigned long area_size() const
      {
        return sizeof(*this);
      }

      KeyType key;
    };

    typedef ProfileRef<uint64_t> ActualProfileRef;

    class ProfileRefsComparator
    {
    public:
      template<typename ProfileRefType>
      bool
      operator() (
        const ProfileRefType& ref,
        const KeyType& key) const
        noexcept
      {
        return (ref.key < key);
      }

      template<typename ProfileRefType>
      bool
      operator() (
        const KeyType& key,
        const ProfileRefType& ref) const
        noexcept
      {
        return (key < ref.key);
      }
    };

    typedef std::deque<ActualProfileRef> ProfileRefs;

  protected:
    virtual ~ReadFileLevel() noexcept
    {}

    void
    read_index_profile_ref_(
      ActualProfileRef& profile_ref,
      FileReader& reader) const
      /*throw(FileReader::Exception)*/;

    void
    write_index_profile_ref_(
      FileWriter& writer,
      const ActualProfileRef& profile_ref) const
      /*throw(FileWriter::Exception)*/;

    static void
    read_body_head_(
      FileReader& reader,
      unsigned long& version,
      unsigned long& id)
      /*throw(FileReader::Exception)*/;

    static void
    skip_body_head_(FileReader& reader)
      /*throw(FileReader::Exception)*/;

    bool
    read_body_key_(
      FileReader& reader,
      Generics::MemBuf& key_buf,
      KeyType& key,
      ProfileOperation& operation,
      uint32_t& access_time)
      const /*throw(
        eh::Exception,
        FileReader::Exception,
        typename ReadBaseLevel<KeyType>::Exception)*/;

    Generics::ConstSmartMemBuf_var
    read_body_profile_(FileReader& reader)
      const /*throw(eh::Exception, FileReader::Exception)*/;

    void
    skip_body_profile_(FileReader& reader)
      const /*throw(FileReader::Exception)*/;

    uint32_t
    read_key_(
      FileReader& reader,
      Generics::MemBuf& key_buf,
      KeyType& key)
      const /*throw(
        eh::Exception,
        FileReader::Exception,
        typename ReadBaseLevel<KeyType>::Exception)*/;

    unsigned long
    write_key_(
      FileWriter& writer,
      Generics::MemBuf& key_buf,
      const KeyType& key)
      const /*throw(
        eh::Exception,
        FileWriter::Exception,
        typename ReadBaseLevel<KeyType>::Exception)*/;

    void
    write_null_key_(FileWriter& writer)
      const /*throw(FileWriter::Exception)*/;

  private:
    const KeySerializerType key_serializer_;
    const bool disable_caching_on_fetch_;
    std::string index_file_name_;
    std::string body_file_name_;
    const FileController_var file_controller_;
    mutable std::unique_ptr<RandomAccessFile> file_;

    /// sorted
    ProfileRefs profiles_;
    unsigned long size_;
    unsigned long area_size_;
    unsigned long merge_free_size_;
    Generics::Time min_access_time_;
  };
}
}

#include "FileLevel.tpp"

#endif /*FILELEVEL_HPP*/
