#ifndef AD_SERVER_CHANNELSVCS_SERIALIZATION_V28
#define AD_SERVER_CHANNELSVCS_SERIALIZATION_V28

#include<string>
#include<Commons/CorbaTypes.hpp>
#include<ChannelSvcs/ChannelCommons/CommonTypes.hpp>

namespace AdServer
{
  namespace ChannelSvcs_v28
  {
    typedef ChannelSvcs::Parts Parts;
    typedef ChannelSvcs::SubStringVector SubStringVector;

#define BREAK_POS_V28 0x00
#define QUOTED_FLAG_V28 0x01
#define NEXT_BYTE_FLAG_V28 0x80 

#define EXACT_BIT_V28 0x01 //exact match
#define TYPE_BITS_V28 0x06 //type bits
#define NEGATIVE_BIT_V28 0x08 //negative bit
#define FLAGS_IN_BYTE_V28 7

    class Serialization
    {
      enum types_of_trigger
      {
        URL_TYPE = 0x02, //'U'
        PAGE_TYPE = 0x04, //'P'
        SEARCH_TYPE = 0x06 //'S'
      }; // type of trigger 2 bites

    public:

      DECLARE_EXCEPTION(BadFormatException, eh::DescriptiveException);

      template<class ALLOCATOR, class RESULTTYPE>
      static void serialize(
        const Parts& parts,
        char type,
        bool exact,
        bool negative,
        ALLOCATOR&& alloc,
        RESULTTYPE& result)
        /*throw(eh::Exception)*/;

      static void serialize(
        const Parts& parts,
        char type,
        bool exact,
        bool negative,
        std::string& result)
        /*throw(eh::Exception)*/;

      static void get_parts(
        const void* data, size_t size, SubStringVector& parts)
        /*throw(eh::Exception, BadFormatException)*/;

      static Parts& get_parts(
        const void* data, size_t size, Parts& parts)
        /*throw(eh::Exception, BadFormatException)*/;
      
      static void get_parts(const std::string& data, SubStringVector& parts)
        /*throw(eh::Exception, BadFormatException)*/;
      
      static std::string& get_trigger(
        const void* data, size_t size, std::string& trigger)
        /*throw(eh::Exception, BadFormatException)*/;
      
      static std::string& get_trigger(
        const std::string& in_trigger, std::string& trigger)
        /*throw(eh::Exception, BadFormatException)*/;

      static size_t get_reserve_size(const void* data) noexcept;
      
      struct OctCorbaAllocator
      {
        OctCorbaAllocator(CORBACommons::OctSeq& oct_seq)
          : oct_seq_(oct_seq) {};
        void allocate(size_t size)
        {
          oct_seq_.length(size);
        }
        CORBACommons::OctSeq& oct_seq_;
      };

      struct StringAllocator
      {
        StringAllocator(std::string& str)
          : str_(str) {};
        void allocate(size_t size)
        {
          str_.resize(size);
        }
        std::string& str_;
      };

      static bool exact(const void* trigger) noexcept;

      static bool negative(const void* trigger) noexcept;

      static bool quoted(const void* trigger, size_t index) noexcept;

      static char trigger_type(const void* trigger) noexcept;

    private:
      static void add_bit_(
        unsigned char& bitvalue,
        bool value,
        unsigned char* &result)
        noexcept;
      static bool get_bit_(
        size_t& position,
        unsigned char& bitvalue,
        const unsigned char* result)
        noexcept;
    };
  }
}
namespace AdServer
{
  namespace ChannelSvcs_v28
  {
    inline
    void Serialization::add_bit_(unsigned char& bitvalue, bool value, unsigned char* &result) noexcept
    {
      if(bitvalue == NEXT_BYTE_FLAG_V28)
      {
        *result |= bitvalue;
        result++;
        *result = 0;
        bitvalue = 0x01;
      }
      if(value)
      {
        *result |= bitvalue;
      }
      bitvalue = bitvalue << 1;
    }

    inline
    bool Serialization::get_bit_(
      size_t& position,
      unsigned char& bitvalue,
      const unsigned char* result) noexcept
    {
      if(bitvalue == NEXT_BYTE_FLAG_V28)
      {
        ++position;
        bitvalue = 0x01;
      }
      bool ret = (result[position] & bitvalue);
      bitvalue = bitvalue << 1;
      return ret;
    }

    template<class ALLOCATOR, class RESULTTYPE>
    void Serialization::serialize(
      const Parts& parts,
      char type,
      bool exact,
      bool negative,
      ALLOCATOR&& alloc,
      RESULTTYPE& result)
      /*throw(eh::Exception)*/
    {
      size_t len = 1, count = parts.size();
      for(Parts::const_iterator it = parts.begin();
          it != parts.end(); ++it)
      {
        len += it->part.size() + 1;
      }
      size_t count_bytes_for_flags =
        (count <= FLAGS_IN_BYTE_V28) ? 1 : (count + FLAGS_IN_BYTE_V28 - 1) / FLAGS_IN_BYTE_V28;
      //should be at least one byte
      alloc.allocate(len + count_bytes_for_flags);
      size_t position = 1;
      unsigned char bitvalue = 0x01;
      result[0] = exact ? EXACT_BIT_V28 : 0;//flags of trigger
      result[0] |= negative ? NEGATIVE_BIT_V28 : 0;//flags of trigger
      unsigned char* flags = reinterpret_cast<unsigned char*>(&result[1]);
      switch(type)
      {
        case 'U':
          result[0] |= URL_TYPE;
          break;
        case 'P':
          result[0] |= PAGE_TYPE;
          break;
        case 'S':
          result[0] |= SEARCH_TYPE;
          break;
      }
      position += count_bytes_for_flags;//space for flags of words
      *flags = 0;
      for(Parts::const_iterator it = parts.begin();
          it != parts.end(); ++it)
      {
        add_bit_(bitvalue, it->quotes, flags);
        memcpy(&result[position], it->part.data(), it->part.size());
        position += it->part.size();
        result[position++] = BREAK_POS_V28;//break separator
      }
    }

    inline
    void Serialization::serialize(
      const Parts& parts,
      char type,
      bool exact,
      bool negative,
      std::string& result)
      /*throw(eh::Exception)*/
    {
      serialize(
        parts, type, exact, negative, StringAllocator(result), result); 
    }

    inline
    void Serialization::get_parts(
      const void* data, size_t size, SubStringVector& parts)
      /*throw(eh::Exception, BadFormatException)*/
    {
      if(size < 3)
      {
        return;
      }
      const unsigned char* ptr = static_cast<const unsigned char*>(data);
      const char* end = reinterpret_cast<const char*>(ptr) + size;
      ptr++;//skip flags of trigger
      size_t reserve_size = FLAGS_IN_BYTE_V28;
      while(*ptr & NEXT_BYTE_FLAG_V28)
      {
        ptr++;//skip flags of triggers
        reserve_size += FLAGS_IN_BYTE_V28;
      }
      ptr++;
      parts.resize(reserve_size);
      const char* word_ptr = reinterpret_cast<const char*>(ptr);
      SubStringVector::iterator it_last = parts.begin();
      while(word_ptr < end)
      {
        *it_last = String::SubString(word_ptr);
        word_ptr += it_last->size() + 1;
        ++it_last;
      }
      parts.resize(std::distance(parts.begin(), it_last));
    }

    inline
    Parts& Serialization::get_parts(
      const void* data, size_t size, Parts& parts)
      /*throw(eh::Exception, BadFormatException)*/
    {
      if(size < 3)
      {
        parts.clear();
        return parts;
      }
      const unsigned char* ptr = static_cast<const unsigned char*>(data);
      const char* end = reinterpret_cast<const char*>(ptr) + size;
      ptr++;//skip flags of trigger
      size_t reserve_size = FLAGS_IN_BYTE_V28;
      while(*ptr & NEXT_BYTE_FLAG_V28)
      {
        ptr++;//skip flags of triggers
        reserve_size += FLAGS_IN_BYTE_V28;
      }
      ptr++;
      parts.resize(reserve_size);
      const char* word_ptr = reinterpret_cast<const char*>(ptr);
      auto it_last = parts.begin();
      while(word_ptr < end)
      {
        it_last->part = String::SubString(word_ptr);
        it_last->quotes =
          quoted(data, std::distance(parts.begin(), it_last));
        word_ptr += it_last->part.size() + 1;
        ++it_last;
      }
      parts.resize(std::distance(parts.begin(), it_last));
      return parts;
    }

    inline
    void Serialization::get_parts(
      const std::string& data, SubStringVector& parts)
      /*throw(eh::Exception, BadFormatException)*/
    {
      get_parts(data.c_str(), data.size(), parts);
    }

    inline
    std::string& Serialization::get_trigger(
      const void* data, size_t size, std::string& trigger)
      /*throw(eh::Exception, BadFormatException)*/
    {
      const char* ptr = static_cast<const char*>(data);
      const char* start = ptr;
      const char* end = ptr + size;
      bool exact_flag = exact(ptr);
      char type = trigger_type(ptr);
      ptr++; //skip flags of trigger
      for(const unsigned char* p = reinterpret_cast<const unsigned char*>(ptr);
          *p & NEXT_BYTE_FLAG_V28; p++, ptr++);
      ptr++; //skip flags of parts
      trigger.clear();
      if(ptr == end)
      {
        return trigger;
      }
      if(exact_flag)
      {
        trigger.push_back('[');
      }
      for(size_t index = 0; ptr < end; index++)
      {
        bool quotes = quoted(start, index);
        String::SubString part(ptr);
        if(index != 0 && type != 'U')
        {
          trigger.push_back(' ');
        }
        if(quotes && type != 'U')
        {
          trigger.push_back('"');
        }
        trigger.append(part.data(), part.length());
        if(quotes && type != 'U')
        {
          trigger.push_back('"');
        }
        ptr += part.length() + 1;
      }
      if(exact_flag)
      {
        trigger.push_back(']');
      }
      return trigger;
    }
    
    inline
    std::string& Serialization::get_trigger(
      const std::string& in, std::string& trigger)
      /*throw(eh::Exception, BadFormatException)*/
    {
      return get_trigger(in.data(), in.size(), trigger);
    }
    
    inline
    size_t Serialization::get_reserve_size(const void* data) noexcept
    {
      size_t size = FLAGS_IN_BYTE_V28;
      for(const unsigned char* flags =
          static_cast<const unsigned char*>(data) + 1;
          *flags & NEXT_BYTE_FLAG_V28; size += FLAGS_IN_BYTE_V28, flags++);
      return size;
    }

    inline
    bool Serialization::exact(const void* trigger) noexcept
    {
      const unsigned char* trigger_flags =
        static_cast<const unsigned char*>(trigger);
      return trigger_flags[0] & EXACT_BIT_V28;
    }

    inline
    bool Serialization::negative(const void* trigger) noexcept
    {
      const unsigned char* trigger_flags =
        static_cast<const unsigned char*>(trigger);
      return trigger_flags[0] & NEGATIVE_BIT_V28;
    }

    inline
    bool Serialization::quoted(const void* trigger, size_t index)
      noexcept
    {
      size_t index_in_flags = index / FLAGS_IN_BYTE_V28;
      unsigned char bitvalue =
        (1 << (index - index_in_flags * FLAGS_IN_BYTE_V28));
      const unsigned char* trigger_flags =
        static_cast<const unsigned char*>(trigger) + 1;
      return trigger_flags[index_in_flags] & bitvalue;
    }

    inline
    char Serialization::trigger_type(const void* trigger) noexcept
    {
      const unsigned char* trigger_flags =
        static_cast<const unsigned char*>(trigger);
      switch(*trigger_flags & TYPE_BITS_V28)
      {
        case URL_TYPE:
          return 'U';
          break;
        case PAGE_TYPE:
          return 'P';
          break;
        case SEARCH_TYPE:
          return 'S';
          break;
      }
      return 'I';
    }
  }
}
#endif//AD_SERVER_CHANNELSVCS_SERIALIZATION_V28

