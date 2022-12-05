#include<libpq-fe.h>
#include "Connection.hpp"
#include<Generics/Rand.hpp>
#include<Commons/Postgres/ResultSet.hpp>
#include<iostream>

namespace AdServer
{
namespace Commons
{
  namespace Postgres
  {

    Connection::Connection(const char* coninfo, ConnectionOwner* owner)
      /*throw(ConnectionError)*/
      : con_info_(coninfo),
        connection_owner_(ReferenceCounting::add_ref(owner)),
        have_result_set_(false),
        bad_(false)
    {
      create_connection_();
    }

    Connection::~Connection() noexcept
    {
      terminate_i_();
      if(connection_owner_.in())
      {
        connection_owner_->connection_destroyed();
      }
    }

    void Connection::create_connection_() /*throw(ConnectionError)*/
    {
      conn_ = PQconnectdb(con_info_.c_str());
      if(PQstatus(conn_) != CONNECTION_OK)
      {
        Stream::Error err;
        err << "Connection to database failed: " << PQerrorMessage(conn_);
        PQfinish(conn_);
        throw ConnectionError(err);
      }
      prepared_statements_.clear();
    }

    void Connection::close_cursor_() /*throw(Postgres::Exception)*/
    {
      if (!cursor_name_.empty())
      {
        Stream::Error query;
        query << "CLOSE " << cursor_name_ << ";";
        try
        {
          execute_query_(query.str().str().c_str());
          execute_query_("COMMIT;");
        }
        catch(const Postgres::Exception& e)
        {//don't throw exception it can be executed from destructor
          std::cerr << "error on close cursor :" << e.what() << std::endl;
        }
        cursor_name_.clear();
      }
    }

    void Connection::terminate_i_() noexcept
    {
      if(conn_)
      {
        PQfinish(conn_);
        conn_ = 0;
        prepared_statements_.clear();
      }
    }

    void Connection::cancel_i_() noexcept
    {
      PGcancel* cancel_obj = PQgetCancel(conn_);
      if(cancel_obj)
      {
        char errbuf[256];
        PQcancel(cancel_obj, errbuf, sizeof(errbuf));
        PQfreeCancel(cancel_obj);
      }
    }

    void Connection::bad() noexcept
    {
      bad_ = true;
    }

    bool Connection::is_free() const noexcept
    {
      return (ref_count_ == 1);
    }

    bool Connection::is_bad() const noexcept
    {
      return (bad_ == true);
    }

    PGresultPtr
    Connection::execute_query_(const char* query)
      /*throw(SqlException, Postgres::Exception)*/
    {
      PGresultPtr res_pg(PQexec(conn_, query), PQclear);
      ExecStatusType status = PQresultStatus(res_pg.get());
      if(status != PGRES_TUPLES_OK &&
         status != PGRES_COMMAND_OK)
      {
        Stream::Error err;
        err << "Query to database failed: ("
          << PQresStatus(status) << ") " 
          << PQresultErrorMessage(res_pg.get());
        throw SqlException(err);
      }
      return res_pg;
    }

    ResultSet_var
    Connection::execute_query(const char* query)
      /*throw(SqlException, Postgres::Exception)*/
    {
      clear_result_();
      return new ResultSet(this, execute_query_(query).release());
    }

    void Connection::create_cursor_(
      Statement* statement,
      int result_format)
      /*throw(Postgres::Exception)*/
    {
      Stream::Error decl_cursor;
      try
      {
        std::vector<const char*> params_values;
        std::vector<int> params_lens;
        if (PQsendQuery(conn_, "BEGIN;"))
        {
          char name[32];
          for(auto i = 0; i < 32; ++i)
          {
            name[i] = static_cast<char>(Generics::safe_rand(0x61, 0x7A));
          }
          cursor_name_.assign(name, 32);
          decl_cursor << "DECLARE " << cursor_name_
            << (result_format == BINARY_FORMAT ? " BINARY" : "") 
            << " CURSOR FOR " << statement->query() << ";";
          prepare_params_(statement, params_values, params_lens);
          PGresultPtr pg_res(PQgetResult(conn_), PQclear);
          ExecStatusType status = PQresultStatus(pg_res.get());
          if(status != PGRES_COMMAND_OK)
          {
            Stream::Error err;
            err << '(' << PQresStatus(status) << ") "
              << PQresultErrorMessage(pg_res.get());
            throw Postgres::Exception(err);
          }
        }
        else
        {
          throw Postgres::Exception("can not start transaction");
        }
        PGresult* pg_res = PQexecParams(
          conn_,
          decl_cursor.str().str().c_str(),
          params_values.size(),
          NULL,
          &params_values.front(),
          &params_lens.front(),
          NULL,
          result_format);
        PGresultPtr res(pg_res, PQclear);
        ExecStatusType status = PQresultStatus(pg_res);
        if(status != PGRES_TUPLES_OK &&
           status != PGRES_COMMAND_OK)
        {
          Stream::Error err;
          err << '(' << PQresStatus(status) << ") "
            << PQresultErrorMessage(pg_res);
          throw Postgres::Exception(err);
        }
        //execute_query_("COMMIT;");
      }
      catch(const Postgres::Exception& e)
      {
        cursor_name_.clear();
        Stream::Error err;
        err << __func__ << ": error on creating cursor, query '" 
          << decl_cursor.str() << "': " << e.what();
        throw Postgres::Exception(e);
      }
    }

    PGresult*
    Connection::fetch_from_cursor_()
      /*throw(Postgres::Exception, SqlException)*/
    {
      if (!end_cursor_)
      {
        Stream::Error query;
        query << "FETCH " << cursor_size_ << " FROM " << cursor_name_;
        Generics::Timer timer;
        timer.start();
        PGresult* res_pg = PQexec(conn_, query.str().str().c_str());
        ExecStatusType status = PQresultStatus(res_pg);
        if(status != PGRES_TUPLES_OK &&
           status != PGRES_COMMAND_OK)
        {
          Stream::Error err;
          err << "Query to database failed: ("
            << PQresStatus(status) << ") "
            << PQresultErrorMessage(res_pg);
          throw SqlException(err);
        }
        if (static_cast<unsigned long>(PQntuples(res_pg)) < cursor_size_)
        {//assume if cursor isn't full is last portion
          end_cursor_ = true;
        }
        timer.stop();
        //std::cout << cursor_size_ << ":fetch portion: " << timer.elapsed_time() << std::endl;
        return res_pg;
      }
      return 0;
    }

    void Connection::prepare_params_(
      Statement* statement,
      std::vector<const char*>& params_values,
      std::vector<int>& params_lens)
      noexcept
    {
      const std::vector<std::string>& params = statement->params();
      params_values.resize(params.size());
      params_lens.resize(params.size());
      for(size_t i = 0; i < params.size(); i++)
      {
        params_values[i] = params[i].c_str();
        params_lens[i] = params[i].size();
      }
    }

    ResultSet_var
    Connection::execute_statement_with_cursor(
      Statement* statement,
      unsigned long portion_size,
      DATA_FORMATS format)
      /*throw(SqlException, Postgres::Exception)*/
    {
      clear_result_();
      int result_format = TEXT_FORMAT;//text format
#ifdef BINARY_RECIVING_DATA
      if(format == AUTO || format == BINARY_FORMAT)
      {
        result_format = BINARY_FORMAT;
      }
#endif
      create_cursor_(statement, result_format);
      cursor_size_ = portion_size;
      end_cursor_ = false;
      PGresult* pg_res = fetch_from_cursor_();
      ResultSet_var res = new ResultSet(this, pg_res);
      return res;
    }

    ResultSet_var
    Connection::execute_statement(
      Statement* statement,
      bool sync,
      DATA_FORMATS format)
      /*throw(SqlException, Postgres::Exception)*/
    {
      clear_result_();
      const std::vector<std::string>& params = statement->params();
      std::vector<const char*> params_values;
      std::vector<int> params_lens;
      prepare_params_(statement, params_values, params_lens);
      if(prepared_statements_.find(statement->name()) ==
         prepared_statements_.end())
      {
        statement->prepare_query(conn_, params.size());
        prepared_statements_.insert(statement->name());
      }
      int result_format = TEXT_FORMAT;//text format
#ifdef BINARY_RECIVING_DATA
      if(format == AUTO || format == BINARY_FORMAT)
      {
        result_format = BINARY_FORMAT;
      }
#endif

      ResultSet_var res;
      if(sync)
      {
        PGresult *pg_res = PQexecPrepared(
          conn_,
          statement->name().c_str(),
          params_values.size(),
          &params_values.front(),
          &params_lens.front(),
          NULL,
          result_format);
        PGresultPtr res_ptr(pg_res, PQclear);
        ExecStatusType status = PQresultStatus(pg_res);
        if(status != PGRES_TUPLES_OK &&
           status != PGRES_SINGLE_TUPLE &&
           status != PGRES_COMMAND_OK)
        {
          Stream::Error err;
          err << __func__ << ": Execution statement '" << statement->name()
            << "' failed: (" << PQresStatus(status) << ") "
            << PQresultErrorMessage(pg_res);
          throw SqlException(err);
        }
        res = new ResultSet(this, res_ptr.release());
      }
      else
      {
        int res_status = PQsendQueryPrepared(
          conn_,
          statement->name().c_str(),
          params_values.size(),
          &params_values.front(),
          &params_lens.front(),
          NULL,
          result_format);
        if (res_status != 1)
        {//error sending command
          Stream::Error err;
          err << __func__ <<  ": Can not send prepared statement '" << statement->name()
            << "' failed: " <<  PQerrorMessage(conn_);
          throw Postgres::Exception(err);
        }
        if (PQsetSingleRowMode(conn_) == 0)
        {//I guess need to cancel query
          cancel_i_();
          Stream::Error err;
          err << __func__ <<  ": Can set single row mode for statement '"
            << statement->name() << "' failed: " <<  PQerrorMessage(conn_);
          throw Postgres::Exception(err);
        }
        res = new ResultSet(this);
        have_result_set_ = true;
      }
      return res;
    }

    PGresult* Connection::next_() /*throw(Postgres::Exception)*/
    {//executed by resultset
      if (!cursor_name_.empty())
      {
        return fetch_from_cursor_();
      }
      if(!have_result_set_)
      {
        return 0;
      }

      ExecStatusType status;

      PGresultPtr res(PQgetResult(conn_), PQclear);
      if(res.get())
      {
        status = PQresultStatus(res.get());

        if(status != PGRES_SINGLE_TUPLE)
        {//save first error
          if(status != PGRES_TUPLES_OK)
          {
            Stream::Error err;
            err << __func__ << ": error on fetching row ("
              << PQresStatus(status) << ") : "
              << PQresultErrorMessage(res.get());
            // should fetch all result even in case error
            clear_result_();
            throw Postgres::Exception(err);
          }
        }
      }
      else
      {
        have_result_set_ = false;
      }
      return res.release();
    }

    void Connection::clear_result_() noexcept
    {
      if (have_result_set_)
      {
        PGresult* res_pg;
        while((res_pg = PQgetResult(conn_)))
        {
          PQclear(res_pg);
        }
        have_result_set_ = false;
      }
      try
      {
        close_cursor_();
      }
      catch(const Postgres::Exception& e)
      {//don't throw exception it can be executed from destructor
        std::cerr << __func__ << ": error on close cursor :" << e.what();
      }
    }

    void
    Connection::delete_this_() const noexcept
    {
      bool delete_this = true;
      if (connection_owner_.in())
      {
        Connection* non_const_this = const_cast<Connection*>(this);

        delete_this = non_const_this->connection_owner_->destroy_connection(
          non_const_this);
      }
      if (delete_this)
      {
        delete this;
      }
    }
  }
}
}

