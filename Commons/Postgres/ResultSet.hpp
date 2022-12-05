#ifndef POSTGRES_RESULTSET_HPP
#define POSTGRES_RESULTSET_HPP

#include <arpa/inet.h>
#include <libpq-fe.h>
#include <string>
#include <Generics/Time.hpp>
#include <Generics/CommonDecimal.hpp>
#include <Generics/SimpleDecimal.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <ReferenceCounting/SmartPtr.hpp>
#include <Commons/Postgres/Declarations.hpp>
#include <Commons/Postgres/Connection.hpp>

namespace AdServer
{
namespace Commons
{
  namespace Postgres
  {
    struct LobHolder
    {
      typedef Generics::ArrayAutoPtr<char> CharArray;

      LobHolder(char* src = 0, size_t src_len = 0, bool buffer_own = true) noexcept;
      LobHolder(const LobHolder&) noexcept = delete;
      LobHolder(LobHolder&& tmp) noexcept;
      LobHolder(CharArray&& tmp, size_t src_len) noexcept;
      ~LobHolder() noexcept;
    private:
      std::unique_ptr<char[]> data_;
    public:
      const char* const buffer;
      const size_t length;
    };

    class ResultSet: public ReferenceCounting::AtomicImpl
    {
    public:
      ResultSet(Connection *conn, PGresult* res = 0) noexcept;

      /* return count rows of result for sync mode
       * always return 1 for async mode
       * for cursor returns count rows in portion
      */
      int rows() const noexcept;

      int columns() const /*throw(NotSupported)*/;

      const char* field_name(int columm_number) const /*throw(NotSupported)*/;

      int field_name(const char* field_name) const /*throw(NotSupported)*/;

      bool next() /*throw(Exception)*/;

      bool prev() /*throw(NotSupported)*/;

      std::string get_string(int column_number) const /*throw(Exception)*/;

      char get_char(int column_number) const /*throw(Exception)*/;

      template<typename Number>
      Number get_number(int column_number) const /*throw(Exception)*/;

      bool is_null(int columm_number) const noexcept;

      Generics::Time get_date(int column_number) const /*throw(Exception)*/;

      Generics::Time get_time(int column_number) const /*throw(Exception)*/;

      Generics::Time get_timestamp(int column_number) const /*throw(Exception)*/;

      template<typename DecimalType>
      DecimalType get_decimal(int column_number) const
        /*throw(typename DecimalType::Overflow, Exception)*/;

      LobHolder get_blob(int column_number) const
        /*throw(Exception)*/;

    protected:
      Oid get_column_type_(int column_number) const /*throw(Exception)*/;

      template<typename Number>
      void parse_integer_(int column_number, Number& value) const /*throw(Exception)*/;

      template<typename Number>
      std::string
      get_number_as_string_(int column_number) const /*throw(Exception)*/;

      // result number is integer + fractional * 10 ^ fractional_power
      // fractional_power always <= 0
      void parse_binary_numeric_(
        int column_number,
        bool& negative,
        unsigned long& integer,
        unsigned long& fractional,
        int16_t& fractional_power) const
        /*throw(Exception)*/;

      static
      unsigned long scale_fractional_(unsigned long, int16_t corr)
        /*throw(Exception)*/;

      std::string parse_string_(int column_number) const /*throw(Exception)*/;

      Generics::Time parse_date_(int column_number) const /*throw(Exception)*/;

      Generics::Time parse_time_(int column_number) const /*throw(Exception)*/;

      virtual
      ~ResultSet() noexcept;
    private:
      void clean_() noexcept;
    private:
      int cur_row_number_;
      PGresultPtr res_;
      Connection_var conn_;//for single row queries
    };

  }
}
}

namespace AdServer
{
namespace Commons
{
  namespace Postgres
  {
    template<typename Number>
    Number ResultSet::get_number(int column_number) const /*throw(Exception)*/
    {
      Oid type = get_column_type_(column_number);
      if (type == NUMERICOID)
      {//allow only numeric values without fractional part
#ifdef BINARY_RECIVING_DATA
        bool negative;
        unsigned long integer;
        unsigned long fractional;
        int16_t fractional_power;
        parse_binary_numeric_(
          column_number, negative, integer, fractional, fractional_power);
        if (fractional != 0)
        {
          Stream::Error err;//not allow to trunc values from database
          err << __func__ << ": '" << field_name(column_number)
            << "' fractional is " << fractional
            << ", power of fractional part is " << fractional_power
            << ", but allow only without fractional part ";
          throw Exception(err);
        }
        return (negative ?  -static_cast<Number>(integer) :
                static_cast<Number>(integer));
#endif
      } else if (type != INT8OID && type != INT2OID && type != INT4OID && type != OIDOID)
      {
        Stream::Error err;
        err << __func__ << ": field '"
          << field_name(column_number)
          << "' has not digital type " << type;
        throw Exception(err);
      }
      Number value;
      parse_integer_(column_number, value);
      return value;
    }

    template<typename Number>
    void ResultSet::parse_integer_(int column_number, Number& value) const
      /*throw(Exception)*/
    {
      int format = PQfformat(res_.get(), column_number - 1);
      char* num_ptr = PQgetvalue(res_.get(), cur_row_number_, column_number - 1);
      Number* src_ptr = reinterpret_cast<Number*>(num_ptr);
#ifdef BINARY_RECIVING_DATA
      if(format == 1)//binary;
      {
        int size = PQgetlength(res_.get(), cur_row_number_, column_number - 1);
        uint32_t* src_ptr2;
        uint32_t* res_ptr;
        switch(size)
        {
          case 1:
            value = *src_ptr;
            break;
          case 2:
            value = ntohs(*src_ptr);
            break;
          case 4:
            value = ntohl(*src_ptr);
            break;
          case 8:
            src_ptr2 = reinterpret_cast<uint32_t*>(num_ptr);
            res_ptr = reinterpret_cast<uint32_t*>(&value);
            res_ptr[0] = ntohl(*(src_ptr2 + 1));
            res_ptr[1] = ntohl(*(src_ptr2));
            break;
          default:
            {
              Stream::Error err;
              err << __func__ << ": column '"
                << field_name(column_number)
                << "' has unsupported size of number " << size;
              throw Exception(err);
            }
/*
            char* res_ptr2 = reinterpret_cast<char*>(&value);
            for(size_t i = 0; i < sizeof(Number); i++)
            {
              res_ptr2[i] = num_ptr[sizeof(Number) - i - 1];
            }
*/
            break;
        }
        return;
      }
#endif
      if(format == 0)
      {
        std::istringstream istr(num_ptr);
        istr >> value;
        return;
      }
      throw Exception("Unsupported format of result");
    }

    template<typename DecimalType>
    DecimalType ResultSet::get_decimal(int column_number) const
      /*throw(typename DecimalType::Overflow, Exception)*/
    {
      try
      {
        Oid type = get_column_type_(column_number);
        if(type != NUMERICOID)
        {
          long integer;
          if (type == INT8OID || type == INT2OID || type == INT4OID)
          {
            parse_integer_(column_number, integer);
            bool negative = false;
            if(integer < 0)
            {
              negative = true;
              integer = -integer;
            }
            return DecimalType(negative, integer, 0);
          }
          Stream::Error err;
          err << __func__ << ": field '"
            << field_name(column_number)
            << "' has not numeric type " << type;
          throw Exception(err);
        }
        int format = PQfformat(res_.get(), column_number - 1);
#ifdef BINARY_RECIVING_DATA
        if(format == 1)//binary;
        {
          bool negative;
          unsigned long integer;
          unsigned long fractional;
          int16_t fractional_power;
          parse_binary_numeric_(
            column_number, negative, integer, fractional, fractional_power);
          fractional_power += DecimalType::FRACTION_RANK;
          if(fractional_power < 0 &&
            fractional % Generics::DecimalHelper::pow10<unsigned long>(
              -fractional_power))
          {
             Stream::Error err;//not allow to trunc values from database
             err << __func__ << ": '" << field_name(column_number) <<
               "' power of fractional part is " << fractional_power <<
               ", but type has only " << DecimalType::FRACTION_RANK <<
               ", fractional is " << fractional;
             throw Exception(err);
          }
          fractional = scale_fractional_(fractional, fractional_power);
          DecimalType ret(negative, integer, fractional);
          return ret;
        }
#endif
        if(format == 0)//text
        {
          char* value = PQgetvalue(res_.get(), cur_row_number_, column_number - 1);
          int size = PQgetlength(res_.get(), cur_row_number_, column_number - 1);
          const String::SubString substr(value, size);
          DecimalType ret(substr);
          return ret;
        }
        throw Exception("Unsupported format of result");
      }
      catch(const eh::Exception& e)
      {
        Stream::Error err;
        err << __func__ << ": on fetching field '" << field_name(column_number)
          << "': " << e.what();
        throw Exception(err);
      }
    }

    inline
    unsigned long ResultSet::scale_fractional_(unsigned long value, int16_t corr)
      /*throw(Exception)*/
    {
      unsigned long fractional = value;
      if(corr < 0)
      {
        unsigned long pw10 = Generics::DecimalHelper::pow10<unsigned long>(-corr);
        fractional /= pw10;
      }
      else if(corr > 0)
      {
        unsigned long pw10 = Generics::DecimalHelper::pow10<unsigned long>(corr);
        if(std::numeric_limits<unsigned long>::max() / pw10 < value)
        {
          Stream::Error err;
          err << __func__ << ": overflow on multimpling " << pw10 << " and " <<  value;
          throw Exception(err);
        }
        fractional *= pw10;
      }
      return fractional;
    }

    inline
    LobHolder::LobHolder(char* src, size_t src_len, bool buffer_own) noexcept
      : data_(buffer_own ? src : 0),
        buffer(buffer_own ? data_.get() : src),
        length(src_len)
    {
    }

    inline
    LobHolder::LobHolder(CharArray&& tmp, size_t src_len) noexcept
      : data_(tmp.release()),
        buffer(data_.get()),
        length(src_len)
    {
    }

    inline
    LobHolder::LobHolder(LobHolder&& lob) noexcept
      : data_(lob.data_.release()),
        buffer(lob.buffer),
        length(lob.length)
    {
    }

    inline
    LobHolder::~LobHolder() noexcept
    {
    }

  }
}
}

#endif //POSTGRES_RESULTSET_HPP
