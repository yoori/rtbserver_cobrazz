#include<libpq-fe.h>
#include<ReferenceCounting/Deque.hpp>
#include "Environment.hpp"
#include "Connection.hpp"
#include "ConnectionPool.hpp"


namespace AdServer
{
namespace Commons
{
  namespace Postgres
  {
    Environment::Environment(const char* conninfo) noexcept
      : conn_info_(conninfo),
        count_(new CountActiveObject),
        children_(new Generics::CompositeSetActiveObject)
    {
      add_child_object(count_);
      add_child_object(children_);
    }

    Connection_var
    Environment::create_connection(ConnectionOwner* owner)
      /*throw(ConnectionError, eh::Exception)*/
    {
      if(!owner)
      {
        owner = this;
      }
      Connection_var res = new Connection(conn_info_.c_str(), owner);
      count_->add_active_count(1, true);
      return res;
    }

    ConnectionPool_var
    Environment::create_connection_pool(unsigned long max_conn)
      /*throw(ConnectionError)*/
    {
      ConnectionPool_var res =
        new Commons::Postgres::ConnectionPool(this, children_, max_conn);
      children_->add_child_object(res);
      return res;
    }

    bool
    Environment::destroy_connection(Connection*)
      noexcept
    {
      return true;
    }

    void
    Environment::connection_destroyed()
      noexcept
    {
      count_->add_active_count(-1);
    }
  }
}
}
