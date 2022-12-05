
#include <limits>

#include "Utils.hpp"
#include "PGConnection.hpp"
#include <tests/AutoTests/Commons/FailContext.hpp>

namespace AutoTest
{
  namespace PQ
  {
    namespace 
    {
      //some stuff from catalog/pg_type.h
#define BOOLOID     16
#define CHAROID			18
#define NAMEOID			19
#define INT8OID			20
#define INT2OID			21
#define INT4OID			23
#define TEXTOID			25
#define FLOAT4OID   700
#define FLOAT8OID   701
#define BPCHAROID		1042
#define VARCHAROID	1043
#define DATEOID			1082
#define TIMESTAMPOID	1114
#define NUMERICOID		1700
      const Oid int8_oid = INT8OID;
      const Oid int2_oid = INT2OID;
      const Oid int4_oid = INT4OID;
      const Oid float8_oid = FLOAT8OID;
      const Oid float4_oid = FLOAT4OID;
      const Oid numeric_oid = NUMERICOID;
      const Oid timestamp_oid = TIMESTAMPOID;
      const Oid date_oid = DATEOID;
  
      const Oid name_oid = NAMEOID;
      const Oid char_oid = CHAROID;
      const Oid bool_oid = BOOLOID;
      const Oid text_oid = TEXTOID;
      const Oid varchar_oid = VARCHAROID;
      const Oid character_oid = BPCHAROID;

      struct numeric_transfer
      {
        signed short ndigits;
        signed short weight;
        signed short sign;
        signed short dscale;
      };
      
      static const int DEC_DIGITS = 4;

      template<bool is_signed>
      int
      get_sign(
        bool)
      {
        return 1;
      }
    
      template<>
      int
      get_sign<true>(
        bool sign)
      {
        if(sign)
        {
          return -1;
        }
        return 1;
      }
      
      template<>
      int
      get_sign<false>(
        bool)
      {
        return 1;
      }

      template <int numshorts>
      class bignum
      {

        unsigned short vals[numshorts];
        bool sign;
        int precision;
        int scale;

      public:
        
        bignum(
          unsigned long l = 0) :
          sign(false),
          precision(0),
          scale(0)
        {
          memset(&(vals[0]), 0, sizeof(vals));
          vals[0] = l & 0xffff;
          vals[1] = l >> 16;
        }
        
        bignum(
          char* ptr) :
          sign(false),
          precision(0),
          scale(0)
        {
          memset(&(vals[0]), 0, sizeof(vals));
          numeric_transfer* val = reinterpret_cast<numeric_transfer*>(ptr);
          unsigned short* data =reinterpret_cast<unsigned short *>
            (ptr + sizeof(numeric_transfer));

          precision = DEC_DIGITS * ntohs(val->ndigits);
          scale = -(ntohs(val->weight) - ntohs(val->ndigits) + 1) * DEC_DIGITS;
          sign = ntohs(val->sign) != 0;

          int numdig = ntohs(val->ndigits);
          for(int i=0; i < numdig; ++i ) {
            *this *= 10000; // 10^DEC_DIGITS
            *this += ntohs(data[i]);
          }
        }
        
        bignum&
        operator *=(
          unsigned short multi)
        {
          unsigned long carry = 0;

          for (int i = 0; i < numshorts; ++i)
          {
            unsigned long res = vals[i];
            res *= multi;
            res += carry;
            vals[i] = res & 0xffff;
            carry = res >> 16;
          }
          return *this;
        }
        
        bignum&
        operator +=(
          unsigned short added)
        {
          unsigned long carry = added;

          for( int i=0; carry != 0 && i < numshorts; ++i )
          {
            carry += vals[i];
            vals[i] = carry & 0xffff;
            carry >>= 16;
          }
          return *this;
        }

        template<class T>
        T to_number()
        {
          T ret = 0;
          for (int i = numshorts - 1; i >= 0; --i)
          {
            ret *= 0x10000;
            ret += vals[i];
          }
          for (int i = 0; i < scale; ++i)
          {
            ret /= 10;
          }
          ret *= get_sign<std::numeric_limits<T>::is_signed>(sign);
          return ret;
        }
        
        void dump(
          unsigned char* mem,
          size_t num )
        {
          int i;
          for( i=0; i<num && i<(numshorts*2); ++i )
            mem[i]=reinterpret_cast<const char *>(vals)[i];

          while(i<num)
            mem[i++]=0;
        }
      };
      
      // Utils
      double
      getFloat(
        Oid oid,
        int len,
        char* ptr,
        bool check_other = true);

      bool
      getBool(
        Oid oid,
        int len,
        char* ptr);

      long long
      getInt(
        Oid oid, int
        len,
        char* ptr,
        bool check_other = true)
      {
        String::SubString str(ptr, len);
        Stream::Parser strm(str);
        switch(oid)
        {
        case int8_oid://int8
          {
            int64_t ret = 0;
            strm >> ret;
            return ret;
          }
        case int2_oid://int2
          {
            int16_t ret = 0;
            strm >> ret;
            return ret;
          }
        case int4_oid://int4
          {
            int32_t ret = 0;
            strm >> ret;
            return ret;
          }
        case numeric_oid:
          {
            int64_t ret = 0;
            strm >> ret;
            return ret;
          }
        default:
          if(check_other)
            return static_cast<long long>(getFloat(oid, len, ptr, false));
        }
        return 0;
      }
      
      double getFloat(
        Oid oid,
        int len,
        char* ptr,
        bool check_other)
      {
        String::SubString str(ptr, len);
        Stream::Parser strm(str);
        switch(oid)
        {
        case float8_oid:
          {
            double ret = 0;
            strm >> ret;
            return ret;
          }
        case float4_oid:
        {
            if(len == 4)
            {
              float ret = 0;
              strm >> ret;
              return ret;
            }
            else if(len == 8)
            {
              double ret = 0;
              strm >> ret;
              return ret;
            }
          }
        case numeric_oid:
          {
            double ret = 0;
            strm >> ret;
            return ret;
          }
        default:
          if(check_other)
            return static_cast<double>(getInt(oid, len, ptr, false));
        }
        return 0;
      }

      bool getBool(
        Oid oid,
        int len,
        char* ptr)
      {
        if (oid != bool_oid)
        {
          throw Exception("Boolean value expected");
        }
        String::SubString str(ptr, len);
        Stream::Parser strm(str);
        bool ret = 0;
        strm >> ret;
        return ret;
      }
    }

    // Conn class
    
    Conn::Conn()
      : connection_(0)
    { }

    Conn::Conn(
      const std::string& user,
      const std::string& pswd,
      const std::string& host,
      const std::string& db)
      :connection_(0)
    {
      open(user, pswd, host, db);
    }

    static void
    noticeProcessor(
      void *,
      const char * msg)
    {
      AutoTest::Logger::thlog().debug_trace(String::SubString(msg));
    }

    void
    Conn::open_()
    {
      connection_ = PQsetdbLogin(host_.c_str(), 0, 0, 0, 
        db_.c_str(), user_.c_str(), pswd_.c_str());
      if (PQstatus(connection_) != CONNECTION_OK)
      {
        throw Exception(PQerrorMessage(connection_));
      }
      PQsetNoticeProcessor(connection_, noticeProcessor, 0);
    }

    void
    Conn::open_(
      const std::string& user,
      const std::string& pswd,
      const std::string& host,
      const std::string& db)
    {
      user_ = user;
      pswd_ = pswd;
      host_ = host;
      db_   = db;
      open_();
    }

    void
    Conn::open(
      const std::string& user,
      const std::string& pswd,
      const std::string& host,
      const std::string& db)
    {
      if(connection_)
      {
        throw Exception("try to open already opened connection");
      }
      open_(user, pswd, host, db);
    }
    
    void
    Conn::open()
    {
      if(connection_)
      {
        PQreset(connection_);
      }
      else
      {
        open_();
      }
    }

    void
    Conn::close()
    {
      if(connection_)
      {
        PQfinish(connection_);
        connection_ = 0;
      }
    }

    Conn::~Conn()
    {
      try
      {
        close ();
      }
      catch (...) {}
    }
    
    QueryTuple
    Conn::query(
      const std::string& statm)
    {
      std::string statement = statm;
      unsigned long param_count = 0;
      for(size_t found = statement.find(':', 0);
        found != std::string::npos; found = statement.find(':', found + 1))
      {
        unsigned long name_sz = 0;

        if (found > 0 and statement[found - 1] == ':')
            continue; // '::' used in postgress arrays
        
        while(
          std::isalnum(
            statement[found + name_sz + 1])) name_sz++;
        
        if (name_sz)
        {
          statement.replace(
            found,
            name_sz + 1,
            "$" + strof(++param_count));
        }
      }
      AutoTest::Logger::thlog().debug_trace("create query: " + statement);
      return QueryTuple(
        connection_, statement);
    }

    AutoTest::DBC::IQuery*
    Conn::get_query(
      const std::string& statement)
    {
      return new Query(query(statement));
    }

    // Query class

    Query::Query(
      const QueryTuple& tuple) :
      connection_(tuple.first),
      statement_(tuple.second)
    { }

    Query::~Query()
    { }

    void
    Query::throw_exception(
      const std::string& message)
    {
      Stream::Error error;
      error << "exception in:"
            << std::endl << "["
            << statement_
            << "]" << std::endl
            << "description: "
            << message;
      throw Exception(error);
    }

    ResultTuple
    Query::ask()
    {
      PGresult* res = 0;
      unsigned int sz = size();
      if(sz > 0)
      {
        typedef const char** values_type;
        values_type values = (values_type)alloca(sizeof(const char*)*sz);
        for(unsigned int i = 0; i < sz; ++i)
        {
          if(lengths_[i] == 0) 
            values[i] = 0;
          else
            values[i] = &buffer_[0] + values_[i];
        }
        res = PQexecParams(connection_,
          statement_.c_str(),
          sz,
          &oids_[0], 
          values,
          &lengths_[0],
          0,
          0);
      }
      else
      {
        res = PQexecParams(connection_,
          statement_.c_str(),
          0,       /* no params */
          0,
          0,
          0,
          0,
          0);
      }

      ExecStatusType pq_status = PQresultStatus(res);
      
      if(pq_status != PGRES_TUPLES_OK &&
         pq_status != PGRES_COMMAND_OK)
      {
        std::ostringstream ostr;
        ostr << PQresStatus(pq_status) << " (" <<
          PQresultErrorMessage(res) << ")";
        throw_exception(ostr.str());
      }
      return ResultTuple(statement_, res);
    }

    AutoTest::DBC::IResult*
    Query::ask_result()
    {
      return new AutoTest::PQ::Result(ask());
    }

    int
    Query::update()
    {
      AutoTest::PQ::Result r(ask());
      ExecStatusType status = PQresultStatus(r.result_set());
      return status == PGRES_COMMAND_OK ? 1 : 0;
    }

    AutoTest::DBC::IBasicQueryStream*
    Query::query_stream()
    {
      return this;
    }

    // Result class

    Result::Result(
      const ResultTuple& tuple) :
      statement_(tuple.first),
      result_set_(tuple.second),
      row_index_(-1),
      value_index_(0)
    {
      last_ = PQntuples(result_set_) - 1;
    }

    Result::~Result()
    {
      try
      {
        PQclear(result_set_);
      }
      catch(...) {}
    }
    
    void
    Result::get(
      int& value)
    {
      Oid oid = PQftype(result_set_, value_index_);
      int len = PQgetlength(result_set_, row_index_, value_index_);
      char* ptr = PQgetvalue(result_set_, row_index_, value_index_++);
      value = static_cast<int>(getInt(oid, len, ptr));
    }

    void
    Result::get(
      bool& value)
    {
      Oid oid = PQftype(result_set_, value_index_);
      int len = PQgetlength(result_set_, row_index_, value_index_);
      char* ptr = PQgetvalue(result_set_, row_index_, value_index_++);
      value = static_cast<int>(getBool(oid, len, ptr));
    }

    void
    Result::get(
      unsigned int& value)
    {
      Oid oid = PQftype(result_set_, value_index_);
      int len = PQgetlength(result_set_, row_index_, value_index_);
      char* ptr = PQgetvalue(result_set_, row_index_, value_index_++);
      value = static_cast<unsigned int>(getInt(oid, len, ptr));
    }

    void
    Result::get(
      float& value)
    {
      Oid oid = PQftype(result_set_, value_index_);
      int len = PQgetlength(result_set_, row_index_, value_index_);
      char* ptr = PQgetvalue(result_set_, row_index_, value_index_++);
      value = static_cast<float>(getFloat(oid, len, ptr));
    }

    void
    Result::get(
      double& value)
    {
      Oid oid = PQftype(result_set_, value_index_);
      int len = PQgetlength(result_set_, row_index_, value_index_);
      char* ptr = PQgetvalue(result_set_, row_index_, value_index_++);
      value = static_cast<double>(getFloat(oid, len, ptr));
    }

    void
    Result::get(
      std::string& value)
    {
      Oid oid = PQftype(result_set_, value_index_);
      int len = PQgetlength(result_set_, row_index_, value_index_);
      char* ptr = PQgetvalue(result_set_, row_index_, value_index_++);
      if(oid == name_oid || oid == char_oid 
        || oid == text_oid || oid == varchar_oid || oid == character_oid) 
      {
        value = std::string(ptr, len);
      }
    }

    void
    Result::get(
      Generics::ExtendedTime& value)
    {
      Oid oid = PQftype(result_set_, value_index_);
      char* ptr = PQgetvalue(result_set_, row_index_, value_index_++);
      struct tm tm;
      memset(&tm, 0, sizeof(tm));
      if(oid == timestamp_oid)
      {
        strptime(ptr, "%Y-%m-%d %H:%M:%S", &tm);
      }
      else if(oid == date_oid)
      {
        strptime(ptr, "%Y-%m-%d", &tm);
      }
      value = Generics::ExtendedTime(tm, 0, Generics::Time::TZ_GMT);
    }

    void
    Result::get(
      Generics::Time& value)
    {
      Oid oid = PQftype(result_set_, value_index_);
      char* ptr = PQgetvalue(result_set_, row_index_, value_index_++);
      struct tm tm;
      memset(&tm, 0, sizeof(tm));
      if(oid == timestamp_oid)
      {
        strptime(ptr, "%Y-%m-%d %H:%M:%S", &tm);
      }
      else if(oid == date_oid)
      {
        strptime(ptr, "%Y-%m-%d", &tm);
      }
      value = Generics::ExtendedTime(tm, 0, Generics::Time::TZ_GMT);
    }

    void
    Result::get(
      AutoTest::DBC::TextField&)
    {
      throw Exception("Text operation not supported");
    }
    
    PGresult*
    Result::result_set()
    {
      return result_set_;
    }

    // BasicQueryStream implementation
    BasicQueryStream::BasicQueryStream()
    { }
    
    void
    BasicQueryStream::set_string_(
      Oid type_oid,
      unsigned long type_size,
      const std::string& str_value)
    {
      AutoTest::Logger::thlog().debug_trace("set query value " +
        strof(values_.size()) + " = " + str_value);
      oids_.push_back(type_oid);
      lengths_.push_back(type_size);
      unsigned int sz = buffer_.size();
      values_.push_back(sz);
      unsigned int len = str_value.size() + 1;
      buffer_.resize(sz + len);
      memcpy(&buffer_[sz], str_value.c_str(), len);      
    }

    template<typename ValueType>
    void
    BasicQueryStream::set_as_string_(
      Oid type_oid,
      unsigned long type_size,
      const ValueType& val)
    {
      set_string_(type_oid, type_size, strof(val));
    }

    void
    BasicQueryStream::set_null_(
      Oid type_oid)
    {
      oids_.push_back(type_oid);
      lengths_.push_back(0);
      values_.push_back(0);
    }

    void
    BasicQueryStream::null(
      char)
    {
      set_null_(char_oid);
    }

    void
    BasicQueryStream::null(
      int)
    {
      set_null_(int4_oid);
    }

    void
    BasicQueryStream::null(
      bool)
    {
      set_null_(bool_oid);
    }

    void
    BasicQueryStream::null(
      unsigned int)
    {
      set_null_(int4_oid);
    }

    void
    BasicQueryStream::null(
      float)
    {
      set_null_(float4_oid);
    }

    void
    BasicQueryStream::null(
      double)
    {
      set_null_(numeric_oid);
    }

    void
    BasicQueryStream::null(
      const std::string&)
    {
      set_null_(text_oid);
    }


    void
    BasicQueryStream::null(
      const Generics::ExtendedTime&)
    {
      set_null_(timestamp_oid);
    }

    void
    BasicQueryStream::null(
      const Generics::Time&)
    {
      set_null_(date_oid);
    }

    void
    BasicQueryStream::null(
      const AutoTest::DBC::TextField&)
    {
      throw Exception("Text operation not supported");
    }

    void
    BasicQueryStream::set(
      char value)
    {
      set_as_string_(char_oid, 1, value);
    }

    void
    BasicQueryStream::set(
      int value)
    {
      set_as_string_(int4_oid, 4, value);
    }

    void
    BasicQueryStream::set(
      bool value)
    {
      set_as_string_(bool_oid, 1, value);
    }

    void
    BasicQueryStream::set(
      unsigned int value)
    {
      set_as_string_(int4_oid, 4, value);
    }

    void
    BasicQueryStream::set(
      float value)
    {
      set_as_string_(float4_oid, 4, value);
    }

    void
    BasicQueryStream::set(
      double value)
    {
      set_as_string_(numeric_oid, 8, value);
    }

    void
    BasicQueryStream::set(
      const std::string&  value)
    {
      set_string_(text_oid, value.size() + 1, value);
    }

    void
    BasicQueryStream::set(
      const Generics::ExtendedTime& value)
    {
      std::string str = value.format("%Y-%m-%d %H:%M:%S");
      set_string_(timestamp_oid, str.size() + 1, str);
    }

    void
    BasicQueryStream::set(
      const Generics::Time& value)
    {
      std::string str = value.get_gm_time().format("%Y-%m-%d %H:%M:%S");
      set_string_(timestamp_oid, str.size() + 1, str);
    }

    void
    BasicQueryStream::set(
      const AutoTest::DBC::TextField&)
    {
      throw Exception("Text operation not supported");
    }

    void
    BasicQueryStream::flush()
    { }

    bool
    Result::next()
    {
      if (row_index_ >= last_)
      {
        return false;
      }
      
      ++row_index_;
      value_index_ = 0;
      return true;
    }
    
    bool
    Result::is_null()
    {
      return
        PQgetisnull(
          result_set_,
          row_index_,
          value_index_) == 1;
    }
  }
}
