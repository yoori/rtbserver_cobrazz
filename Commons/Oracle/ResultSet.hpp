#ifndef _ORACLE_RESULTSET_HPP_
#define _ORACLE_RESULTSET_HPP_

#include <memory>
#include <vector>

#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <Generics/Time.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/ArrayAutoPtr.hpp>
#include <String/SubString.hpp>

#include "OraException.hpp"
#include "InnerEntities.hpp"

namespace AdServer
{
  namespace Commons
  {
    namespace Oracle
    {
      class Statement;
      typedef ReferenceCounting::SmartPtr<Statement> Statement_var;

      class Environment;
      
      /** Lob */
      struct Lob
      {
        const char* const buffer;
        const size_t length;
        const bool buffer_own;

        Lob(const char* src = 0, size_t src_len = 0, bool buffer_own = true) noexcept;
        Lob(const Lob&) noexcept;
        ~Lob() noexcept;
      };

      /** ResultSet */
      class ResultSet: public ReferenceCounting::AtomicImpl
      {
      public:
        typedef AdServer::Commons::Oracle::Exception Exception;
        typedef AdServer::Commons::Oracle::SqlException SqlException;
        typedef AdServer::Commons::Oracle::NotSupported NotSupported;
        typedef AdServer::Commons::Oracle::InvalidValue InvalidValue;
        DECLARE_EXCEPTION(Overflow, SqlException);

        bool next() /*throw(TimedOut, SqlException, NotSupported)*/;

        unsigned long rows_count() const /*throw(SqlException)*/;
        
        bool is_null(unsigned int col_index) const
          /*throw(SqlException, NotSupported)*/;

        Generics::Time get_timestamp(unsigned int col_index) const
          /*throw(Exception, SqlException, NotSupported)*/;

        Generics::Time get_date(unsigned int col_index) const
          /*throw(Exception, SqlException, NotSupported)*/;

        char get_char(unsigned int col_index) const
          /*throw(SqlException, InvalidValue)*/;

        std::string get_string(unsigned int col_index) const
          /*throw(SqlException, NotSupported)*/;

        unsigned int get_uint(unsigned int col_index) const
          /*throw(InvalidValue, SqlException, NotSupported)*/;

        int get_int(unsigned int col_index) const
          /*throw(InvalidValue, SqlException, NotSupported)*/;

        uint64_t get_uint64(unsigned int col_index) const
          /*throw(InvalidValue, SqlException, NotSupported)*/;

        int64_t get_int64(unsigned int col_index) const
          /*throw(InvalidValue, SqlException, NotSupported)*/;

        double get_double(unsigned int col_index) const
          /*throw(SqlException, NotSupported)*/;

        std::string get_number_as_string(
          unsigned int col_index) const
          /*throw(Overflow, Exception, SqlException)*/;

        template<typename DecimalType>
        DecimalType
        get_decimal(unsigned int col_index, bool trunc = true) const
          /*throw(typename DecimalType::Overflow, Exception, SqlException)*/;
        
        Lob get_blob(unsigned int col_index) const
          /*throw(TimedOut, Exception, SqlException, NotSupported)*/;

      protected:
        ResultSet(Statement* statement, unsigned long fetch_size = 1024)
          /*throw(SqlException)*/;

        virtual ~ResultSet() noexcept;

        void check_terminated_(const char* fun) const
          /*throw(NotSupported)*/;
        
        unsigned long columns_count_() const /*throw(SqlException)*/;
        
        Generics::Time get_date_(const void*) const
          /*throw(Exception, SqlException, NotSupported)*/;

        Generics::Time get_datetime_(const void*) const
          /*throw(Exception, SqlException, NotSupported)*/;

        template<typename IntType, unsigned int OciSign>
        IntType get_long_(unsigned int col_index) const
          /*throw(InvalidValue, SqlException, NotSupported)*/;

        void check_column_index_(unsigned long) const
          /*throw(SqlException)*/;

      private:
        ResultSet() noexcept;

        friend class Statement;

      private:
        struct Column: public ReferenceCounting::DefaultImpl<>
        {
          Column(
            Environment* environment_val,
            long oci_type_val,
            unsigned long size_val,
            unsigned long fetch_size_val)
            /*throw(SqlException, eh::Exception)*/;

          virtual ~Column() noexcept;
          
          Environment* environment;          
          long oci_type;
          unsigned long size;
          unsigned long fetch_size;

          void init_() /*throw(SqlException, eh::Exception)*/;

          void init_fetch_cells_() /*throw(SqlException)*/;

          void clear_fetch_cells_() noexcept;
        
#ifdef _USE_OCCI
          Generics::ArrayAutoPtr<char> fetch_buffer;
          Generics::ArrayAutoPtr<sb2> indicators;
          Generics::ArrayAutoPtr<ub2> data_lens;
#endif
        };

        typedef ReferenceCounting::SmartPtr<Column> Column_var;
        typedef std::vector<Column_var> ColumnList;
        
      private:
        Statement_var statement_;
        ColumnList columns_;
        unsigned long fetch_count_;
        unsigned long rows_fetched_;
        unsigned long current_row_;
	bool is_eod_;
        
#ifdef _USE_OCCI
        mutable OCIHandlePtr<OCIError, OCI_HTYPE_ERROR> error_handle_;
#endif
      };

      typedef ReferenceCounting::SmartPtr<ResultSet> ResultSet_var;
    }
  }
}

namespace AdServer
{
namespace Commons
{
namespace Oracle
{
  template<typename DecimalType>
  DecimalType
  string_to_decimal(const std::string& str, bool trunc)
    /*throw(typename DecimalType::Overflow)*/
  {
    static const char* FUN = "string_to_decimal()";

    long exp;
    Stream::Parser istr(str.c_str() + str.size() - 3);
    istr >> exp;
    std::string::const_iterator pos = str.begin();
    const long sign = (*pos == '-' || *pos == '+' ? 1 : 0);
    const long digits_count = str.size() - 4 - sign;

    if(exp > DecimalType::INTEGER_RANK ||
       (!trunc &&
       digits_count - exp - 1 - (*pos == '-' || *pos == '+' ? 1 : 0) >
         DecimalType::FRACTION_RANK))
    {
      Stream::Error ostr;
      ostr << FUN << ": can't convert number '" << str <<
        "' to DecimalType<" << DecimalType::TOTAL_RANK << ", " <<
        DecimalType::FRACTION_RANK << ">";
      throw typename DecimalType::Overflow(ostr);
    }

    size_t res_len = digits_count + sign + (
      exp < 0 ? -exp + 1 : (
        digits_count > exp ? 0 : exp + 1 - digits_count) + 1);
    Generics::ArrayAutoPtr<char> decimal_str(res_len);
    char* decimal_pos = decimal_str.get();

    if(sign)
    {
      *decimal_pos++ = *pos;
      ++pos;
    }

    long frac_i = 0;
    if(exp < 0)
    {
      *decimal_pos++ = '0';
      *decimal_pos++ = '.';
      long max_j = std::min(-exp - 1, static_cast<long>(DecimalType::FRACTION_RANK));
      for(long j = 0; j < max_j; ++j)
      {
        *decimal_pos++ = '0';
      }
      frac_i = max_j;
    }

    long i = 0;
    for(; *pos != 'E' && frac_i < static_cast<long>(DecimalType::FRACTION_RANK);
        ++pos)
    {
      if(*pos != '.')
      {
        *decimal_pos++ = *pos;
        if(i == exp)
        {
          *decimal_pos++ = '.';
        }
        if(i > exp)
        {
          ++frac_i;
        }
        ++i;
      }
    }

    for(; i < exp + 1; ++i)
    {
      *decimal_pos++ = '0';
    }
    *decimal_pos = 0;

    assert(decimal_pos - decimal_str.get() + 1 <= static_cast<int>(res_len));

    return DecimalType(String::SubString(decimal_str.get()));
  }

  template<typename DecimalType>
  DecimalType
  ResultSet::get_decimal(unsigned int col_index, bool trunc) const
    /*throw(typename DecimalType::Overflow, Exception, SqlException)*/
  {
    static const char* FUN = "ResultSet::get_decimal()";

    try
    {
      std::string str = get_number_as_string(col_index);
      return string_to_decimal<DecimalType>(str, trunc);
    }
    catch(const SqlException& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": " << ex.what();
      throw SqlException(ostr);
    }
    catch(const typename DecimalType::Overflow& ex)
    {
      Stream::Error ostr;
      ostr << FUN << ": caught Decimal<>::Overflow: " << ex.what();
      throw typename DecimalType::Overflow(ostr);
    }
  }
}
}
}

#endif

