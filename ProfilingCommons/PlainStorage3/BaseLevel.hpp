#ifndef BASELEVEL_HPP
#define BASELEVEL_HPP

#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <Generics/Time.hpp>
#include <Generics/MemBuf.hpp>

namespace AdServer
{
namespace ProfilingCommons
{
  enum ProfileState
  {
    PS_NOT_FOUND,
    PS_ERASED,
    PS_FOUND
  };

  enum ProfileOperation
  {
    PO_NOT_FOUND = 0,
    PO_INSERT,
    PO_REWRITE,
    PO_ERASE
  };

  struct CheckProfileResult
  {
    ProfileOperation operation;
    unsigned long size;
  };

  struct GetProfileResult
  {
    GetProfileResult()
      : operation(PO_NOT_FOUND)
    {}

    ProfileOperation operation;
    Generics::ConstSmartMemBuf_var mem_buf;
    Generics::Time access_time;
  };

  template<typename KeyType>
  class ReadBaseLevel: public virtual ReferenceCounting::Interface
  {
  public:
    DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

    struct KeyIterator: public virtual ReferenceCounting::Interface
    {
      virtual bool
      get_next(KeyType& key, ProfileOperation& operation, Generics::Time& access_time)
        /*throw(Exception)*/ = 0;
    };

    typedef ReferenceCounting::SmartPtr<KeyIterator> KeyIterator_var;

    struct Iterator: public virtual ReferenceCounting::Interface
    {
      virtual bool
      get_next(KeyType& key, ProfileOperation& operation, Generics::Time& access_time)
        /*throw(Exception)*/ = 0;

      virtual Generics::ConstSmartMemBuf_var
      get_profile()
        /*throw(Exception)*/ = 0;
    };

    typedef ReferenceCounting::SmartPtr<Iterator> Iterator_var;

  public:
    virtual CheckProfileResult
    check_profile(const KeyType& key) const
      /*throw(Exception)*/ = 0;

    virtual GetProfileResult
    get_profile(const KeyType& key) const
      /*throw(Exception)*/ = 0;

    virtual KeyIterator_var
    get_key_iterator() const
      noexcept = 0;

    virtual Iterator_var
    get_iterator(unsigned long read_buffer_size) const
      /*throw(Exception)*/ = 0;

    // number of profiles
    virtual unsigned long
    size() const noexcept = 0;

    // estimated physical size
    virtual unsigned long
    area_size() const noexcept = 0;

    // size that will be freed on merge this level with next levels
    virtual unsigned long
    merge_free_size() const noexcept = 0;

    virtual Generics::Time
    min_access_time() const noexcept = 0;
  };

  template<typename KeyType>
  struct RWBaseLevel: public ReadBaseLevel<KeyType>
  {
    virtual Generics::ConstSmartMemBuf_var
    save_profile(
      const KeyType& key,
      const Generics::ConstSmartMemBuf* mem_buf,
      ProfileOperation operation,
      unsigned long next_size,
      const Generics::Time& now)
      /*throw(typename ReadBaseLevel<KeyType>::Exception)*/ = 0;

    virtual unsigned long
    remove_profile(
      const KeyType& key,
      unsigned long next_size)
      /*throw(typename ReadBaseLevel<KeyType>::Exception)*/ = 0;
  };
}
}

#endif /*BASELEVEL_HPP*/
