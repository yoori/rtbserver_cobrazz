#include<libpq-fe.h>
#include<Stream/MemoryStream.hpp>
#include<Commons/Postgres/SqlStream.hpp>
#include"Statement.hpp"

namespace AdServer
{
namespace Commons
{
  namespace Postgres
  {
    Statement::Statement(const char* query, const char* name) noexcept
      : query_(query)
    {
      if(name)
      {
        name_ = name;
      }
      else
      {
        name_ = query;
      }
    }

    void Statement::prepare_query(PGconn* conn, int count_params)
      /*throw(SqlException)*/
    {
      PGresult* res = PQprepare(
        conn,
        name_.c_str(),
        query_.c_str(),
        count_params,
        NULL);
      ExecStatusType status = PQresultStatus(res);
      PQclear(res);
      if(status != PGRES_COMMAND_OK)
      {
        Stream::Error err;
        err << "Preparing statement '" << name_
          << "' failed: " << PQresultErrorMessage(res);
        throw SqlException(err);
      }
    }

    void Statement::set_date(unsigned int param_num, const Generics::Time& date)
      /*throw(eh::Exception)*/
    {
      add_param_(date.get_gm_time().format("%Y-%m-%d"), param_num - 1);
    }

    void Statement::set_time(
      unsigned int param_num,
      const Generics::Time& time)
      /*throw(eh::Exception)*/
    {
      add_param_(time.get_gm_time().format("%H:%M:%S.%q"), param_num - 1);
    }

    void Statement::set_timestamp(
      unsigned int param_num,
      const Generics::Time& timestamp)
      /*throw(eh::Exception)*/
    {
      Generics::ExtendedTime ex_timestamp = timestamp.get_gm_time();
      std::string timestamp_str = ex_timestamp.format("%Y-%m-%d %H:%M:%S.%q");
      add_param_(timestamp_str, param_num - 1);
    }

    void Statement::set_array(
      unsigned int param_num,
      const ObjectVector& objs)
      /*throw(Exception)*/
    {
      std::ostringstream ostr;
      ostr << "{";
      for (auto it = objs.begin(); it != objs.end(); it++)
      {
        if (it != objs.begin())
        {
          ostr << ',';
        }
        SqlStream_var stream = new SqlStream();
        (*it)->writeSQL(*stream);
        ostr << "\"(" << stream->str() << ")\"";
      }
      ostr << "}";
      add_param_(ostr.str(), param_num - 1);
    }

  }
}
}

