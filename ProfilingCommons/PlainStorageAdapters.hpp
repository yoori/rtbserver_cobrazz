#ifndef _PROFILINGCOMMONS_PLAIN_STORAGE_ADAPTERS_HPP_
#define _PROFILINGCOMMONS_PLAIN_STORAGE_ADAPTERS_HPP_

#include <eh/Exception.hpp>
#include <Generics/HashTableAdapters.hpp>
#include <Commons/UserInfoManip.hpp>

namespace AdServer
{
  namespace ProfilingCommons
  {
    namespace
    {
      const unsigned long MAP_HASH_BLOCKS = 100000;
    }

    struct UuidPackHashAdapter: public Generics::Uuid
    {
      UuidPackHashAdapter();
      UuidPackHashAdapter(const char* init);
      UuidPackHashAdapter(const Generics::Uuid& init);

      unsigned long hash() const;
    };

    struct UuidAccessor
    {
      DECLARE_EXCEPTION(Exception, eh::DescriptiveException);

      static unsigned int size(const Generics::Uuid& key) noexcept;

      static void load(const void* buf, unsigned int size, Generics::Uuid& key)
        /*throw(eh::Exception)*/;

      static void save(const Generics::Uuid& key, void* buf, unsigned int size)
        /*throw(eh::Exception)*/;

      static void
      read(Generics::Uuid& key, const void* buf, unsigned long buf_size)
        /*throw(eh::Exception)*/;

      static void
      write(void* buf, unsigned long buf_size, const Generics::Uuid& key)
        /*throw(eh::Exception)*/;

      static unsigned int size(const UuidPackHashAdapter& key) noexcept;

      static void load(const void* buf, unsigned int size, UuidPackHashAdapter& key)
        /*throw(eh::Exception)*/;
    };

    typedef UuidPackHashAdapter RequestIdPackHashAdapter;
    typedef UuidAccessor RequestIdAccessor;

    typedef UuidAccessor UserIdAccessor;

    struct StringSerializer
    {
      static void
      read(std::string& key, void* buf, unsigned long buf_size)
      {
        key.assign(static_cast<const char*>(buf), buf_size);
      }

      static void
      read(Generics::StringHashAdapter& key, void* buf, unsigned long buf_size)
      {
        key = Generics::StringHashAdapter(
          std::string(static_cast<const char*>(buf), buf_size));
      }

      static void
      write(void* buf, unsigned long buf_size, const std::string& key)
      {
        ::memcpy(buf, key.data(), buf_size);
      }

      static void
      write(void* buf, unsigned long buf_size, const Generics::StringHashAdapter& key)
      {
        write(buf, buf_size, key.text());
      }

      static unsigned long
      size(const std::string& key)
      {
        return key.size();
      }

      static unsigned long
      size(const Generics::StringHashAdapter& key)
      {
        return key.text().size();
      }
    };

    struct NumericSerializer
    {
      static void
      read(uint64_t& key, void* buf, unsigned long buf_size)
      {
        assert(buf_size == 8);
        key = *static_cast<const uint64_t*>(buf);
      }

      template<typename NumberType>
      static void
      read(Generics::NumericHashAdapter<NumberType>& key, void* buf, unsigned long buf_size)
      {
        assert(buf_size == 8);
        key = Generics::NumericHashAdapter<NumberType>(*static_cast<const uint64_t*>(buf));
      }

      static void
      write(void* buf, unsigned long buf_size, uint64_t key)
      {
        assert(buf_size == 8);
        *static_cast<uint64_t*>(buf) = key;
      }

      template<typename NumberType>
      static void
      write(void* buf, unsigned long buf_size, const Generics::NumericHashAdapter<NumberType>& key)
      {
        assert(buf_size == 8);
        *static_cast<uint64_t*>(buf) = key.value();
      }
 
      template<typename NumberType>
      static unsigned long
      size(const Generics::NumericHashAdapter<NumberType>&)
      {
        return sizeof(NumberType);
      }

      static unsigned long
      size(uint64_t)
      {
        return 8;
      }
    };
  }
}

namespace AdServer
{
  namespace ProfilingCommons
  {
    /* UuidPackHashAdapter */
    inline
    UuidPackHashAdapter::UuidPackHashAdapter()
    {}

    inline
    UuidPackHashAdapter::UuidPackHashAdapter(const char* init)
      : Generics::Uuid(init)
    {}

    inline
    UuidPackHashAdapter::UuidPackHashAdapter(
      const Generics::Uuid& init)
      : Generics::Uuid(init)
    {}

    inline
    unsigned long UuidPackHashAdapter::hash() const
    {
      return AdServer::Commons::uuid_distribution_hash(*this) %
        MAP_HASH_BLOCKS;
    }

    /* UuidAccessor */
    inline
    unsigned int
    UuidAccessor::size(const Generics::Uuid&) noexcept
    {
      return Generics::Uuid::size();
    }

    inline
    unsigned int
    UuidAccessor::size(const UuidPackHashAdapter&) noexcept
    {
      return Generics::Uuid::size();
    }

    inline
    void
    UuidAccessor::load(
      const void* buf, unsigned int size, Generics::Uuid& key)
      /*throw(eh::Exception)*/
    {
      const char* buf_ptr = static_cast<const char*>(buf);

      if(size == Generics::Uuid::encoded_size())
      {
        key = Generics::Uuid(String::SubString(buf_ptr, size));
      }
      else
      {
        key = Generics::Uuid(buf_ptr, buf_ptr + size);
      }
    }

    inline
    void
    UuidAccessor::load(
      const void* buf, unsigned int size, UuidPackHashAdapter& key)
      /*throw(eh::Exception)*/
    {
      Generics::Uuid id;
      load(buf, size, id);
      key = UuidPackHashAdapter(id);
    }

    inline
    void
    UuidAccessor::save(
      const Generics::Uuid& key, void* buf, unsigned int size)
      /*throw(eh::Exception)*/
    {
      if(key.size() > size)
      {
        throw Exception("error");
      }

      ::memcpy(static_cast<char*>(buf),
        &*key.begin(), key.size());
    }

    inline
    void
    UuidAccessor::read(
      Generics::Uuid& key,
      const void* buf,
      unsigned long size)
      /*throw(eh::Exception)*/
    {
      load(buf, size, key);
    }

    inline
    void
    UuidAccessor::write(
      void* buf,
      unsigned long size,
      const Generics::Uuid& key)
      /*throw(eh::Exception)*/
    {
      if(key.size() > size)
      {
        throw Exception("error");
      }

      ::memcpy(static_cast<char*>(buf),
        &*key.begin(), key.size());
    }
  }
}

#endif /*_PROFILINGCOMMONS_PLAIN_STORAGE_ADAPTERS_HPP_*/
