#include "ResultSet.hpp"
#include "Lob.hpp"

namespace AdServer
{
namespace Commons
{
  namespace
  {
    const Generics::Time POSTGRES_EPOCH_DATE(
      String::SubString("2000-01-01"), "%Y-%m-%d");
  }

  namespace Postgres
  {
    ResultSet::ResultSet(Connection *conn, PGresult* res) noexcept
      : cur_row_number_(-1),
        res_(res, PQclear),
        conn_(ReferenceCounting::add_ref(conn))
    {
    }

    ResultSet::~ResultSet() noexcept
    {
      clean_();
      if (conn_)
      {
        conn_->clear_result_();
      }
    }

    Oid
    ResultSet::get_column_type_(int column_number) const /*throw(Exception)*/
    {
      if (!res_.get())
      {
        Stream::Error err;
        err << __func__ << ": no result, do next() first";
        throw Exception(err);
      }
      Oid oid = PQftype(res_.get(), column_number - 1);
      if(oid == InvalidOid)
      {
        Stream::Error err;
        err << __func__ << ": invalid oid, most likely column number "
          << column_number << " out of range";
        throw Exception(err);
      }
      return oid;
    }

    void
    ResultSet::clean_() noexcept
    {
      if(res_.get())
      {
        res_ = nullptr;
      }
    }

    int
    ResultSet::rows() const noexcept
    {
      if(res_.get())
      {
        return PQntuples(res_.get());
      }
      else
      {
        return 0;
      }
    }

    int
    ResultSet::columns() const /*throw(NotSupported)*/
    {
      if(res_.get())
      {
        return PQnfields(res_.get());
      }
      else
      {
        throw NotSupported("Not supported for single row connection");
      }
    }

    const char*
    ResultSet::field_name(int column_number) const /*throw(NotSupported)*/
    {
      if(res_.get())
      {
        return PQfname(res_.get(), column_number - 1);
      }
      else
      {
        throw NotSupported("Not supported for single row connection");
      }
    }

    int
    ResultSet::field_name(const char* field_name) const /*throw(NotSupported)*/
    {
      if(res_.get())
      {
        return PQfnumber(res_.get(), field_name);
      }
      else
      {
        throw NotSupported("Not supported for single row connection");
      }
    }

    bool
    ResultSet::next() /*throw(Exception)*/
    {
      if (++cur_row_number_ < rows())
      {
        return true;
      }
      else
      {
        clean_();
        if (conn_)
        {
          res_.reset(conn_->next_());
          cur_row_number_ = 0;
          return (rows() > 0);
        }
        else
        {
          return false;
        }
      }
    }

    /*
    bool
    ResultSet::prev() throw(NotSupported)
    {
      if (!full_res_)
      {
        throw NotSupported("Not supported in single row mode");
      }
      if (cur_row_number_ == 0)
      {
        return false;
      }
      --cur_row_number_;

      return true;
    }
    */

    std::string
    ResultSet::parse_string_(int column_number) const
      /*throw(Exception)*/
    {
      std::string ret;
      int size = PQgetlength(res_.get(), cur_row_number_, column_number - 1);
      ret.reserve(size);
      ret = PQgetvalue(res_.get(), cur_row_number_, column_number - 1);
      return ret;
    }

    void
    ResultSet::parse_binary_numeric_(
      int column_number,
      bool& negative,
      unsigned long& integer,
      unsigned long& fractional,
      int16_t& fractional_power) const
      /*throw(Exception)*/
    {
      char* value = PQgetvalue(res_.get(), cur_row_number_, column_number - 1);
      int size = PQgetlength(res_.get(), cur_row_number_, column_number - 1);
      if(size < 8)
      {
        Stream::Error err;
        err << __func__ << ": for numeric size of field '" <<
          field_name(column_number) << "' is too small " << size;
        throw Exception(err);
      }
      uint16_t* src_ptr = reinterpret_cast<uint16_t*>(value);
      uint16_t len = ntohs(*src_ptr++);//len of number in words
      if(size - 8 < static_cast<int>(len * sizeof(uint16_t)))
      {
        Stream::Error err;
        err << __func__ << ": '" << field_name(column_number) <<
          ", expected " << len * sizeof(uint16_t) <<
          " bytes for digits, but have only " << size - 8;
        throw Exception(err);
      }
      short int weight = static_cast<short int>(ntohs(*src_ptr++));
      uint16_t sign_value = ntohs(*src_ptr++);
      if (!(sign_value == NUMERIC_POS ||
            sign_value == NUMERIC_NEG ||
            sign_value == NUMERIC_NAN))
      {
        Stream::Error err;
        err << __func__ << ": '" << field_name(column_number) <<
          "invalid value of sign " << sign_value;
        throw Exception(err);
      }
      ++src_ptr; // skip dscale
      negative = (sign_value == NUMERIC_NEG);
      integer = 0;
      fractional = 0;
      fractional_power = 0;

      for (uint16_t i = 0; i < len; i++)
      {
        int16_t d = ntohs(*src_ptr++);
        if (d < 0 || d >= NBASE)
        {
          Stream::Error err;
          err << __func__ << ": invalid representation of digits " << d;
          throw Exception(err);
        }

        if(weight >= 0)
        {
          integer *= NBASE;
          integer += d;
          --weight;
        }
        else
        {
          fractional_power -= DEC_DIGITS;
          fractional *= NBASE;
          fractional += d;
        }
      }

      if(weight >= 0)
      {
        integer *= Generics::DecimalHelper::pow10<unsigned long>(
          DEC_DIGITS * (weight + 1));
        assert(fractional == 0 && fractional_power == 0);
      }
      else
      {
        // change power if original weight <= -2
        fractional_power += (weight + 1) * DEC_DIGITS;
      }
    }

    std::string
    ResultSet::get_string(int column_number) const
      /*throw(Exception)*/
    {
      if (!res_.get())
      {
        Stream::Error err;
        err << __func__ << ": no result, do next() first";
        throw Exception(err);
      }

      int format = PQfformat(res_.get(), column_number - 1);

#ifdef BINARY_RECIVING_DATA
      if(format == 1)//binary;
      {
        Oid type = get_column_type_(column_number);
        switch(type)
        {
          case INT8OID:
            return get_number_as_string_<int64_t>(column_number);
          case INT2OID:
            return get_number_as_string_<int16_t>(column_number);
          case INT4OID:
            return get_number_as_string_<int32_t>(column_number);
          case TEXTOID:
          case BPCHAROID:
          case VARCHAROID:
            return parse_string_(column_number);
          case NUMERICOID:
            {
              bool negative;
              unsigned long integer;
              unsigned long fractional;
              int16_t fractional_power;
              parse_binary_numeric_(
                column_number, negative, integer, fractional, fractional_power);
              std::ostringstream val;
              val << (negative ? '-' : ' ') << integer << "." << fractional; // INCORRECT !!!
              return val.str();
            }
          default:
            {
              Stream::Error err;
              err << __func__ << ": not supported getting as string field '" <<
                field_name(column_number) << "' has type " << type;
              throw Exception(err);
            }
        }
      }
#endif

      if(format == 0)
      {
        return parse_string_(column_number);
      }

      throw Exception("Unsupported format of result");
    }

    char
    ResultSet::get_char(int column_number) const /*throw(Exception)*/
    {
      Oid type = get_column_type_(column_number);
      if(type != CHAROID && type != BPCHAROID && type != VARCHAROID)
      {
        Stream::Error err;
        err << __func__ << ": field '" <<
          field_name(column_number) <<
          "' has not character type " << type;
        throw Exception(err);
      }
      return *PQgetvalue(res_.get(), cur_row_number_, column_number - 1);
    }

    bool
    ResultSet::is_null(int columm_number) const noexcept
    {
      if(PQgetisnull(res_.get(), cur_row_number_, columm_number - 1))
      {
        return true;
      }
      return false;
    }

    Generics::Time
    ResultSet::get_date(int column_number) const /*throw(Exception)*/
    {
      Oid type = get_column_type_(column_number);
      switch(type)
      {
        case DATEOID:
          return parse_date_(column_number);
          break;
        case TIMESTAMPOID:
          {
            Generics::Time value = get_timestamp(column_number);
            value -= value.tv_sec % 86400;
            return value;
          }
          break;
        default:
          {
            Stream::Error err;
            err << __func__ << ": field '" <<
              field_name(column_number) <<
              "' has not date type " << type;
            throw Exception(err);
          }
          break;
      }
    }

    template<typename NumberType>
    std::string
    ResultSet::get_number_as_string_(int column_number) const /*throw(Exception)*/
    {
      static const char* FUN = "ResultSet::get_number_as_string_()";

      char val_str[std::numeric_limits<NumberType>::digits10 + 3];
      NumberType val;
      parse_integer_(column_number, val);
      if(!String::StringManip::int_to_str(val, val_str, sizeof(val_str)))
      {
        Stream::Error ostr;
        ostr << FUN << ": internal error: can't do int_to_str";
        throw Exception(ostr);
      }
      return val_str;
    }

    Generics::Time
    ResultSet::parse_date_(int column_number) const /*throw(Exception)*/
    {
      try
      {
        int format = PQfformat(res_.get(), column_number - 1);
        char* db_str = PQgetvalue(res_.get(), cur_row_number_, column_number - 1);
#ifdef BINARY_RECIVING_DATA
        if(format == 1)//binary;
        {
          int size = PQgetlength(res_.get(), cur_row_number_, column_number - 1);
          if(size != sizeof(uint32_t))
          {
            Stream::Error err;
            err << " wrong size of field '" << field_name(column_number) <<
              "' expected " << sizeof(uint32_t) << ", got " << size;
            throw Exception(err);
          }
          uint32_t* data = reinterpret_cast<uint32_t*>(db_str);
          Generics::Time ret = POSTGRES_EPOCH_DATE +
            static_cast<int32_t>(ntohl(*data)) *
            Generics::Time::ONE_DAY.tv_sec;
          return ret;
        }
#endif
        if(format == 0)//text
        {
          Generics::Time ret(String::SubString(db_str), "%Y-%m-%d");
          return ret;
        }
        throw Exception("Unsupported format of result");
      }
      catch(const eh::Exception& e)
      {
        Stream::Error err;
        err << __func__ << ": " << e.what();
        throw Exception(err);
      }
    }

    Generics::Time
    ResultSet::get_time(int column_number) const /*throw(Exception)*/
    {
      Oid type = get_column_type_(column_number);
      switch(type)
      {
        case TIMEOID:
          return parse_time_(column_number);
          break;
        case TIMESTAMPOID:
          {
            Generics::Time value = get_timestamp(column_number);
            return Generics::Time(
              value.tv_sec % 86400, value.tv_usec);
          }
          break;
        default:
          {
            Stream::Error err;
            err << __func__ << ": field '" <<
              field_name(column_number) <<
              "' has not date type " << type;
            throw Exception(err);
          }
          break;
      }
    }

    Generics::Time
    ResultSet::parse_time_(int column_number) const /*throw(Exception)*/
    {
      try
      {
        int format = PQfformat(res_.get(), column_number - 1);
        char* db_str = PQgetvalue(res_.get(), cur_row_number_, column_number - 1);
#ifdef BINARY_RECIVING_DATA
        if(format == 1)//binary;
        {
          int size = PQgetlength(res_.get(), cur_row_number_, column_number - 1);
          if(size != sizeof(uint64_t))
          {
            Stream::Error err;
            err << __func__ << ": field '" << field_name(column_number) <<
              "' has wrong size, expected " << sizeof(uint64_t) <<
              ", got " << size;
            throw Exception(err);
          }
          uint64_t value;
          uint32_t* data = reinterpret_cast<uint32_t*>(db_str);
          uint32_t* res = reinterpret_cast<uint32_t*>(&value);
          res[0] = ntohl(data[1]);
          res[1] = ntohl(data[0]);
          Generics::Time ret(static_cast<time_t>(value/1000000));
          return ret;
        }
#endif
        if(format == 0)//text;
        {
          Generics::Time ret(String::SubString(db_str), "%H:%M:%S");
          return ret;
        }
        throw Exception("Unsupported format of result");
      }
      catch(const eh::Exception& e)
      {
        Stream::Error err;
        err << __func__ << ": " << e.what();
        throw Exception(err);
      }
    }

    Generics::Time
    ResultSet::get_timestamp(int column_number) const
      /*throw(Exception)*/
    {
      Oid type = get_column_type_(column_number);
      if(type != TIMESTAMPOID)
      {
        Stream::Error err;
        err << __func__ << ": field '" <<
          field_name(column_number) <<
          "' has not timestamp type " << type;
        throw Exception(err);
      }
      try
      {
        int format = PQfformat(res_.get(), column_number - 1);
        char* db_str = PQgetvalue(res_.get(), cur_row_number_, column_number - 1);
#ifdef BINARY_RECIVING_DATA
        if(format == 1)//binary;
        {
          int size = PQgetlength(res_.get(), cur_row_number_, column_number - 1);
          if(size != sizeof(uint64_t))
          {
            Stream::Error err;
            err << " wrong size of field, expected 8, got " << size;
            throw Exception(err);
          }
          int64_t mseconds;
          uint32_t* data = reinterpret_cast<uint32_t*>(db_str);
          uint32_t* res = reinterpret_cast<uint32_t*>(&mseconds);
          res[0] = ntohl(data[1]);
          res[1] = ntohl(data[0]);
          suseconds_t usec = mseconds % Generics::Time::USEC_MAX;
          if(usec < 0)
          {
            usec = Generics::Time::USEC_MAX + usec;
            mseconds -= Generics::Time::USEC_MAX;
          }
          Generics::Time ret(mseconds / Generics::Time::USEC_MAX, usec);
          ret += POSTGRES_EPOCH_DATE;
          return ret;
        }
#endif
        if(format == 0)//text;
        {
          std::string temp_string;
          String::SubString sub_str(db_str);
          if (sub_str.length() >= 7 && sub_str.at(sub_str.length() - 7) != '.')
          {//Postgres return less than 6 digit for microseconds
            temp_string.reserve(sub_str.length() + 7);
            temp_string.assign(sub_str.data(), sub_str.length());
            size_t count = 1;
            size_t i = 6;
            while(i > 1 && sub_str.at(sub_str.length() - i) != '.')
            {
              count++;
              i--;
            }
            temp_string.append(count, '0');
            sub_str = temp_string;
          }
          Generics::Time ret(sub_str, "%Y-%m-%d %H:%M:%S.%q");
          return ret;
        }
        throw Exception("Unsupported format of result");
      }
      catch(const eh::Exception& e)
      {
        Stream::Error err;
        err << __func__ << ": " << e.what();
        throw Exception(err);
      }
    }

    LobHolder
    ResultSet::get_blob(int column_number) const
      /*throw(Exception)*/
    {
      Oid oid = get_number<Oid>(column_number);
      Lob lob(conn_, oid);
      pg_int64 len = lob.length();
      LobHolder::CharArray array(len);
      lob.read(array.get(), len);
      return LobHolder(std::move(array), len);
    }
  }
}
}
