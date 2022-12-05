#ifndef AD_SERVER_CHANNELSVCS_SERIALIZATION
#define AD_SERVER_CHANNELSVCS_SERIALIZATION

#include<string>
#include<Commons/CorbaTypes.hpp>
#include<ChannelSvcs/ChannelCommons/CommonTypes.hpp>

namespace AdServer
{
  namespace ChannelSvcs
  {
    namespace
    {
      enum FLAGS: unsigned char 
      {
        BREAK_POS = 0x00,
        QUOTED_FLAG = 0x01,
        NEXT_BYTE_FLAG = 0x80
      };
      enum BITS: unsigned char 
      {
        EXACT_BIT = 0x01, //exact match
        TYPE_BITS = 0x0E, //type bits
        NEGATIVE_BIT = 0x10 //negative bit
      };

      const size_t k_flags_in_byte = 7;
    }

    /* provide interface to extract information from Part */
    class PartAdapter
    {
    public:
      bool is_quoted(const Part& part) noexcept
      {
        return part.quotes;
      }
      const String::SubString& get_part(const Part& part) noexcept
      {
        return part.part;
      }
    };

    /* Format of trigger:
     * #0byte - flags of trigger:
     * 0 bit - exact trigger
     * 1-3 bits - type of trigger
     * 4 bit - negative
     * #1+ byte flags of parts of trigger, count bytes depends on length of trigger
     * for every part one bit of quoting, it is used 7 bytes for flag, 
     * last bit is flag of next byte for flags
     * % parts of trigger splitted by zero
     * */

    class Serialization
    {
      enum types_of_trigger: unsigned char
      {
        URL_TYPE = 0x02, //'U'
        PAGE_TYPE = 0x04, //'P'
        SEARCH_TYPE = 0x06, //'S'
        URL_KEYWORD_TYPE = 0x08,//'R'
        UID_KEYWORD_TYPE = 0x0A //'D'
      }; // type of trigger 3 bites

    public:

      DECLARE_EXCEPTION(BadFormatException, eh::DescriptiveException);

      template<
        class ALLOCATOR, class RESULTTYPE,
        class PARTS, class PARTADAPTER = PartAdapter()>
      static void serialize(
        const PARTS& parts,
        char type,
        bool exact,
        bool negative,
        ALLOCATOR&& alloc,
        RESULTTYPE& result,
        PARTADAPTER adapter)
        /*throw(eh::Exception)*/;

      static void serialize(
        const Parts& parts,
        char type,
        bool exact,
        bool negative,
        std::string& result)
        /*throw(eh::Exception)*/;

      static SubStringVector& get_parts(
        const void* data, size_t size, SubStringVector& parts)
        /*throw(eh::Exception, BadFormatException)*/;
      
      static Parts& get_parts(
        const void* data, size_t size, Parts& parts)
        /*throw(eh::Exception, BadFormatException)*/;
      
      static SubStringVector& get_parts(
        const std::string& data, SubStringVector& parts)
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

      struct NullAllocator
      {
        void allocate(size_t ){};
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
  namespace ChannelSvcs
  {
    inline
    void Serialization::add_bit_(unsigned char& bitvalue, bool value, unsigned char* &result) noexcept
    {
      if(bitvalue == NEXT_BYTE_FLAG)
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
      if(bitvalue == NEXT_BYTE_FLAG)
      {
        ++position;
        bitvalue = 0x01;
      }
      bool ret = (result[position] & bitvalue);
      bitvalue = bitvalue << 1;
      return ret;
    }

    template<class ALLOCATOR, class RESULTTYPE, class PARTS, class PARTADAPTER>
    void Serialization::serialize(
      const PARTS& parts,
      char type,
      bool exact,
      bool negative,
      ALLOCATOR&& alloc,
      RESULTTYPE& result,
      PARTADAPTER adapter)
      /*throw(eh::Exception)*/
    {
      size_t len = 1, count = 0;
      for(typename PARTS::const_iterator it = parts.begin();
          it != parts.end(); ++it)
      {
        len += adapter.get_part(*it).size() + 1;
        ++count;
      }
      size_t count_bytes_for_flags = (count <= k_flags_in_byte) ?
        1 : (count + k_flags_in_byte - 1) / k_flags_in_byte;
      //should be at least one byte
      alloc.allocate(len + count_bytes_for_flags);
      size_t position = 1;
      unsigned char bitvalue = 0x01;
      result[0] = exact ? EXACT_BIT : 0;//flags of trigger
      result[0] |= negative ? NEGATIVE_BIT : 0;//flags of trigger
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
        case 'R':
          result[0] |= URL_KEYWORD_TYPE;
          break;
        case 'D':
          result[0] |= UID_KEYWORD_TYPE;
          break;
      }
      position += count_bytes_for_flags;//space for flags of words
      *flags = 0;
      for(typename PARTS::const_iterator it = parts.begin();
          it != parts.end(); ++it)
      {
        add_bit_(bitvalue, adapter.is_quoted(*it), flags);
        const String::SubString& part = adapter.get_part(*it);
        memcpy(&result[position], part.data(), part.size());
        position += part.size();
        result[position++] = BREAK_POS;//break separator
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
        parts,
        type,
        exact, negative, StringAllocator(result), result, PartAdapter());
    }

    inline
    SubStringVector& Serialization::get_parts(
      const void* data, size_t size, SubStringVector& parts)
      /*throw(eh::Exception, BadFormatException)*/
    {
      if(size < 3)
      {
        return parts;
      }
      const unsigned char* ptr = static_cast<const unsigned char*>(data);
      const char* end = reinterpret_cast<const char*>(ptr) + size;
      ptr++;//skip flags of trigger
      size_t reserve_size = k_flags_in_byte;
      while(*ptr & NEXT_BYTE_FLAG)
      {
        ptr++;//skip flags of triggers
        reserve_size += k_flags_in_byte;
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
      return parts;
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
      size_t reserve_size = k_flags_in_byte;
      while(*ptr & NEXT_BYTE_FLAG)
      {
        ptr++;//skip flags of triggers
        reserve_size += k_flags_in_byte;
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
    SubStringVector& Serialization::get_parts(
      const std::string& data, SubStringVector& parts)
      /*throw(eh::Exception, BadFormatException)*/
    {
      return get_parts(data.c_str(), data.size(), parts);
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
          *p & NEXT_BYTE_FLAG; p++, ptr++);
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
          if(type == 'D')
          {
            trigger.push_back('\n');
          }
          else
          {
            trigger.push_back(' ');
          }
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
      size_t size = k_flags_in_byte;
      for(const unsigned char* flags =
          static_cast<const unsigned char*>(data) + 1;
          *flags & NEXT_BYTE_FLAG; size += k_flags_in_byte, flags++);
      return size;
    }

    inline
    bool Serialization::exact(const void* trigger) noexcept
    {
      const unsigned char* trigger_flags =
        static_cast<const unsigned char*>(trigger);
      return trigger_flags[0] & EXACT_BIT;
    }

    inline
    bool Serialization::negative(const void* trigger) noexcept
    {
      const unsigned char* trigger_flags =
        static_cast<const unsigned char*>(trigger);
      return trigger_flags[0] & NEGATIVE_BIT;
    }

    inline
    bool Serialization::quoted(const void* trigger, size_t index)
      noexcept
    {
      size_t index_in_flags = index / k_flags_in_byte;
      unsigned char bitvalue =
        (1 << (index - index_in_flags * k_flags_in_byte));
      const unsigned char* trigger_flags =
        static_cast<const unsigned char*>(trigger) + 1;
      return trigger_flags[index_in_flags] & bitvalue;
    }

    inline
    char Serialization::trigger_type(const void* trigger) noexcept
    {
      const unsigned char* trigger_flags =
        static_cast<const unsigned char*>(trigger);
      switch(*trigger_flags & TYPE_BITS)
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
        case URL_KEYWORD_TYPE:
          return 'R';
          break;
        case UID_KEYWORD_TYPE:
          return 'D';
          break;
      }
      return 'I';
    }
  }
}
#endif//AD_SERVER_CHANNELSVCS_SERIALIZATION

