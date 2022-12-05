#ifndef POSTGRES_CONNECTION_HPP
#define POSTGRES_CONNECTION_HPP

#include<set>
#include<Generics/ActiveObject.hpp>
#include<Sync/PosixLock.hpp>
#include<Commons/Postgres/Declarations.hpp>
#include<Commons/Postgres/Statement.hpp>

namespace AdServer
{
namespace Commons
{
  namespace Postgres
  {
    class ConnectionOwner:
      public virtual ReferenceCounting::Interface
    {
    public:
      // connection pre destroy hook
      virtual bool
      destroy_connection(Connection* conn) noexcept = 0;

      // connection post destroy hook
      virtual void
      connection_destroyed() noexcept = 0;
    };

    typedef ReferenceCounting::SmartPtr<ConnectionOwner>
      ConnectionOwner_var;

    class ResultSet;
    typedef ReferenceCounting::SmartPtr<ResultSet> ResultSet_var;

    typedef decltype(&PQclear) PGresultDeleterType;
    typedef std::unique_ptr<PGresult, PGresultDeleterType> PGresultPtr;

    class Lob;

    class Connection: public ReferenceCounting::AtomicImpl
    {
    public:
      explicit
      Connection(const char* coninfo, ConnectionOwner* owner = 0)
        /*throw(ConnectionError)*/;

      /**
       * Check that connection isn't held by another thread
       * */
      bool is_free() const noexcept;

      /**
       * Check that connection was marked as bad
       * */
      bool is_bad() const noexcept;

      PGresultPtr
      execute_query_(const char* query)
        /*throw(SqlException, Postgres::Exception)*/;

      /**
       *  Execute query in sync mode
       *  @param sql query
       *  @return result of query
       * */
      ResultSet_var
      execute_query(const char* query)
        /*throw(SqlException, Postgres::Exception)*/;

      /**
       *  Execute statement
       *  @param statement to execute on connection
       *  @param sync mode of query, if query is async then result
       *  is fetched by one row
       *  @param format of returning data, binary should be faster
       *  @return result of query, result should be used (next() method)
       *  or destroyed before next query
       * */
      ResultSet_var
      execute_statement(
        Statement* statement,
        bool sync = false,
        DATA_FORMATS format = AUTO)
        /*throw(SqlException, Postgres::Exception)*/;

      /**
       *  Execute statement using cursor
       *  @param statement to execute on connection
       *  @param portion_size  size of buffer for one portion
       *  @param format of returning data, binary should be faster
       *  @return result of query, result should be used (next() method)
       *  or destroyed before next query
       * */
      ResultSet_var
      execute_statement_with_cursor(
        Statement* statement,
        unsigned long portion_size,
        DATA_FORMATS format = AUTO)
        /*throw(SqlException, Postgres::Exception)*/;

      void bad() noexcept;

    protected:
      friend class ResultSet;
      friend class Lob;

      void create_connection_() /*throw(ConnectionError)*/;

      void close_cursor_() /*throw(Postgres::Exception)*/;

      virtual ~Connection() noexcept;

      PGresult* next_() /*throw(Postgres::Exception)*/;

      void clear_result_() noexcept;

      void terminate_i_() noexcept;

      void cancel_i_() noexcept;

      void create_cursor_(Statement* statement, int result_format)
        /*throw(Postgres::Exception)*/;

      PGresult* fetch_from_cursor_()
        /*throw(Postgres::Exception, SqlException)*/;

      void prepare_params_(
        Statement* statement,
        std::vector<const char*>& params_values,
        std::vector<int>& params_lens)
        noexcept;

      virtual
      void delete_this_() const noexcept;

      PGconn* connection_() noexcept;

    private:
      std::string con_info_;
      ConnectionOwner_var connection_owner_;
      PGconn *conn_;
      std::string cursor_name_;
      unsigned long cursor_size_;
      bool have_result_set_;
      bool end_cursor_;
      volatile sig_atomic_t bad_;
      std::set<std::string> prepared_statements_;
    };

    typedef ReferenceCounting::SmartPtr<Connection> Connection_var;
  }
}
}

namespace AdServer
{
namespace Commons
{
  namespace Postgres
  {
    inline
    PGconn* Connection::connection_() noexcept
    {
      return conn_;
    }
  }
}
}

#endif //POSTGRES_CONNECTION_HPP

