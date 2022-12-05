#include "ConnectionPool.hpp"
#include "Environment.hpp"

namespace AdServer
{
namespace Commons
{
  namespace Postgres
  {
    ConnectionPool::ConnectionPool(
      Environment* env,
      Generics::ActiveObjectChildRemover* child_remover,
      unsigned int max_connections) noexcept
      : Generics::RemovableActiveObject(child_remover),
        env_(env),
        max_connections_(max_connections)
    {
    }

    void ConnectionPool::deactivate_object()
      noexcept
    {
      ConnectionCont free_connections;
      {
        Sync::PosixGuard lock(free_connection_cond_);
        CountActiveObject::deactivate_object();
        free_connections_.swap(free_connections);
      }
      free_connections.clear();
    }

    Connection_var
    ConnectionPool::get_connection(bool wait_free_connection)
      /*throw(Postgres::Exception, Postgres::NotActive, MaxConnectionsReached)*/
    {
      try
      {
        bool create_connection = false;
        Connection_var ret;
        while (active())
        {
          Sync::ConditionalGuard guard(free_connection_cond_);
          ret = get_free_connection_i_();
          if (ret.in())
          {
            return ret;
          }
          if (max_connections_ && active_count() >= max_connections_)
          {
            if(!wait_free_connection)
            {
              throw MaxConnectionsReached("");
            }
            guard.wait();
          }
          else
          {
            if (add_active_count(1))
            {
              create_connection = true;
              break;
            }
            else
            {
              throw Postgres::NotActive("pool isn't active");
            }
          }
        }
        if (create_connection)
        {
          try
          {
            ret = create_connection_i_();
            return ret;
          }
          catch (...)
          {
            add_active_count(-1);
            throw;
          }
        }
      }
      catch(const eh::Exception& e)
      {
        Stream::Error err;
        err << __func__ << " exception " << e.what();
        throw Postgres::Exception(err);
      }
      throw Postgres::NotActive("pool isn't active");
    }

    Connection_var ConnectionPool::get_free_connection_i_()
      /*throw(ConnectionError, eh::Exception)*/
    {
      Connection_var res;
      if (!free_connections_.empty())
      {
        res = std::move(free_connections_.front());
        free_connections_.pop_front();
      }
      return res;
    }

    Connection_var ConnectionPool::create_connection_i_()
      /*throw(ConnectionError, eh::Exception)*/
    {
      Connection_var new_conn = env_->create_connection(this);
      return new_conn;
    }

    void ConnectionPool::bad_connection(Connection* conn) noexcept
    {
      if (conn)
      {
        conn->bad();
      }
    }

    bool
    ConnectionPool::destroy_connection(Connection* conn) noexcept
    {
      assert(conn);
      if (active() && !conn->is_bad())
      {
        Sync::PosixGuard lock(free_connection_cond_);
        Connection_var temp = ReferenceCounting::add_ref(conn);
        free_connections_.push_back(temp);
        free_connection_cond_.signal();
        return false;
      }
      add_active_count(-1);
      return env_->destroy_connection(conn);
    }

    void
    ConnectionPool::connection_destroyed() noexcept
    {
      env_->connection_destroyed();
    }
  }
}
}

