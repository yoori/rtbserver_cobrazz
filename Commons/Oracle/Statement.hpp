#ifndef _ORACLE_STATEMENT_HPP_
#define _ORACLE_STATEMENT_HPP_

#include <memory>
#include <vector>
#include <sstream>

#include <eh/Exception.hpp>
#include <ReferenceCounting/ReferenceCounting.hpp>
#include <ReferenceCounting/AtomicImpl.hpp>
#include <Generics/Time.hpp>
#include <Generics/Scheduler.hpp>
#include <Generics/ArrayAutoPtr.hpp>

#include "OraException.hpp"
#include "InnerEntities.hpp"
#include "ResultSet.hpp"

namespace AdServer
{
  namespace Commons
  {
    namespace Oracle
    {
      class Environment;
      class Object;
      class ParamArrayHolder;
      typedef ReferenceCounting::SmartPtr<Object> Object_var;

      class Statement;
      typedef ReferenceCounting::SmartPtr<Statement> Statement_var;

      class Connection;
      typedef ReferenceCounting::SmartPtr<Connection> Connection_var;      

      /**
       * class Statement
       */
      class Statement: public ReferenceCounting::AtomicImpl
      {
      public:
        typedef AdServer::Commons::Oracle::Exception Exception;
        typedef AdServer::Commons::Oracle::SqlException SqlException;
        typedef AdServer::Commons::Oracle::NotSupported NotSupported;
        typedef AdServer::Commons::Oracle::TimedOut TimedOut;

        enum DataTypes
        {
          CHAR = 0,
          STRING,
          NUMBER,
          INT,
          UNSIGNED_INT,
          FLOAT,
          BLOB,
          DATE,
          TIMESTAMP
        };

        void execute(const Generics::Time* timeout = 0)
          /*throw(SqlException, TimedOut, NotSupported)*/;

        ResultSet_var execute_query(
          const Generics::Time* timeout = 0,
          unsigned long fetch_size = 1024)
          /*throw(Exception, SqlException, TimedOut, NotSupported)*/;

        template<class ContainerType>
        void set_array(
          unsigned int param_index,
          const ContainerType& cont,
          const char* type)
          /*throw(Exception, SqlException, NotSupported)*/;

        void set_number_array(
          unsigned int ind,
          std::vector<std::string>& vec,
          const char* type)
          /*throw(eh::Exception, TimedOut, Exception, SqlException, NotSupported)*/;

        void set_array(
          unsigned int ind,
          std::vector<std::string>& vec,
          const char* type)
          /*throw(eh::Exception, TimedOut, Exception, SqlException, NotSupported)*/;

        void set_array(
          unsigned int ind,
          std::vector<Object_var>& vec,
          const char* type)
          /*throw(eh::Exception, TimedOut, Exception, SqlException, NotSupported)*/;

        void set_char(unsigned int col_index, char ch)
          /*throw(Exception, SqlException, NotSupported)*/;

        void set_date(unsigned int col_index, const Generics::Time& date)
          /*throw(Exception, SqlException, NotSupported)*/;

        void set_timestamp(unsigned int col_index, const Generics::Time& date)
          /*throw(Exception, SqlException, NotSupported)*/;

        void set_string(unsigned int col_index, const std::string& str)
          /*throw(SqlException, NotSupported)*/;

        void set_uint(unsigned int col_index, unsigned int val)
          /*throw(SqlException, NotSupported)*/;

        void set_double(unsigned int col_index, double val)
          /*throw(SqlException, NotSupported)*/;

        void set_number_from_string(
          unsigned int col_index,
          const char* val)
          /*throw(Exception, SqlException, NotSupported)*/;
        
        void set_blob(unsigned int col_index, const Lob& val)
          /*throw(Exception, SqlException, NotSupported)*/;

        void set_null(unsigned int col_index, DataTypes type)
          /*throw(Exception, SqlException, NotSupported)*/;

        template<typename DecimalType>
        void set_decimal(unsigned int col_index, const DecimalType& val)
          /*throw(Exception, SqlException)*/;

      protected:
        Statement(
          Connection* connection,
          const char* query)
          /*throw(Exception, SqlException)*/;

        virtual ~Statement() noexcept;

        bool execute_(const Generics::Time* timeout)
          /*throw(TimedOut, SqlException, NotSupported)*/;

        ResultSet_var execute_query_(
          const Generics::Time* timeout,
          unsigned long fetch_size)
          /*throw(TimedOut, Exception, SqlException, NotSupported)*/;

        bool is_terminated_() noexcept;

        void check_terminated_(const char* fun)
          /*throw(NotSupported)*/;

        void bind_(
          unsigned long col_index,
          unsigned long oci_type,
          const void* buf,
          unsigned long size,
          void* indicator,
          void* data_len) /*throw(SqlException)*/;
        
#ifdef _USE_OCCI
        void describe_type_(
          const char* full_type_name,
          OCIObjectPtr<OCIType, true>& type_info)
          /*throw(TimedOut, SqlException)*/;
#endif
        
        const Generics::Time* use_timeout_(
          const Generics::Time* timeout) const
          noexcept;

        void handle_timeout_() /*throw(SqlException, ConnectionError)*/;

      public:
        struct ParamValueHolder: public ReferenceCounting::DefaultImpl<>
        {
          virtual ~ParamValueHolder() noexcept {}
        };

        typedef ReferenceCounting::SmartPtr<ParamValueHolder> ParamValueHolder_var;

      private:
        Statement() noexcept;

        friend class Connection;
        friend class ResultSet;
        friend class SqlStream;
        friend class ParamArrayHolder;
        friend class ParamStreamHolder;
        friend class OCIObjectFillCache;

        Connection_var connection_;
        std::string query_;
        std::list<ParamValueHolder_var> params_;

#ifdef _USE_OCCI
        OCIHandlePtr<OCIStmt, OCI_HTYPE_STMT> stmt_handle_;
        OCIHandlePtr<OCIError, OCI_HTYPE_ERROR> error_handle_;
#endif
        unsigned long type_;
      };

      /**
       * SqlStream
       */
      class SqlStream: public ReferenceCounting::AtomicImpl
      {
        friend class Statement;
        static const unsigned long MAX_OBJECT_FIELDS = 1024;

      public:
        typedef AdServer::Commons::Oracle::Exception Exception;
        typedef AdServer::Commons::Oracle::SqlException SqlException;
        typedef AdServer::Commons::Oracle::NotSupported NotSupported;

        SqlStream(Statement* statement, void* type)
          /*throw(SqlException, NotSupported)*/;

        void set_date(const Generics::Time& date)
          /*throw(Exception, SqlException)*/;

        void set_string(const std::string& str)
          /*throw(SqlException)*/;

        //is not a built-in method; <setNumber> is used
        void set_int(int val)
          /*throw(SqlException)*/;

        //is not a built-in method; <setNumber> is used
        void set_uint(unsigned int val)
          /*throw(SqlException)*/;

        //is not a built-in method; <setNumber> is used
        void set_long(long val)
          /*throw(SqlException)*/;

        //is not a built-in method; <setNumber> is used
        void set_ulong(unsigned long val)
          /*throw(SqlException)*/;

        //is not a built-in method; <setNumber> is used
        void set_double(double val)
          /*throw(SqlException)*/;

        void set_number_from_string(
          const char* val)
          /*throw(Exception, SqlException)*/;

        //the default precision is 5
        void set_null(Statement::DataTypes type)
          /*throw(Exception, SqlException, NotSupported)*/;

        template<typename DecimalType>
        void set_decimal(const DecimalType& val)
          /*throw(Exception, SqlException)*/;

        virtual ~SqlStream() noexcept;

      private:
        void close_(void* obj) /*throw(SqlException)*/;

        void* object_ind_() noexcept;

        void set_object_attr_(
          const char* fun, void* val, unsigned long type_id)
          /*throw(SqlException)*/;
        
      private:
        Statement_var statement_;

#ifdef _USE_OCCI
        OCIType* oci_type_;
        OCIDuration oci_duration_;
        OCIAnyData* oci_any_data_;
        OCIInd obj_ind_[MAX_OBJECT_FIELDS];
        
        std::list<OCIInd> indicators_;
#endif
      };

      typedef ReferenceCounting::SmartPtr<SqlStream> SqlStream_var;
      
      /**
       * class Object
       */
      class Object: public ReferenceCounting::AtomicImpl
      {
      public:
        typedef AdServer::Commons::Oracle::Exception Exception;
        typedef AdServer::Commons::Oracle::SqlException SqlException;

        virtual const char* getSQLTypeName() const /*throw(eh::Exception)*/ = 0;

        virtual void writeSQL(SqlStream& stream)
          /*throw(eh::Exception, SqlException)*/ = 0;

        virtual void readSQL(SqlStream& stream)
          /*throw(eh::Exception, SqlException)*/ = 0;

      protected:
        virtual ~Object() noexcept;
      };

      typedef std::vector<Object_var> ObjectVector;
    }
  }
}

namespace AdServer
{
  namespace Commons
  {
    namespace Oracle
    {
      /*
       * struct Lob
       */
      inline
      Lob::Lob(const char* src, size_t src_len, bool src_own) noexcept
        : buffer(src),
          length(src_len),
          buffer_own(src_own)
      {}

      inline
      Lob::Lob(const Lob& src) noexcept
        : buffer(src.buffer),
          length(src.length),
          buffer_own(src.buffer_own)
      {
        *const_cast<size_t*>(&src.length) = 0;
      }

      inline
      Lob::~Lob() noexcept
      {
        if (length && buffer_own)
        {
          delete[] const_cast<char*>(buffer);
        }
      }
      
      /*
       * class Object
       */
      inline
      Object::~Object() noexcept
      {}

      /*
       * class Statement
       */
      template<class ContainerType>
      void Statement::set_array(
        unsigned int param_index,
        const ContainerType& cont,
        const char* type)
        /*throw(Exception, SqlException, NotSupported)*/
      {
        std::vector<std::string> vec;
        for(typename ContainerType::const_iterator it = cont.begin();
            it != cont.end(); ++it)
        {
          std::ostringstream ostr;
          ostr << *it;
          vec.push_back(ostr.str());
        }
        set_number_array(param_index, vec, type);
      }
      
    } //namespace Oracle
  } //namespace Commons
} //namespace AdServer

namespace AdServer
{
namespace Commons
{
namespace Oracle
{
  template<typename DecimalType>
  void Statement::set_decimal(unsigned int col_index, const DecimalType& val)
    /*throw(Exception, SqlException)*/
  {
    set_number_from_string(col_index, val.str().c_str());
  }

  template<typename DecimalType>
  void SqlStream::set_decimal(const DecimalType& val)
    /*throw(Exception, SqlException)*/
  {
    set_number_from_string(val.str().c_str());
  }
}
}
}

#endif /*_ORACLE_STATEMENT_HPP_*/
